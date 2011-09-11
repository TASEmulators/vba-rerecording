#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <algorithm>

using namespace std;

#ifdef HAVE_STRINGS_H
#   include <strings.h>
#endif

#if defined(__unix) || defined(__linux) || defined(__sun) || defined(__DJGPP)
#   include <unistd.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <climits>
#   define stricmp strcasecmp
// FIXME: this is wrong, but we don't want buffer overflow
#   if defined _MAX_PATH
#       undef _MAX_PATH
//#       define _MAX_PATH 128
#       define _MAX_PATH 260
#   endif
#endif

#ifdef WIN32
#   include <io.h>
#   ifndef W_OK
#       define W_OK 2
#   endif
#   define ftruncate chsize
#endif

#include "movie.h"
#include "System.h"
#include "../gba/GBA.h"
#include "../gba/GBAGlobals.h"
#include "../gba/RTC.h"
#include "../gb/GB.h"
#include "../gb/gbGlobals.h"
#include "inputGlobal.h"
#include "unzip.h"
#include "Util.h"

#include "vbalua.h"

#if (defined(WIN32) && !defined(SDL))
#   include "../win32/stdafx.h"
#   include "../win32/MainWnd.h"
#   include "../win32/VBA.h"
#   include "../win32/WinMiscUtil.h"
#endif

extern int emulating; // from system.cpp
extern u16 currentButtons[4];     // from System.cpp
extern u16 lastKeys;

SMovie Movie;
bool   loadingMovie = false;

// probably bad idea to have so many global variables, but I hate to recompile almost everything after editing VBA.h
bool autoConvertMovieWhenPlaying = false;

static u16 initialInputs[4] = { 0 };

static bool resetSignaled	  = false;
static bool resetSignaledLast = false;

static int prevEmulatorType, prevBorder, prevWinBorder, prevBorderAuto;

// little-endian integer pop/push functions:
static inline uint32 Pop32(const uint8 * &ptr)
{
	uint32 v = (ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24));
	ptr += 4;
	return v;
}

static inline uint16 Pop16(const uint8 * &ptr) /* const version */
{
	uint16 v = (ptr[0] | (ptr[1] << 8));
	ptr += 2;
	return v;
}

static inline uint16 Pop16(uint8 * &ptr) /* non-const version */
{
	uint16 v = (ptr[0] | (ptr[1] << 8));
	ptr += 2;
	return v;
}

static inline uint8 Pop8(const uint8 * &ptr)
{
	return *(ptr)++;
}

static inline void Push32(uint32 v, uint8 * &ptr)
{
	ptr[0] = (uint8)(v & 0xff);
	ptr[1] = (uint8)((v >> 8) & 0xff);
	ptr[2] = (uint8)((v >> 16) & 0xff);
	ptr[3] = (uint8)((v >> 24) & 0xff);
	ptr	  += 4;
}

static inline void Push16(uint16 v, uint8 * &ptr)
{
	ptr[0] = (uint8)(v & 0xff);
	ptr[1] = (uint8)((v >> 8) & 0xff);
	ptr	  += 2;
}

static inline void Push8(uint8 v, uint8 * &ptr)
{
	*ptr++ = v;
}

// little-endian integer read/write functions:
static inline uint16 Read16(const uint8 *ptr)
{
	return ptr[0] | (ptr[1] << 8);
}

static inline void Write16(uint16 v, uint8 *ptr)
{
	ptr[0] = uint8(v & 0xff);
	ptr[1] = uint8((v >> 8) & 0xff);
}

static long file_length(FILE *fp)
{
	long cur_pos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	long length = ftell(fp);
	fseek(fp, cur_pos, SEEK_SET);
	return length;
}

static int bytes_per_frame(SMovie &mov)
{
	int num_controllers = 0;

	for (int i = 0; i < MOVIE_NUM_OF_POSSIBLE_CONTROLLERS; ++i)
		if (mov.header.controllerFlags & MOVIE_CONTROLLER(i))
			++num_controllers;

	return CONTROLLER_DATA_SIZE * num_controllers;
}

static void reserve_buffer_space(uint32 space_needed)
{
	if (space_needed > Movie.inputBufferSize)
	{
		uint32 ptr_offset	= Movie.inputBufferPtr - Movie.inputBuffer;
		uint32 alloc_chunks = (space_needed - 1) / BUFFER_GROWTH_SIZE + 1;
		Movie.inputBufferSize = BUFFER_GROWTH_SIZE * alloc_chunks;
		Movie.inputBuffer	  = (uint8 *)realloc(Movie.inputBuffer, Movie.inputBufferSize);
		Movie.inputBufferPtr  = Movie.inputBuffer + ptr_offset;
	}
}

static int read_movie_header(FILE *file, SMovie &movie)
{
	assert(file != NULL);
	assert(VBM_HEADER_SIZE == sizeof(SMovieFileHeader)); // sanity check on the header type definition

	uint8 headerData [VBM_HEADER_SIZE];

	if (fread(headerData, 1, VBM_HEADER_SIZE, file) != VBM_HEADER_SIZE)
		return MOVIE_WRONG_FORMAT;  // if we failed to read in all VBM_HEADER_SIZE bytes of the header

	const uint8 *	  ptr	 = headerData;
	SMovieFileHeader &header = movie.header;

	header.magic = Pop32(ptr);
	if (header.magic != VBM_MAGIC)
		return MOVIE_WRONG_FORMAT;

	header.version = Pop32(ptr);
	if (header.version != VBM_VERSION)
		return MOVIE_WRONG_VERSION;

	header.uid = Pop32(ptr);
	header.length_frames  = Pop32(ptr) + 1;    // HACK: add 1 to the length for compatibility
	header.rerecord_count = Pop32(ptr);

	header.startFlags	   = Pop8(ptr);
	header.controllerFlags = Pop8(ptr);
	header.typeFlags	   = Pop8(ptr);
	header.optionFlags	   = Pop8(ptr);

	header.saveType		  = Pop32(ptr);
	header.flashSize	  = Pop32(ptr);
	header.gbEmulatorType = Pop32(ptr);

	for (int i = 0; i < 12; i++)
		header.romTitle[i] = Pop8(ptr);

	header.minorVersion = Pop8(ptr);

	header.romCRC = Pop8(ptr);
	header.romOrBiosChecksum = Pop16(ptr);
	header.romGameCode		 = Pop32(ptr);

	header.offset_to_savestate		 = Pop32(ptr);
	header.offset_to_controller_data = Pop32(ptr);

	return MOVIE_SUCCESS;
}

static void write_movie_header(FILE *file, const SMovie &movie)
{
	assert(ftell(file) == 0); // we assume file points to beginning of movie file

	uint8  headerData [VBM_HEADER_SIZE];
	uint8 *ptr = headerData;
	const SMovieFileHeader &header = movie.header;

	Push32(header.magic, ptr);
	Push32(header.version, ptr);

	Push32(header.uid, ptr);
	Push32(header.length_frames - 1, ptr);     // HACK: reduce the length by 1 for compatibility with certain faulty old tools
	                                           // like TME
	Push32(header.rerecord_count, ptr);

	Push8(header.startFlags, ptr);
	Push8(header.controllerFlags, ptr);
	Push8(header.typeFlags, ptr);
	Push8(header.optionFlags, ptr);

	Push32(header.saveType, ptr);
	Push32(header.flashSize, ptr);
	Push32(header.gbEmulatorType, ptr);

	for (int i = 0; i < 12; ++i)
		Push8(header.romTitle[i], ptr);

	Push8(header.minorVersion, ptr);

	Push8(header.romCRC, ptr);
	Push16(header.romOrBiosChecksum, ptr);
	Push32(header.romGameCode, ptr);

	Push32(header.offset_to_savestate, ptr);
	Push32(header.offset_to_controller_data, ptr);

	fwrite(headerData, 1, VBM_HEADER_SIZE, file);
}

static void flush_movie_header()
{
	assert(Movie.file != 0 && "logical error!");
	if (!Movie.file)
		return;

	long originalPos = ftell(Movie.file);

	// (over-)write the header
	fseek(Movie.file, 0, SEEK_SET);
	write_movie_header(Movie.file, Movie);

	fflush(Movie.file);

	fseek(Movie.file, originalPos, SEEK_SET);
}

static void flush_movie_frames()
{
	assert(Movie.file && "logical error!");
	if (!Movie.file)
		return;

	long originalPos = ftell(Movie.file);

	// overwrite the controller data
	fseek(Movie.file, Movie.header.offset_to_controller_data, SEEK_SET);
	fwrite(Movie.inputBuffer, 1, Movie.bytesPerFrame * Movie.header.length_frames, Movie.file);

	fflush(Movie.file);

	fseek(Movie.file, originalPos, SEEK_SET);
}

static void truncate_movie(long length)
{
	// truncate movie to length
	// NOTE: it's certain that the savestate block is never after the
	//       controller data block, because the VBM format decrees it.

	assert(Movie.file && length >= 0);
	if (!Movie.file || length < 0)
		return;

	assert(Movie.header.offset_to_savestate <= Movie.header.offset_to_controller_data);
	if (Movie.header.offset_to_savestate > Movie.header.offset_to_controller_data)
		return;

	Movie.header.length_frames = length;
	flush_movie_header();
	const long truncLen = long(Movie.header.offset_to_controller_data + Movie.bytesPerFrame * length);
	if (file_length(Movie.file) != truncLen)
	{
		ftruncate(fileno(Movie.file), truncLen);
	}
}

static void remember_input_state()
{
	for (int i = 0; i < MOVIE_NUM_OF_POSSIBLE_CONTROLLERS; ++i)
	{
		if (systemCartridgeType == 0)
		{
			initialInputs[i] = u16(~P1 & 0x03FF);
		}
		else
		{
			extern int32 gbJoymask[4];
			for (int i = 0; i < 4; ++i)
				initialInputs[i] = u16(gbJoymask[i] & 0xFFFF);
		}
	}
}

static void change_state(MovieState new_state)
{
	if (Movie.state == MOVIE_STATE_RECORD)
	{
		//truncate_movie(Movie.header.length_frames);
	}

#if (defined(WIN32) && !defined(SDL))
	theApp.frameSearching	   = false;
	theApp.frameSearchSkipping = false;
#endif

	if (new_state == MOVIE_STATE_NONE)
	{
		Movie.pauseFrame = -1;

		if (Movie.state == MOVIE_STATE_NONE)
			return;

		truncate_movie(Movie.header.length_frames);

		fclose(Movie.file);
		Movie.file		   = NULL;
		Movie.currentFrame = 0;
#if (defined(WIN32) && !defined(SDL))
		// undo changes to border settings
		{
			gbBorderOn = prevBorder;
			theApp.winGbBorderOn = prevWinBorder;
			gbBorderAutomatic	 = prevBorderAuto;
			systemGbBorderOn();
		}
#endif
		gbEmulatorType = prevEmulatorType;

		extern int32 gbDMASpeedVersion;
		gbDMASpeedVersion = 1;

		extern int32 gbEchoRAMFixOn;
		gbEchoRAMFixOn = 1;

		gbNullInputHackTempEnabled = gbNullInputHackEnabled;

		if (Movie.inputBuffer)
		{
			free(Movie.inputBuffer);
			Movie.inputBuffer = NULL;
		}
	}
	else if (new_state == MOVIE_STATE_PLAY)
	{
		assert(Movie.file);

		// this would cause problems if not dealt with
		if (Movie.currentFrame >= Movie.header.length_frames)
		{
			new_state = MOVIE_STATE_END;
			Movie.inputBufferPtr = Movie.inputBuffer + Movie.bytesPerFrame * Movie.header.length_frames;
		}
	}
	else if (new_state == MOVIE_STATE_RECORD)
	{
		assert(Movie.file);

		// this would cause problems if not dealt with
		if (Movie.currentFrame > Movie.header.length_frames)
		{
			new_state = MOVIE_STATE_END;
			Movie.inputBufferPtr = Movie.inputBuffer + Movie.bytesPerFrame * Movie.header.length_frames;
		}

		fseek(Movie.file, Movie.header.offset_to_controller_data + Movie.bytesPerFrame * Movie.currentFrame, SEEK_SET);
	}

	if (new_state == MOVIE_STATE_END && Movie.state != MOVIE_STATE_END)
	{
		systemScreenMessage("Movie end");
	}

	Movie.state = new_state;

	// checking for movie end
	bool willPause = false;

	// if the movie's been set to pause at a certain frame
	if (Movie.state != MOVIE_STATE_NONE && Movie.pauseFrame >= 0 && Movie.currentFrame == (uint32)Movie.pauseFrame)
	{
		Movie.pauseFrame = -1;
		willPause		 = true;
	}

	if (Movie.state == MOVIE_STATE_END)
	{
		if (Movie.currentFrame == Movie.header.length_frames)
		{
#if (defined(WIN32) && !defined(SDL))
			if (theApp.movieOnEndPause)
			{
				willPause = true;
			}
#else
			// SDL FIXME
#endif

#if (defined(WIN32) && !defined(SDL))
			switch (theApp.movieOnEndBehavior)
			{
			case 1:
				// the old behavior
				//VBAMovieRestart();
				break;
			case 2:
#else
			// SDL FIXME
#endif
				if (Movie.RecordedThisSession)
				{
					// if user has been recording this movie since the last time it started playing,
					// they probably don't want the movie to end now during playback,
					// so switch back to recording when it reaches the end
					VBAMovieSwitchToRecording();
					systemScreenMessage("Recording resumed");
					willPause = true;
				}
#if (defined(WIN32) && !defined(SDL))
				break;
			case 3:
				// keep open
				break;
			case 0:
				// fall through
			default:
				// close movie
				//VBAMovieStop(false);
				break;
			}
#else
				// SDL FIXME
#endif
		}
#if 1
		else if (Movie.currentFrame > Movie.header.length_frames)
		{
#if (defined(WIN32) && !defined(SDL))
			switch (theApp.movieOnEndBehavior)
			{
			case 1:
				// FIXME: this should be delayed till the current frame ends
				VBAMovieRestart();
				break;
			case 2:
				// nothing
				break;
			case 3:
				// keep open
				break;
			case 0:
				// fall through
			default:
				// close movie
				VBAMovieStop(false);
				break;
			}
#else
			// SDLFIXME
#endif
		}
#endif
	} // end if (Movie.state == MOVIE_STATE_END)

	if (willPause)
	{
		systemSetPause(true);
	}
}

void VBAMovieInit()
{
	memset(&Movie, 0, sizeof(Movie));
	Movie.state		 = MOVIE_STATE_NONE;
	Movie.pauseFrame = -1;

	resetSignaled	  = false;
	resetSignaledLast = false;
}

void VBAMovieGetRomInfo(const SMovie &movieInfo, char romTitle [12], uint32 &romGameCode, uint16 &checksum, uint8 &crc)
{
	if (systemCartridgeType == 0) // GBA
	{
		extern u8 *bios, *rom;
		memcpy(romTitle, &rom[0xa0], 12); // GBA TITLE
		memcpy(&romGameCode, &rom[0xac], 4); // GBA ROM GAME CODE
		if ((movieInfo.header.optionFlags & MOVIE_SETTING_USEBIOSFILE) != 0)
			checksum = utilCalcBIOSChecksum(bios, 4);  // GBA BIOS CHECKSUM
		else
			checksum = 0;
		crc = rom[0xbd]; // GBA ROM CRC
	}
	else // non-GBA
	{
		extern u8 *gbRom;
		memcpy(romTitle, &gbRom[0x134], 12); // GB TITLE (note this can be 15 but is truncated to 12)
		romGameCode = (uint32)gbRom[0x146]; // GB ROM UNIT CODE

		checksum = (gbRom[0x14e] << 8) | gbRom[0x14f]; // GB ROM CHECKSUM, read from big-endian
		crc		 = gbRom[0x14d]; // GB ROM CRC
	}
}

#ifdef SDL
static void GetBatterySaveName(char *buffer)
{
	extern char batteryDir[2048], filename[2048];     // from SDL.cpp
	extern char *sdlGetFilename(char *name);     // from SDL.cpp
	if (batteryDir[0])
		sprintf(buffer, "%s/%s.sav", batteryDir, sdlGetFilename(filename));
	else
		sprintf(buffer, "%s.sav", filename);
}

#endif

static void SetPlayEmuSettings()
{
	prevEmulatorType = gbEmulatorType;
	gbEmulatorType	 = Movie.header.gbEmulatorType;

#if (defined(WIN32) && !defined(SDL))
//    theApp.removeIntros   = false;
	theApp.skipBiosFile = (Movie.header.optionFlags & MOVIE_SETTING_SKIPBIOSFILE) != 0;
	theApp.useBiosFile	= (Movie.header.optionFlags & MOVIE_SETTING_USEBIOSFILE) != 0;
#else
	extern int	 saveType, sdlRtcEnable, sdlFlashSize;   // from SDL.cpp
	extern bool8 useBios, skipBios, removeIntros;     // from SDL.cpp
	useBios		 = (Movie.header.optionFlags & MOVIE_SETTING_USEBIOSFILE) != 0;
	skipBios	 = (Movie.header.optionFlags & MOVIE_SETTING_SKIPBIOSFILE) != 0;
	removeIntros = false /*(Movie.header.optionFlags & MOVIE_SETTING_REMOVEINTROS) != 0*/;
#endif

	extern void SetPrefetchHack(bool);
	if (systemCartridgeType == 0)    // lag disablement applies only to GBA
		SetPrefetchHack((Movie.header.optionFlags & MOVIE_SETTING_LAGHACK) != 0);

	gbNullInputHackTempEnabled = ((Movie.header.optionFlags & MOVIE_SETTING_GBINPUTHACK) != 0);

	// some GB/GBC games depend on the sound rate, so just use the highest one
	systemSoundSetQuality(1);
	useOldFrameTiming = false;

	extern int32 gbDMASpeedVersion;
	if ((Movie.header.optionFlags & MOVIE_SETTING_GBCFF55FIX) != 0)
		gbDMASpeedVersion = 1;
	else
		gbDMASpeedVersion = 0;     // old CGB HDMA5 timing was used

	extern int32 gbEchoRAMFixOn;
	if ((Movie.header.optionFlags & MOVIE_SETTING_GBECHORAMFIX) != 0)
		gbEchoRAMFixOn = 1;
	else
		gbEchoRAMFixOn = 0;

#if (defined(WIN32) && !defined(SDL))
	rtcEnable((Movie.header.optionFlags & MOVIE_SETTING_RTCENABLE) != 0);
	theApp.winSaveType	= Movie.header.saveType;
	theApp.winFlashSize = Movie.header.flashSize;

	prevBorder	   = gbBorderOn;
	prevWinBorder  = theApp.winGbBorderOn;
	prevBorderAuto = gbBorderAutomatic;
	if ((gbEmulatorType == 2 || gbEmulatorType == 5)
	    && !theApp.hideMovieBorder) // games played in SGB mode can have a border
	{
		gbBorderOn = true;
		theApp.winGbBorderOn = true;
		gbBorderAutomatic	 = false;
	}
	else
	{
		gbBorderOn = false;
		theApp.winGbBorderOn = false;
		gbBorderAutomatic	 = false;
		if (theApp.hideMovieBorder)
		{
			theApp.hideMovieBorder = false;
			prevBorder = false;     // it might be expected behaviour that it stays hidden after the movie
		}
	}
	systemGbBorderOn();
#else
	sdlRtcEnable = (Movie.header.optionFlags & MOVIE_SETTING_RTCENABLE) != 0;
	saveType	 = Movie.header.saveType;
	sdlFlashSize = Movie.header.flashSize;
#endif
}

static void HardResetAndSRAMClear()
{
#if (defined(WIN32) && !defined(SDL))
	winEraseBatteryFile(); // delete the damn SRAM file
	extern bool reopenTheSameImage; reopenTheSameImage = true;     // keep it from being resurrected from RAM
	((MainWnd *)theApp.m_pMainWnd)->winFileRun();     // start running the game
#else
	char fname [1024];
	GetBatterySaveName(fname);
	remove(fname);     // delete the damn SRAM file

	// Henceforth, emuCleanUp means "clear out SRAM"
	//theEmulator.emuCleanUp();     // keep it from being resurrected from RAM <--This is wrong, it'll deallocate all variables --Felipe

	/// FIXME the correct SDL code to call for a full restart isn't in a function yet
	theEmulator.emuReset(false);
#endif
}

int VBAMovieOpen(const char *filename, bool8 read_only)
{
	loadingMovie = true;
	uint8 movieReadOnly = read_only ? 1 : 0;

	FILE * file;
	STREAM stream;
	int	   result;
	int	   fn;

	char movie_filename[_MAX_PATH];
#ifdef WIN32
	_fullpath(movie_filename, filename, _MAX_PATH);
#else
	// SDL FIXME: convert to fullpath
	strncpy(movie_filename, filename, _MAX_PATH);
	movie_filename[_MAX_PATH - 1] = '\0';
#endif

	if (movie_filename[0] == '\0')
	{ loadingMovie = false; return MOVIE_FILE_NOT_FOUND; }

	if (!emulating)
	{ loadingMovie = false; return MOVIE_UNKNOWN_ERROR; }

//	bool alreadyOpen = (Movie.file != NULL && _stricmp(movie_filename, Movie.filename) == 0);

//	if (alreadyOpen)
	change_state(MOVIE_STATE_NONE);     // have to stop current movie before trying to re-open it

	if (!(file = fopen(movie_filename, "rb+")))
		if (!(file = fopen(movie_filename, "rb")))
		{ loadingMovie = false; return MOVIE_FILE_NOT_FOUND; }
	//else
	//	movieReadOnly = 2; // we have to open the movie twice, no need to do this both times

//	if (!alreadyOpen)
//		change_state(MOVIE_STATE_NONE); // stop current movie when we're able to open the other one
//
//	if (!(file = fopen(movie_filename, "rb+")))
//		if(!(file = fopen(movie_filename, "rb")))
//			{loadingMovie = false; return MOVIE_FILE_NOT_FOUND;}
//		else
//			movieReadOnly = 2;

	// clear out the current movie
	VBAMovieInit();

	// read header
	if ((result = read_movie_header(file, Movie)) != MOVIE_SUCCESS)
	{
		fclose(file);
		{ loadingMovie = false; return result; }
	}

	// set emulator settings that make the movie more likely to stay synchronized
	SetPlayEmuSettings();

//	extern bool systemLoadBIOS();
//	if (!systemLoadBIOS())
//	{ loadingMovie = false; return MOVIE_UNKNOWN_ERROR; }

	// read the metadata / author info from file
	fread(Movie.authorInfo, 1, MOVIE_METADATA_SIZE, file);
	fn = dup(fileno(file)); // XXX: why does this fail?? it returns -1 but errno == 0
	fclose(file);

	// apparently this lseek is necessary
	lseek(fn, Movie.header.offset_to_savestate, SEEK_SET);
	if (!(stream = utilGzReopen(fn, "rb")))
		if (!(stream = utilGzOpen(movie_filename, "rb")))
		{ loadingMovie = false; return MOVIE_FILE_NOT_FOUND; }
		else
			fn = dup(fileno(file));
	// in case the above dup failed but opening the file normally doesn't fail

	if (Movie.header.startFlags & MOVIE_START_FROM_SNAPSHOT)
	{
		// load the snapshot
		result = theEmulator.emuReadStateFromStream(stream) ? MOVIE_SUCCESS : MOVIE_WRONG_FORMAT;

		// FIXME: Kludge for conversion
		remember_input_state();
	}
	else if (Movie.header.startFlags & MOVIE_START_FROM_SRAM)
	{
		// 'soft' reset:
		theEmulator.emuReset(false);

		// load the SRAM
		result = theEmulator.emuReadBatteryFromStream(stream) ? MOVIE_SUCCESS : MOVIE_WRONG_FORMAT;
	}
	else
	{
		HardResetAndSRAMClear();
	}

	utilGzClose(stream);

	if (result != MOVIE_SUCCESS)
	{ loadingMovie = false; return result; }

//	if (!(file = fopen(movie_filename, /*read_only ? "rb" :*/ "rb+"))) // want to be able to switch out of read-only later
//	{
//		if(!Movie.readOnly || !(file = fopen(movie_filename, "rb"))) // try read-only if failed
//			return MOVIE_FILE_NOT_FOUND;
//	}
	if (!(file = fopen(movie_filename, "rb+")))
		if (!(file = fopen(movie_filename, "rb")))
		{ loadingMovie = false; return MOVIE_FILE_NOT_FOUND; }
		else
			movieReadOnly = 2;

	// recalculate length of movie from the file size
	Movie.bytesPerFrame = bytes_per_frame(Movie);
	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	Movie.header.length_frames = (fileSize - Movie.header.offset_to_controller_data) / Movie.bytesPerFrame;

	if (fseek(file, Movie.header.offset_to_controller_data, SEEK_SET))
	{ fclose(file); loadingMovie = false; return MOVIE_WRONG_FORMAT; }

	strcpy(Movie.filename, movie_filename);
	Movie.file = file;
	Movie.inputBufferPtr	  = Movie.inputBuffer;
	Movie.currentFrame		  = 0;
	Movie.readOnly			  = movieReadOnly;
	Movie.RecordedThisSession = false;

	// read controller data
	uint32 to_read = Movie.bytesPerFrame * Movie.header.length_frames;
	reserve_buffer_space(to_read);
	fread(Movie.inputBuffer, 1, to_read, file);

	change_state(MOVIE_STATE_PLAY);

	char messageString[64] = "Movie ";
	bool converted		   = false;
	if (autoConvertMovieWhenPlaying)
	{
		int result = VBAMovieConvertCurrent();
		if (result == MOVIE_SUCCESS)
			strcat(messageString, "converted and ");
		else if (result == MOVIE_WRONG_VERSION)
			strcat(messageString, "higher revision ");
	}

	if (Movie.state == MOVIE_STATE_PLAY)
		strcat(messageString, "replaying ");
	else
		strcat(messageString, "finished ");
	if (Movie.readOnly)
		strcat(messageString, "(read)");
	else
		strcat(messageString, "(edit)");
	systemScreenMessage(messageString);

	VBAUpdateButtonPressDisplay();
	VBAUpdateFrameCountDisplay();
	systemRefreshScreen();

	{ loadingMovie = false; return MOVIE_SUCCESS; }
}

static void SetRecordEmuSettings()
{
	Movie.header.optionFlags = 0;
#if (defined(WIN32) && !defined(SDL))
	if (theApp.useBiosFile)
		Movie.header.optionFlags |= MOVIE_SETTING_USEBIOSFILE;
	if (theApp.skipBiosFile)
		Movie.header.optionFlags |= MOVIE_SETTING_SKIPBIOSFILE;
	if (rtcIsEnabled())
		Movie.header.optionFlags |= MOVIE_SETTING_RTCENABLE;
	Movie.header.saveType  = theApp.winSaveType;
	Movie.header.flashSize = theApp.winFlashSize;
#else
	extern int	 saveType, sdlRtcEnable, sdlFlashSize;   // from SDL.cpp
	extern bool8 useBios, skipBios;     // from SDL.cpp
	if (useBios)
		Movie.header.optionFlags |= MOVIE_SETTING_USEBIOSFILE;
	if (skipBios)
		Movie.header.optionFlags |= MOVIE_SETTING_SKIPBIOSFILE;
	if (sdlRtcEnable)
		Movie.header.optionFlags |= MOVIE_SETTING_RTCENABLE;
	Movie.header.saveType  = saveType;
	Movie.header.flashSize = sdlFlashSize;
#endif
	prevEmulatorType = Movie.header.gbEmulatorType = gbEmulatorType;

	if (!memLagTempEnabled)
		Movie.header.optionFlags |= MOVIE_SETTING_LAGHACK;

	if (gbNullInputHackTempEnabled)
		Movie.header.optionFlags |= MOVIE_SETTING_GBINPUTHACK;

	Movie.header.optionFlags |= MOVIE_SETTING_GBCFF55FIX;
	extern int32 gbDMASpeedVersion;
	gbDMASpeedVersion = 1;

	Movie.header.optionFlags |= MOVIE_SETTING_GBECHORAMFIX;
	extern int32 gbEchoRAMFixOn;
	gbEchoRAMFixOn = 1;

	// some GB/GBC games depend on the sound rate, so just use the highest one
	systemSoundSetQuality(1);

	useOldFrameTiming = false;

#if (defined(WIN32) && !defined(SDL))
//    theApp.removeIntros   = false;

	prevBorder	   = gbBorderOn;
	prevWinBorder  = theApp.winGbBorderOn;
	prevBorderAuto = gbBorderAutomatic;
	if (gbEmulatorType == 2 || gbEmulatorType == 5)     // only games played in SGB mode will have a border
	{
		gbBorderOn = true;
		theApp.winGbBorderOn = true;
		gbBorderAutomatic	 = false;
	}
	else
	{
		gbBorderOn = false;
		theApp.winGbBorderOn = false;
		gbBorderAutomatic	 = false;
	}
	systemGbBorderOn();
#else
	/// SDLFIXME
#endif
}

uint16 VBAMovieGetCurrentInputOf(int controllerNum, bool normalOnly)
{
	if (controllerNum < 0 || controllerNum >= MOVIE_NUM_OF_POSSIBLE_CONTROLLERS)
		return 0;

	return normalOnly ? (currentButtons[controllerNum] & BUTTON_REGULAR_MASK) : currentButtons[controllerNum];
}

int VBAMovieCreate(const char *filename, const char *authorInfo, uint8 startFlags, uint8 controllerFlags, uint8 typeFlags)
{
	// make sure at least one controller is enabled
	if ((controllerFlags & MOVIE_CONTROLLERS_ANY_MASK) == 0)
		return MOVIE_WRONG_FORMAT;

	if (!emulating)
		return MOVIE_UNKNOWN_ERROR;

	loadingMovie = true;

	FILE * file;
	STREAM stream;
	int	   fn;

	char movie_filename [_MAX_PATH];
#ifdef WIN32
	_fullpath(movie_filename, filename, _MAX_PATH);
#else
	// FIXME: convert to fullpath
	strncpy(movie_filename, filename, _MAX_PATH);
	movie_filename[_MAX_PATH - 1] = '\0';
#endif

	bool alreadyOpen = (Movie.file != NULL && stricmp(movie_filename, Movie.filename) == 0);

	if (alreadyOpen)
		change_state(MOVIE_STATE_NONE);  // have to stop current movie before trying to re-open it

	if (movie_filename[0] == '\0')
	{ loadingMovie = false; return MOVIE_FILE_NOT_FOUND; }

	if (!(file = fopen(movie_filename, "wb")))
	{ loadingMovie = false; return MOVIE_FILE_NOT_FOUND; }

	if (!alreadyOpen)
		change_state(MOVIE_STATE_NONE);  // stop current movie when we're able to open the other one

	// clear out the current movie
	VBAMovieInit();

	// fill in the movie's header
	Movie.header.uid   = (uint32)time(NULL);
	Movie.header.magic = VBM_MAGIC;
	Movie.header.version		 = VBM_VERSION;
	Movie.header.rerecord_count	 = 0;
	Movie.header.length_frames	 = 0;
	Movie.header.startFlags		 = startFlags;
	Movie.header.controllerFlags = controllerFlags;
	Movie.header.typeFlags		 = typeFlags;
	Movie.header.minorVersion	 = VBM_REVISION;

	// set emulator settings that make the movie more likely to stay synchronized when it's later played back
	SetRecordEmuSettings();

	// set ROM and BIOS checksums and stuff
	VBAMovieGetRomInfo(Movie, Movie.header.romTitle, Movie.header.romGameCode, Movie.header.romOrBiosChecksum, Movie.header.romCRC);

	// write the header to file
	write_movie_header(file, Movie);

	// copy over the metadata / author info
	VBAMovieSetMetadata(authorInfo);

	// write the metadata / author info to file
	fwrite(Movie.authorInfo, 1, sizeof(char) * MOVIE_METADATA_SIZE, file);

	// write snapshot or SRAM if applicable
	if (Movie.header.startFlags & MOVIE_START_FROM_SNAPSHOT
	    || Movie.header.startFlags & MOVIE_START_FROM_SRAM)
	{
		Movie.header.offset_to_savestate = (uint32)ftell(file);

		// close the file and reopen it as a stream:

		fn = dup(fileno(file));
		fclose(file);

		if (!(stream = utilGzReopen(fn, "ab"))) // append mode to start at end, no seek necessary
		{ loadingMovie = false; return MOVIE_FILE_NOT_FOUND; }

		// write the save data:
		if (Movie.header.startFlags & MOVIE_START_FROM_SNAPSHOT)
		{
			// save snapshot
			if (!theEmulator.emuWriteStateToStream(stream))
			{
				utilGzClose(stream);
				{ loadingMovie = false; return MOVIE_UNKNOWN_ERROR; }
			}
		}
		else if (Movie.header.startFlags & MOVIE_START_FROM_SRAM)
		{
			// save SRAM
			if (!theEmulator.emuWriteBatteryToStream(stream))
			{
				utilGzClose(stream);
				{ loadingMovie = false; return MOVIE_UNKNOWN_ERROR; }
			}

			// 'soft' reset:
			theEmulator.emuReset(false);
		}

		utilGzClose(stream);

		// reopen the file and seek back to the end

		if (!(file = fopen(movie_filename, "rb+")))
		{ loadingMovie = false; return MOVIE_FILE_NOT_FOUND; }

		fseek(file, 0, SEEK_END);
	}
	else // no snapshot or SRAM
	{
		HardResetAndSRAMClear();
	}

	Movie.header.offset_to_controller_data = (uint32)ftell(file);

	strcpy(Movie.filename, movie_filename);
	Movie.file = file;
	Movie.bytesPerFrame		  = bytes_per_frame(Movie);
	Movie.inputBufferPtr	  = Movie.inputBuffer;
	Movie.currentFrame		  = 0;
	Movie.readOnly			  = false;
	Movie.RecordedThisSession = true;

	change_state(MOVIE_STATE_RECORD);

	systemScreenMessage("Recording movie...");
	{ loadingMovie = false; return MOVIE_SUCCESS; }
}

void VBAUpdateButtonPressDisplay()
{
	uint32 keys = currentButtons[0] & BUTTON_REGULAR_RECORDING_MASK;

	const static char KeyMap[]	 =  { 'A', 'B', 's', 'S', '>', '<', '^', 'v', 'R', 'L', '!', '?', '{', '}', 'v', '^' };
	const static int  KeyOrder[] = { 5, 6, 4, 7, 0, 1, 9, 8, 3, 2, 12, 15, 13, 14, 11, 10 }; // < ^ > v   A B  L R  S s  { = } _
	                                                                                         // ? !
	char buffer[256];
	sprintf(buffer, "                    ");

#ifndef WIN32
	// don't bother color-coding autofire and such
	int i;
	for (i = 0; i < 15; i++)
	{
		int j	 = KeyOrder[i];
		int mask = (1 << (j));
		buffer[strlen("    ") + i] = ((keys & mask) != 0) ? KeyMap[j] : ' ';
	}

	systemScreenMessage(buffer, 2, -1);
#else
	const bool eraseAll		= !theApp.inputDisplay;
	uint32	   autoHeldKeys = eraseAll ? 0 : theApp.autoHold & BUTTON_REGULAR_RECORDING_MASK;
	uint32	   autoFireKeys = eraseAll ? 0 : (theApp.autoFire | theApp.autoFire2) & BUTTON_REGULAR_RECORDING_MASK;
	uint32	   pressedKeys	= eraseAll ? 0 : keys;

	char colorList[64];
	memset(colorList, 1, strlen(buffer));

	if (!eraseAll)
	{
		for (int i = 0; i < 15; i++)
		{
			const int  j		 = KeyOrder[i];
			const int  mask		 = (1 << (j));
			bool	   pressed	 = (pressedKeys  & mask) != 0;
			const bool autoHeld	 = (autoHeldKeys & mask) != 0;
			const bool autoFired = (autoFireKeys & mask) != 0;
			const bool erased	 = (lastKeys & mask) != 0 && (!pressed && !autoHeld && !autoFired);
			extern int textMethod;
			if (textMethod != 2 && (autoHeld || (autoFired && !pressed) || erased))
			{
				int colorNum = 1;     // default is white
				if (autoHeld)
					colorNum += (pressed ? 2 : 1);     // yellow if pressed, red if not
				else if (autoFired)
					colorNum += 5;     // blue if autofired and not currently pressed
				else if (erased)
					colorNum += 8;     // black on black

				colorList[strlen("    ") + i] = colorNum;
				pressed = true;
			}
			buffer[strlen("    ") + i] = pressed ? KeyMap[j] : ' ';
		}
	}

	lastKeys  = currentButtons[0];
	lastKeys |= theApp.autoHold & BUTTON_REGULAR_RECORDING_MASK;
	lastKeys |= (theApp.autoFire | theApp.autoFire2) & BUTTON_REGULAR_RECORDING_MASK;

	systemScreenMessage(buffer, 2, -1, colorList);
#endif
}

void VBAUpdateFrameCountDisplay()
{
	const int MAGICAL_NUMBER = 64;  // FIXME: this won't do any better, but only to remind you of sz issues
	char	  frameDisplayString[MAGICAL_NUMBER];
	char	  lagFrameDisplayString[MAGICAL_NUMBER];

#if (defined(WIN32) && !defined(SDL))
	if (theApp.frameCounter)
#else
	/// SDL FIXME
#endif
	{
		switch (Movie.state)
		{
		case MOVIE_STATE_PLAY:
		case MOVIE_STATE_END:
		{
			sprintf(frameDisplayString, "%d / %d", Movie.currentFrame, Movie.header.length_frames);
			if (!Movie.readOnly)
				strcat(frameDisplayString, " (edit)");
			break;
		}
		case MOVIE_STATE_RECORD:
		{
			sprintf(frameDisplayString, "%d (record)", Movie.currentFrame);
			break;
		}
		default:
		{
			sprintf(frameDisplayString, "%d (no movie)", systemCounters.frameCount);
			break;
		}
		}

#if (defined(WIN32) && !defined(SDL))
		if (theApp.lagCounter)
#else
		/// SDL FIXME
#endif
		{
//			sprintf(lagFrameDisplayString, " %c %d", systemCounters.laggedLast ? '*' : '|', systemCounters.lagCount);
			sprintf(lagFrameDisplayString, " | %d%s", systemCounters.lagCount, systemCounters.laggedLast ? " *" : "");
			strcat(frameDisplayString, lagFrameDisplayString);
		}
	}
#if (defined(WIN32) && !defined(SDL))
	else
	{
		frameDisplayString[0] = '\0';
	}
#else
	/// SDL FIXME
#endif
	systemScreenMessage(frameDisplayString, 1, -1);
}

// this function should only be called once every frame
void VBAMovieUpdateState()
{
	++Movie.currentFrame;

	if (Movie.state == MOVIE_STATE_PLAY)
	{
		Movie.inputBufferPtr += Movie.bytesPerFrame;
		if (Movie.currentFrame >= Movie.header.length_frames)
		{
			// the movie ends anyway; what to do next depends on the settings
			change_state(MOVIE_STATE_END);
		}
	}
	else if (Movie.state == MOVIE_STATE_RECORD)
	{
		// use first fseek?
		fwrite(Movie.inputBufferPtr, 1, Movie.bytesPerFrame, Movie.file);
		Movie.header.length_frames = Movie.currentFrame;
		Movie.inputBufferPtr	 += Movie.bytesPerFrame;
		Movie.RecordedThisSession = true;
		flush_movie_header();
	}
	else if (Movie.state == MOVIE_STATE_END)
	{
		change_state(MOVIE_STATE_END);
	}
}

void VBAMovieRead(int i, bool /*sensor*/)
{
	if (Movie.state != MOVIE_STATE_PLAY)
		return;

	if (i < 0 || i >= MOVIE_NUM_OF_POSSIBLE_CONTROLLERS)
		return;      // not a controller we're recognizing

	if (Movie.header.controllerFlags & MOVIE_CONTROLLER(i))
	{
		currentButtons[i] = Read16(Movie.inputBufferPtr + CONTROLLER_DATA_SIZE * i);
	}
	else
	{
		currentButtons[i] = 0;        // pretend the controller is disconnected
	}

	if ((currentButtons[i] & BUTTON_MASK_NEW_RESET) != 0)
		resetSignaled = true;
}

void VBAMovieWrite(int i, bool /*sensor*/)
{
	if (Movie.state != MOVIE_STATE_RECORD)
		return;

	if (i < 0 || i >= MOVIE_NUM_OF_POSSIBLE_CONTROLLERS)
		return;      // not a controller we're recognizing

	reserve_buffer_space((uint32)((Movie.inputBufferPtr - Movie.inputBuffer) + Movie.bytesPerFrame));

	if (Movie.header.controllerFlags & MOVIE_CONTROLLER(i))
	{
		// get the current controller data
		uint16 buttonData = currentButtons[i];

		// mask away the irrelevent bits
		buttonData &= BUTTON_REGULAR_MASK | BUTTON_MOTION_MASK;

		// soft-reset "button" for 1 frame if the game is reset while recording
		if (resetSignaled)
		{
			buttonData |= BUTTON_MASK_NEW_RESET;
		}

		// backward compatibility kludge
		if (resetSignaledLast)
		{
			buttonData |= BUTTON_MASK_OLD_RESET;
		}

		Write16(buttonData, Movie.inputBufferPtr + CONTROLLER_DATA_SIZE * i);

		// and for display
		currentButtons[i] = buttonData;
	}
	else
	{
		// pretend the controller is disconnected (otherwise input it gives could cause desync since we're not writing it to the
		// movie)
		currentButtons[i] = 0;
	}
}

void VBAMovieStop(bool8 suppress_message)
{
	if (Movie.state != MOVIE_STATE_NONE)
	{
		change_state(MOVIE_STATE_NONE);
		if (!suppress_message)
			systemScreenMessage("Movie stop");
	}
}

int VBAMovieGetInfo(const char *filename, SMovie *info)
{
	assert(info != NULL);
	if (info == NULL)
		return -1;

	FILE *	file;
	int		result;
	SMovie &local_movie = *info;

	memset(info, 0, sizeof(*info));
	if (filename[0] == '\0')
		return MOVIE_FILE_NOT_FOUND;
	if (!(file = fopen(filename, "rb")))
		return MOVIE_FILE_NOT_FOUND;

	// read header
	if ((result = (read_movie_header(file, local_movie))) != MOVIE_SUCCESS)
		return result;

	// read the metadata / author info from file
	fread(local_movie.authorInfo, 1, sizeof(char) * MOVIE_METADATA_SIZE, file);

	strncpy(local_movie.filename, filename, _MAX_PATH);
	local_movie.filename[_MAX_PATH - 1] = '\0';

	if (Movie.file != NULL && stricmp(local_movie.filename, Movie.filename) == 0) // alreadyOpen
	{
		local_movie.bytesPerFrame		 = Movie.bytesPerFrame;
		local_movie.header.length_frames = Movie.header.length_frames;
	}
	else
	{
		// recalculate length of movie from the file size
		local_movie.bytesPerFrame = bytes_per_frame(local_movie);
		fseek(file, 0, SEEK_END);
		int fileSize = ftell(file);
		local_movie.header.length_frames =
		    (fileSize - local_movie.header.offset_to_controller_data) / local_movie.bytesPerFrame;
	}

	fclose(file);

	if (access(filename, W_OK))
		info->readOnly = true;

	return MOVIE_SUCCESS;
}

bool8 VBAMovieActive()
{
	return (Movie.state != MOVIE_STATE_NONE);
}

bool8 VBAMovieLoading()
{
	return loadingMovie;
}

bool8 VBAMoviePlaying()
{
	return (Movie.state == MOVIE_STATE_PLAY);
}

bool8 VBAMovieRecording()
{
	return (Movie.state == MOVIE_STATE_RECORD);
}

bool8 VBAMovieReadOnly()
{
	if (!VBAMovieActive())
		return false;

	return Movie.readOnly;
}

void VBAMovieToggleReadOnly()
{
	if (!VBAMovieActive())
		return;

	if (Movie.readOnly != 2)
	{
		Movie.readOnly = !Movie.readOnly;

		systemScreenMessage(Movie.readOnly ? "Movie now read-only" : "Movie now editable");
	}
	else
	{
		systemScreenMessage("Can't toggle read-only movie");
	}
}

uint32 VBAMovieGetVersion()
{
	if (!VBAMovieActive())
		return 0;

	return Movie.header.version;
}

uint32 VBAMovieGetMinorVersion()
{
	if (!VBAMovieActive())
		return 0;

	return Movie.header.minorVersion;
}

uint32 VBAMovieGetId()
{
	if (!VBAMovieActive())
		return 0;

	return Movie.header.uid;
}

uint32 VBAMovieGetLength()
{
	if (!VBAMovieActive())
		return 0;

	return Movie.header.length_frames;
}

uint32 VBAMovieGetFrameCounter()
{
	if (!VBAMovieActive())
		return 0;

	return Movie.currentFrame;
}

uint32 VBAMovieGetRerecordCount()
{
	if (!VBAMovieActive())
		return 0;

	return Movie.header.rerecord_count;
}

uint32 VBAMovieSetRerecordCount(uint32 newRerecordCount)
{
	uint32 oldRerecordCount = 0;
	if (!VBAMovieActive())
		return 0;

	oldRerecordCount = Movie.header.rerecord_count;
	Movie.header.rerecord_count = newRerecordCount;
	return oldRerecordCount;
}

std::string VBAMovieGetAuthorInfo()
{
	if (!VBAMovieActive())
		return "";

	return Movie.authorInfo;
}

std::string VBAMovieGetFilename()
{
	if (!VBAMovieActive())
		return "";

	return Movie.filename;
}

void VBAMovieFreeze(uint8 * *buf, uint32 *size)
{
	// sanity check
	if (!VBAMovieActive())
	{
		return;
	}

	*buf  = NULL;
	*size = 0;

	// compute size needed for the buffer
	// room for header.uid, currentFrame, and header.length_frames
	uint32 size_needed = sizeof(Movie.header.uid) + sizeof(Movie.currentFrame) + sizeof(Movie.header.length_frames);
	size_needed += (uint32)(Movie.bytesPerFrame * Movie.header.length_frames);
	*buf		 = new uint8[size_needed];
	*size		 = size_needed;

	uint8 *ptr = *buf;
	if (!ptr)
	{
		return;
	}

	Push32(Movie.header.uid, ptr);
	Push32(Movie.currentFrame, ptr);
	Push32(Movie.header.length_frames - 1, ptr);   // HACK: shorten the length by 1 for backward compatibility

	memcpy(ptr, Movie.inputBuffer, Movie.bytesPerFrame * Movie.header.length_frames);
}

int VBAMovieUnfreeze(const uint8 *buf, uint32 size)
{
	// sanity check
	if (!VBAMovieActive())
	{
		return MOVIE_NOT_FROM_A_MOVIE;
	}

	const uint8 *ptr = buf;
	if (size < sizeof(Movie.header.uid) + sizeof(Movie.currentFrame) + sizeof(Movie.header.length_frames))
	{
		return MOVIE_WRONG_FORMAT;
	}

	uint32 movie_id		 = Pop32(ptr);
	uint32 current_frame = Pop32(ptr);
	uint32 end_frame	 = Pop32(ptr) + 1;     // HACK: restore the length for backward compatibility
	uint32 space_needed	 = Movie.bytesPerFrame * end_frame;

	if (movie_id != Movie.header.uid)
		return MOVIE_NOT_FROM_THIS_MOVIE;

	if (space_needed > size)
		return MOVIE_WRONG_FORMAT;

	if (Movie.readOnly)
	{
		// here, we are going to keep the input data from the movie file
		// and simply rewind to the currentFrame pointer
		// this will cause a desync if the savestate is not in sync // <-- NOT ANYMORE
		// with the on-disk recording data, but it's easily solved
		// by loading another savestate or playing the movie from the beginning

		// don't allow loading a state inconsistent with the current movie
		uint32 length_history = min(current_frame, Movie.header.length_frames);
		if (end_frame < length_history)
			return MOVIE_SNAPSHOT_INCONSISTENT;

		uint32 space_shared = Movie.bytesPerFrame * length_history;
		if (memcmp(Movie.inputBuffer, ptr, space_shared))
			return MOVIE_SNAPSHOT_INCONSISTENT;

		Movie.currentFrame	 = current_frame;
		Movie.inputBufferPtr = Movie.inputBuffer + Movie.bytesPerFrame * min(current_frame, Movie.header.length_frames);

		change_state(MOVIE_STATE_PLAY);
	}
	else
	{
		// here, we are going to take the input data from the savestate
		// and make it the input data for the current movie, then continue
		// writing new input data at the currentFrame pointer
		Movie.currentFrame		   = current_frame;
		Movie.header.length_frames = end_frame;
		if (!VBALuaRerecordCountSkip())
			++Movie.header.rerecord_count;

		Movie.RecordedThisSession = true;

		// do this before calling reserve_buffer_space()
		Movie.inputBufferPtr = Movie.inputBuffer + Movie.bytesPerFrame * min(current_frame, Movie.header.length_frames);
		reserve_buffer_space(space_needed);
		memcpy(Movie.inputBuffer, ptr, space_needed);

		// for consistency, no auto movie conversion here since we don't auto convert the corresponding savestate
		flush_movie_header();
		flush_movie_frames();

		change_state(MOVIE_STATE_RECORD);
	}

	// necessary!
	resetSignaled	  = false;
	resetSignaledLast = false;

	// necessary to check if there's a reset signal at the previous frame
	if (current_frame > 0)
	{
		const u8 NEW_RESET = u8(BUTTON_MASK_NEW_RESET >> 8);
		for (int i = 0; i < MOVIE_NUM_OF_POSSIBLE_CONTROLLERS; ++i)
		{
			if ((Movie.header.controllerFlags & MOVIE_CONTROLLER(i)) && (Movie.inputBufferPtr[1 - Movie.bytesPerFrame] & NEW_RESET))
			{
				resetSignaledLast = true;
				break;
			}
		}
	}

	return MOVIE_SUCCESS;
}

bool VBAMovieEnded()
{
	return (Movie.state == MOVIE_STATE_END);
//	return (Movie.state != MOVIE_STATE_NONE && Movie.currentFrame >= Movie.header.length_frames);
}

bool VBAMovieAllowsRerecording()
{
	bool allows = (Movie.state != MOVIE_STATE_NONE) && (Movie.currentFrame <= Movie.header.length_frames);
	return /*!VBAMovieReadOnly() &&*/ allows;
}

bool VBAMovieSwitchToPlaying()
{
	if (!VBAMovieActive())
		return false;

	if (!Movie.readOnly)
	{
		VBAMovieToggleReadOnly();
	}

	change_state(MOVIE_STATE_PLAY);
	if (Movie.state == MOVIE_STATE_PLAY)
		systemScreenMessage("Movie replay (continue)");
	else
		systemScreenMessage("Movie end");

	return true;
}

bool VBAMovieSwitchToRecording()
{
	if (!VBAMovieAllowsRerecording())
		return false;

	if (Movie.readOnly)
	{
		VBAMovieToggleReadOnly();
	}

	if (!VBALuaRerecordCountSkip())
		++Movie.header.rerecord_count;

	change_state(MOVIE_STATE_RECORD);
	systemScreenMessage("Movie re-record");

	truncate_movie(Movie.currentFrame);

	return true;
}

uint32 VBAMovieGetState()
{
	// ?
	if (!VBAMovieActive())
		return MOVIE_STATE_NONE;

	return Movie.state;
}

void VBAMovieSignalReset()
{
	if (VBAMovieActive())
		resetSignaled = true;
}

void VBAMovieResetIfRequested()
{
	if (resetSignaled)
	{
		theEmulator.emuReset(false);
		resetSignaled	  = false;
		resetSignaledLast = true;
	}
	else
	{
		resetSignaledLast = false;
	}
}

void VBAMovieSetMetadata(const char *info)
{
	if (!memcmp(Movie.authorInfo, info, MOVIE_METADATA_SIZE))
		return;

	memcpy(Movie.authorInfo, info, MOVIE_METADATA_SIZE); // strncpy would omit post-0 bytes
	Movie.authorInfo[MOVIE_METADATA_SIZE - 1] = '\0';

	if (Movie.file)
	{
		// (over-)write the header
		fseek(Movie.file, 0, SEEK_SET);
		write_movie_header(Movie.file, Movie);

		// write the metadata / author info to file
		fwrite(Movie.authorInfo, 1, sizeof(char) * MOVIE_METADATA_SIZE, Movie.file);

		fflush(Movie.file);
	}
}

void VBAMovieRestart()
{
	if (VBAMovieActive())
	{
		systemSoundClearBuffer();

		bool8 modified = Movie.RecordedThisSession;

		VBAMovieStop(true);

		char movieName [_MAX_PATH];
		strncpy(movieName, Movie.filename, _MAX_PATH);
		movieName[_MAX_PATH - 1] = '\0';
		VBAMovieOpen(movieName, Movie.readOnly); // can't just pass in Movie.filename, since VBAMovieOpen clears out Movie's
		                                         // variables

		Movie.RecordedThisSession = modified;

		systemScreenMessage("Movie replay (restart)");
	}
}

void VBAMovieSetPauseAt(int at)
{
	Movie.pauseFrame = at;
}

///////////////////////
// movie tools

// FIXME: is it safe to convert/flush a movie while recording it (considering fseek() problem)?
int VBAMovieConvertCurrent()
{
	if (!VBAMovieActive())
	{
		return MOVIE_NOTHING;
	}

	if (Movie.header.minorVersion > VBM_REVISION)
	{
		return MOVIE_WRONG_VERSION;
	}

	if (Movie.header.minorVersion == VBM_REVISION)
	{
		return MOVIE_NOTHING;
	}

	Movie.header.minorVersion = VBM_REVISION;

	if (Movie.header.length_frames == 0) // this could happen
	{
		truncate_movie(0);
		return MOVIE_SUCCESS;
	}

	// fix movies recorded from snapshots
	if (Movie.header.startFlags & MOVIE_START_FROM_SNAPSHOT)
	{
		uint8 *firstFramePtr = Movie.inputBuffer;
		for (int i = 0; i < MOVIE_NUM_OF_POSSIBLE_CONTROLLERS; ++i)
		{
			if (Movie.header.controllerFlags & MOVIE_CONTROLLER(i))
			{
				Push16(initialInputs[i], firstFramePtr);
				// note: this is correct since Push16 advances the dest pointer by sizeof u16
			}
		}
	}

	// convert old resets to new ones
	const u8 OLD_RESET = u8(BUTTON_MASK_OLD_RESET >> 8);
	const u8 NEW_RESET = u8(BUTTON_MASK_NEW_RESET >> 8);
	for (int i = 0; i < MOVIE_NUM_OF_POSSIBLE_CONTROLLERS; ++i)
	{
		if (Movie.header.controllerFlags & MOVIE_CONTROLLER(i))
		{
			uint8 *startPtr = Movie.inputBuffer + sizeof(u16) * i + 1;
			uint8 *endPtr	= Movie.inputBuffer + Movie.bytesPerFrame * (Movie.header.length_frames - 1);
			for (; startPtr < endPtr; startPtr += Movie.bytesPerFrame)
			{
				if (startPtr[Movie.bytesPerFrame] & OLD_RESET)
				{
					startPtr[0] |= NEW_RESET;
				}
			}
		}
	}

	flush_movie_header();
	flush_movie_frames();
	return MOVIE_SUCCESS;
}

void VBAMovieExtractFromSnapshot()
{
	// Currently, snapshots taken from a movie don't contain the initial SRAM or savestate of the movie,
	// even if the movie was recorded from either of them. If a snapshot was taken at the first frame
	// i.e. Frame 0, it can be safely assumed that the snapshot reflects the initial state of such a movie.
	// However, if it was taken after the first frame, the SRAM contained might either be still the same
	// as the original (usually true if no write operations on the SRAM occured) or have been modified,
	// while the exact original state could hardly, if not impossibly, be safely worked out.

	// TODO
	;
}

