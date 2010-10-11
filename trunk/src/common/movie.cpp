#include "../Port.h"
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <cassert>

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

#if (defined(WIN32) && !defined(SDL))
#   include "../win32/stdafx.h"
#   include "../win32/MainWnd.h"
#   include "../win32/VBA.h"
#   include "../win32/WinMiscUtil.h"
#endif

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

extern int emulating; // from VBA.cpp

SMovie Movie;
bool   loadingMovie		   = false;
bool8  loadedMovieSnapshot = 0;

#if (defined(WIN32) && !defined(SDL))
extern u32 currentButtons [4];     // from System.cpp
#else
u32 currentButtons [4];
#endif
static bool resetSignaled = false;

static int controllersLeftThisFrame = 0;
static int prevBorder, prevWinBorder, prevBorderAuto;

static int bytes_per_frame(SMovie & mov)
{
	int num_controllers = 0;

	for (int i = 0; i < MOVIE_NUM_OF_POSSIBLE_CONTROLLERS; i++)
		if (mov.header.controllerFlags & MOVIE_CONTROLLER(i))
			num_controllers++;

	return CONTROLLER_DATA_SIZE * num_controllers;
}

// little-endian integer read/write functions:
static inline uint32 Read32(const uint8 *& ptr)
{
	uint32 v = (ptr[0] | (ptr[1]<<8) | (ptr[2]<<16) | (ptr[3]<<24));
	ptr += 4;
	return v;
}

static inline uint16 Read16(const uint8 *& ptr) /* const version */
{
	uint16 v = (ptr[0] | (ptr[1]<<8));
	ptr += 2;
	return v;
}

// WHY?
static inline uint16 Read16(uint8 *& ptr) /* non-const version */
{
	uint16 v = (ptr[0] | (ptr[1]<<8));
	ptr += 2;
	return v;
}

#define Read8(ptr) (*(ptr)++)
static void Write32(uint32 v, uint8 *& ptr)
{
	ptr[0] = (uint8)(v&0xff);
	ptr[1] = (uint8)((v>>8)&0xff);
	ptr[2] = (uint8)((v>>16)&0xff);
	ptr[3] = (uint8)((v>>24)&0xff);
	ptr	  += 4;
}

static void Write16(uint16 v, uint8 *& ptr)
{
	ptr[0] = (uint8)(v&0xff);
	ptr[1] = (uint8)((v>>8)&0xff);
	ptr	  += 2;
}

#define Write8(v, ptr) (*ptr++ = v)

static int read_movie_header(FILE *file, SMovie &movie)
{
    assert(file != NULL);
    assert(VBM_HEADER_SIZE == sizeof(SMovieFileHeader)); // sanity check on the header type definition

    uint8 headerData [VBM_HEADER_SIZE];

    if (fread(headerData, 1, VBM_HEADER_SIZE, file) != VBM_HEADER_SIZE)
		return WRONG_FORMAT;  // if we failed to read in all VBM_HEADER_SIZE bytes of the header

    const uint8		 *ptr	 = headerData;
    SMovieFileHeader &header = movie.header;

    header.magic = Read32(ptr);
    if (header.magic != VBM_MAGIC)
		return WRONG_FORMAT;

    header.version = Read32(ptr);
    if (header.version != VBM_VERSION)
		return WRONG_VERSION;

    header.uid = Read32(ptr);
    header.length_frames  = Read32(ptr);
    header.rerecord_count = Read32(ptr);

    header.startFlags	   = Read8(ptr);
    header.controllerFlags = Read8(ptr);
    header.typeFlags	   = Read8(ptr);
    header.optionFlags	   = Read8(ptr);

    header.saveType		  = Read32(ptr);
    header.flashSize	  = Read32(ptr);
    header.gbEmulatorType = Read32(ptr);

    for (int i = 0; i < 12; i++)
		header.romTitle[i] = Read8(ptr);

    header.reservedByte = Read8(ptr);

    header.romCRC = Read8(ptr);
    header.romOrBiosChecksum = Read16(ptr);
    header.romGameCode		 = Read32(ptr);

    header.offset_to_savestate		 = Read32(ptr);
    header.offset_to_controller_data = Read32(ptr);

    return SUCCESS;
}

static void write_movie_header(FILE *file, const SMovie & movie)
{
    assert(ftell(file) == 0); // we assume file points to beginning of movie file

    uint8 headerData [VBM_HEADER_SIZE];
    uint8*ptr = headerData;
    const SMovieFileHeader & header = movie.header;

    Write32(header.magic, ptr);
    Write32(header.version, ptr);

    Write32(header.uid, ptr);
    Write32(header.length_frames, ptr);
    Write32(header.rerecord_count, ptr);

    Write8(header.startFlags, ptr);
    Write8(header.controllerFlags, ptr);
    Write8(header.typeFlags, ptr);
    Write8(header.optionFlags, ptr);

    Write32(header.saveType, ptr);
    Write32(header.flashSize, ptr);
    Write32(header.gbEmulatorType, ptr);

    for (int i = 0; i < 12; i++)
		Write8(header.romTitle[i], ptr);

    Write8(header.reservedByte, ptr);

    Write8(header.romCRC, ptr);
    Write16(header.romOrBiosChecksum, ptr);
    Write32(header.romGameCode, ptr);

    Write32(header.offset_to_savestate, ptr);
    Write32(header.offset_to_controller_data, ptr);

    fwrite(headerData, 1, VBM_HEADER_SIZE, file);
}

static void flush_movie()
{
    if (!Movie.file)
		return;

    // (over-)write the header
    fseek(Movie.file, 0, SEEK_SET);
    write_movie_header(Movie.file, Movie);

    // (over-)write the controller data
    fseek(Movie.file, Movie.header.offset_to_controller_data, SEEK_SET);
    fwrite(Movie.inputBuffer, 1, Movie.bytesPerFrame * (Movie.header.length_frames + 1), Movie.file);	// +1 dummy

    fflush(Movie.file);
}

static void truncateMovie()
{
    // truncate movie to header.length_frames + 1 length
    // NOTE: it's certain that the savestate block is never after the
    //       controller data block, because the VBM format decrees it.

    if (!Movie.file)
		return;

    assert(Movie.header.offset_to_savestate <= Movie.header.offset_to_controller_data);
    if (Movie.header.offset_to_savestate > Movie.header.offset_to_controller_data)
		return;

    const u32 truncLen = Movie.header.offset_to_controller_data + Movie.bytesPerFrame * (Movie.header.length_frames + 1);	// +1 dummy
    ftruncate(fileno(Movie.file), truncLen);
}

static void change_state(MovieState new_state)
{
    if (new_state == Movie.state)
		return;

    if (Movie.state == MOVIE_STATE_RECORD)
    {
        flush_movie();
	}

#if (defined(WIN32) && !defined(SDL))
    theApp.frameSearching	   = false;
    theApp.frameSearchSkipping = false;
#endif

    if (new_state == MOVIE_STATE_NONE)
    {
        truncateMovie();

        fclose(Movie.file);
        Movie.file		   = NULL;
        Movie.currentFrame = 0;

#if (defined(WIN32) && !defined(SDL))
        // undo changes to border settings
        {
            theApp.winGbBorderOn = prevWinBorder;
            gbBorderAutomatic	 = prevBorderAuto;
		}
#endif
        extern int32 gbDMASpeedVersion;
        gbDMASpeedVersion = 1;

        extern int32 gbEchoRAMFixOn;
        gbEchoRAMFixOn = 1;

        if (Movie.inputBuffer)
        {
            free(Movie.inputBuffer);
            Movie.inputBuffer = NULL;
		}
	}

    Movie.state = new_state;
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
		for (int i = 0; i < Movie.bytesPerFrame; ++i)
		{
			Movie.inputBuffer[i] = 0;	// clear the dummy frame
		}
	}
}

static void read_frame_controller_data(int i)
{
    if (i < 0 || i >= MOVIE_NUM_OF_POSSIBLE_CONTROLLERS)
    {
        assert(0);
        return;
	}

    // the number of controllers an SGB game checks per frame is not constant throughout the entire game
    // so fill in the gaps with blank data when we hit a duplicate check when other controllers remain unchecked
    if ((controllersLeftThisFrame & MOVIE_CONTROLLER(i)) == 0)
    {
        if (controllersLeftThisFrame)
        {
            // already requested, fill in others first
            for (int controller = 0; controller < MOVIE_NUM_OF_POSSIBLE_CONTROLLERS; controller++)
				if ((controllersLeftThisFrame & MOVIE_CONTROLLER(controller)) != 0)
					read_frame_controller_data(controller);
		}
	}
    else
		controllersLeftThisFrame ^= MOVIE_CONTROLLER(i);

    if (!controllersLeftThisFrame)
		controllersLeftThisFrame = Movie.header.controllerFlags;

    if (Movie.header.controllerFlags & MOVIE_CONTROLLER(i))
    {
        currentButtons[i] = Read16(Movie.inputBufferPtr);
	}
    else
    {
        currentButtons[i] = 0;        // pretend the controller is disconnected
	}

    if ((currentButtons[i] & BUTTON_MASK_RESET) != 0)
		resetSignaled = true;
}

static void write_frame_controller_data(int i)
{
    if (i < 0 || i >= MOVIE_NUM_OF_POSSIBLE_CONTROLLERS)
    {
        assert(0);
        return;
	}

    // the number of controllers an SGB game checks per frame is not constant throughout the entire game
    // so fill in the gaps with blank data when we hit a duplicate check when other controllers remain unchecked
    if ((controllersLeftThisFrame & MOVIE_CONTROLLER(i)) == 0)
    {
        if (controllersLeftThisFrame)
        {
            // already requested, fill in others first
            for (int controller = 0; controller < MOVIE_NUM_OF_POSSIBLE_CONTROLLERS; controller++)
				if ((controllersLeftThisFrame & MOVIE_CONTROLLER(controller)) != 0)
					write_frame_controller_data(controller);
		}
	}
    else
		controllersLeftThisFrame ^= MOVIE_CONTROLLER(i);

    if (!controllersLeftThisFrame)
		controllersLeftThisFrame = Movie.header.controllerFlags;

    if (i == 0)
    {
        reserve_buffer_space((uint32)((Movie.inputBufferPtr + Movie.bytesPerFrame) - Movie.inputBuffer));
	}

    if (Movie.header.controllerFlags & MOVIE_CONTROLLER(i))
    {
        // get the current controller data
        uint16 buttonData = (uint16)currentButtons[i];

        // mask away the irrelevent bits
        buttonData &= BUTTON_REGULAR_MASK;

#       if (defined(WIN32) && !defined(SDL))
        // add in the motion sensor bits
        extern BOOL	  checkKey(LONG_PTR key);   // from Input.cpp
        extern USHORT motion[4];     // from DirectInput.cpp
        if (checkKey(motion[KEY_LEFT]))
			buttonData |= BUTTON_MASK_LEFT_MOTION;
        if (checkKey(motion[KEY_RIGHT]))
			buttonData |= BUTTON_MASK_RIGHT_MOTION;
        if (checkKey(motion[KEY_DOWN]))
			buttonData |= BUTTON_MASK_DOWN_MOTION;
        if (checkKey(motion[KEY_UP]))
			buttonData |= BUTTON_MASK_UP_MOTION;
#       else
#           ifdef SDL
        extern bool sdlCheckJoyKey(int key);         // from SDL.cpp
        extern u16	motion[4];        // from SDL.cpp
        if (sdlCheckJoyKey(motion[KEY_LEFT]))
			buttonData |= BUTTON_MASK_LEFT_MOTION;
        if (sdlCheckJoyKey(motion[KEY_RIGHT]))
			buttonData |= BUTTON_MASK_RIGHT_MOTION;
        if (sdlCheckJoyKey(motion[KEY_DOWN]))
			buttonData |= BUTTON_MASK_DOWN_MOTION;
        if (sdlCheckJoyKey(motion[KEY_UP]))
			buttonData |= BUTTON_MASK_UP_MOTION;
#           endif
#       endif

        // soft-reset "button" for 1 frame if the game is reset while recording
        if (resetSignaled)
			buttonData |= BUTTON_MASK_RESET;

        // write it to file
        Write16(buttonData, Movie.inputBufferPtr);

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

void VBAMovieInit()
{
    memset(&Movie, 0, sizeof(Movie));
    Movie.state		 = MOVIE_STATE_NONE;
    Movie.pauseFrame = -1;
    for (int i = 0; i < MOVIE_NUM_OF_POSSIBLE_CONTROLLERS; i++)
		currentButtons[i] = 0;
}

#if (defined(WIN32) && !defined(SDL))

static void GetBatterySaveName(CString & filename)
{
    filename = winGetDestFilename(theApp.filename, IDS_BATTERY_DIR, ".sav");
}

#else
#   ifdef SDL
static void GetBatterySaveName(char *buffer)
{
    extern char batteryDir[2048], filename[2048];     // from SDL.cpp
    extern char *sdlGetFilename(char *name);     // from SDL.cpp
    if (batteryDir[0])
		sprintf(buffer, "%s/%s.sav", batteryDir, sdlGetFilename(filename));
    else
		sprintf(buffer, "%s.sav", filename);
}

#   endif
#endif

static void SetPlayEmuSettings()
{
    gbEmulatorType = Movie.header.gbEmulatorType;
    extern void SetPrefetchHack(bool);
#   if (defined(WIN32) && !defined(SDL))
    if (theApp.cartridgeType == 0)    // lag disablement applies only to GBA
		SetPrefetchHack((Movie.header.optionFlags & MOVIE_SETTING_LAGHACK) != 0);

    // some GB/GBC games depend on the sound rate, so just use the highest one
    systemSetSoundQuality(1);

    theApp.useOldGBTiming = false;
//    theApp.removeIntros   = false;
    theApp.skipBiosFile = (Movie.header.optionFlags & MOVIE_SETTING_SKIPBIOSFILE) != 0;
    theApp.useBiosFile	= (Movie.header.optionFlags & MOVIE_SETTING_USEBIOSFILE) != 0;
    rtcEnable((Movie.header.optionFlags & MOVIE_SETTING_RTCENABLE) != 0);
    theApp.winSaveType	= Movie.header.saveType;
    theApp.winFlashSize = Movie.header.flashSize;

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

    prevBorder	   = gbBorderOn;
    prevWinBorder  = theApp.winGbBorderOn;
    prevBorderAuto = gbBorderAutomatic;
    if ((gbEmulatorType == 2 || gbEmulatorType == 5)
        && !theApp.hideMovieBorder) // games played in SGB mode can have a border
    {
        gbBorderOn = true;
        theApp.winGbBorderOn = true;
        gbBorderAutomatic	 = false;
        theApp.updateWindowSize(theApp.videoOption);
	}
    else
    {
        gbBorderOn = false;
        theApp.winGbBorderOn = false;
        gbBorderAutomatic	 = false;
        theApp.updateWindowSize(theApp.videoOption);
        if (theApp.hideMovieBorder)
        {
            theApp.hideMovieBorder = false;
            prevBorder = false;     // it might be expected behaviour that it stays hidden after the movie
		}
	}

#   else
    extern int	 saveType, sdlRtcEnable, sdlFlashSize;   // from SDL.cpp
    extern bool8 useBios, skipBios, removeIntros;     // from SDL.cpp
    useBios		 = (Movie.header.optionFlags & MOVIE_SETTING_USEBIOSFILE) != 0;
    skipBios	 = (Movie.header.optionFlags & MOVIE_SETTING_SKIPBIOSFILE) != 0;
    removeIntros = false /*(Movie.header.optionFlags & MOVIE_SETTING_REMOVEINTROS) != 0*/;
    sdlRtcEnable = (Movie.header.optionFlags & MOVIE_SETTING_RTCENABLE) != 0;
    saveType	 = Movie.header.saveType;
    sdlFlashSize = Movie.header.flashSize;

    extern int cartridgeType;     // from SDL.cpp
    if (cartridgeType == 0)    // lag disablement applies only to GBA
		SetPrefetchHack((Movie.header.optionFlags & MOVIE_SETTING_LAGHACK) != 0);
#   endif
}

static void HardResetAndSRAMClear()
{
#   if (defined(WIN32) && !defined(SDL))
    CString filename;
    GetBatterySaveName(filename);
    remove(filename);     // delete the damn SRAM file
    extern bool noWriteNextBatteryFile; noWriteNextBatteryFile = true;     // keep it from being resurrected from RAM
    ((MainWnd *)theApp.m_pMainWnd)->FileRun();     // start running the game
#   else
    char fname [1024];
    GetBatterySaveName(fname);
    remove(fname);     // delete the damn SRAM file

    // Henceforth, emuCleanUp means "clear out SRAM"
    theEmulator.emuCleanUp();     // keep it from being resurrected from RAM

    /// FIXME the correct SDL code to call for a full restart isn't in a function yet
    theEmulator.emuReset(false);
#   endif
}

int VBAMovieOpen(const char *filename, bool8 read_only)
{
    loadingMovie = true;
    uint8 movieReadOnly = read_only ? 1 : 0;

    FILE*  file;
    STREAM stream;
    int	   result;
    int	   fn;

    char movie_filename [_MAX_PATH];
#ifdef WIN32
    _fullpath(movie_filename, filename, _MAX_PATH);
#else
    // FIXME: convert to fullpath
    strncpy(movie_filename, filename, _MAX_PATH);
    movie_filename[_MAX_PATH-1] = '\0';
#endif

    if (movie_filename[0] == '\0')
    {loadingMovie = false; return FILE_NOT_FOUND; }

    if (!emulating)
    {loadingMovie = false; return UNKNOWN_ERROR; }

//	bool alreadyOpen = (Movie.file != NULL && _stricmp(movie_filename, Movie.filename) == 0);

//	if (alreadyOpen)
    change_state(MOVIE_STATE_NONE);     // have to stop current movie before trying to re-open it

    if (!(file = fopen(movie_filename, "rb+")))
		if (!(file = fopen(movie_filename, "rb")))
		{loadingMovie = false; return FILE_NOT_FOUND; }
    //else
    //	movieReadOnly = 2; // we have to open the movie twice, no need to do this both times

//	if (!alreadyOpen)
//		change_state(MOVIE_STATE_NONE); // stop current movie when we're able to open the other one
//
//	if (!(file = fopen(movie_filename, "rb+")))
//		if(!(file = fopen(movie_filename, "rb")))
//			{loadingMovie = false; return FILE_NOT_FOUND;}
//		else
//			movieReadOnly = 2;

    // clear out the current movie
    VBAMovieInit();

    // read header
    if ((result = read_movie_header(file, Movie)) != SUCCESS)
    {
        fclose(file);
        {loadingMovie = false; return result; }
	}

    // set emulator settings that make the movie more likely to stay synchronized
    SetPlayEmuSettings();

    // read the metadata / author info from file
    fread(Movie.authorInfo, 1, sizeof(char) * MOVIE_METADATA_SIZE, file);
    fn = dup(fileno(file)); // XXX: why does this fail?? it returns -1 but errno == 0
    fclose(file);

    // apparently this lseek is necessary
    lseek(fn, Movie.header.offset_to_savestate, SEEK_SET);
    if (!(stream = utilGzReopen(fn, "rb")))
		if (!(stream = utilGzOpen(movie_filename, "rb")))
		{loadingMovie = false; return FILE_NOT_FOUND; }
		else
			fn = dup(fileno(file));
    // in case the above dup failed but opening the file normally doesn't fail

    if (Movie.header.startFlags & MOVIE_START_FROM_SNAPSHOT)
    {
        // load the snapshot
        result = theEmulator.emuReadStateFromStream(stream) ? SUCCESS : WRONG_FORMAT;
	}
    else if (Movie.header.startFlags & MOVIE_START_FROM_SRAM)
    {
        // 'soft' reset:
        theEmulator.emuReset(false);

        // load the SRAM
        result = theEmulator.emuReadBatteryFromStream(stream) ? SUCCESS : WRONG_FORMAT;
	}
    else
    {
        HardResetAndSRAMClear();
	}

    resetSignaled = false;
    controllersLeftThisFrame = Movie.header.controllerFlags;

    utilGzClose(stream);

    if (result != SUCCESS)
    {loadingMovie = false; return result; }

//	if (!(file = fopen(movie_filename, /*read_only ? "rb" :*/ "rb+"))) // want to be able to switch out of read-only later
//	{
//		if(!Movie.readOnly || !(file = fopen(movie_filename, "rb"))) // try read-only if failed
//			return FILE_NOT_FOUND;
//	}
    if (!(file = fopen(movie_filename, "rb+")))
		if (!(file = fopen(movie_filename, "rb")))
		{loadingMovie = false; return FILE_NOT_FOUND; }
		else
			movieReadOnly = 2;

    // recalculate length of movie from the file size
    Movie.bytesPerFrame = bytes_per_frame(Movie);
    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    Movie.header.length_frames = (fileSize - Movie.header.offset_to_controller_data) / Movie.bytesPerFrame - 1;	// -1 dummy frame

    if (fseek(file, Movie.header.offset_to_controller_data, SEEK_SET))
    {loadingMovie = false; return WRONG_FORMAT; }

    // read controller data
	uint32 to_read = Movie.bytesPerFrame * (Movie.header.length_frames + 1);	// +1 dummy
    reserve_buffer_space(to_read);
    fread(Movie.inputBuffer, 1, to_read, file);


    strcpy(Movie.filename, movie_filename);
    Movie.file = file;
    Movie.inputBufferPtr = Movie.inputBuffer + Movie.bytesPerFrame;	// skipping 1 dummy frame
    Movie.currentFrame = 0;
    Movie.readOnly = movieReadOnly;

	change_state(MOVIE_STATE_PLAY);

    Movie.RecordedThisSession = false;

    if (Movie.readOnly)
		systemScreenMessage("Movie replay (read)");
    else
		systemScreenMessage("Movie replay (edit)");

    {loadingMovie = false; return SUCCESS; }
}

static void CalcROMInfo()
{
#if (defined(WIN32) && !defined(SDL))
    if (theApp.cartridgeType == 0) // GBA
#else
    extern int cartridgeType; // from SDL.cpp
    if (cartridgeType == 0) // GBA
#endif
    {
        extern u8 *bios, *rom;
        memcpy(Movie.header.romTitle, (const char *)&rom[0xa0], 12); // GBA TITLE
        memcpy(&Movie.header.romGameCode, &rom[0xac], 4); // GBA ROM GAME CODE

        extern u16 checksumBIOS();
        Movie.header.romOrBiosChecksum = checksumBIOS(); // GBA BIOS CHECKSUM
        Movie.header.romCRC = rom[0xbd]; // GBA ROM CRC
	}
    else // non-GBA
    {
        extern u8 *gbRom;
        memcpy(Movie.header.romTitle, (const char *)&gbRom[0x134], 12); // GB TITLE (note this can be 15 but is truncated to 12)
        Movie.header.romGameCode = (uint32)gbRom[0x146]; // GB ROM UNIT CODE

        Movie.header.romOrBiosChecksum = (gbRom[0x14e]<<8)|gbRom[0x14f]; // GB ROM CHECKSUM
        Movie.header.romCRC = gbRom[0x14d]; // GB ROM CRC
	}
}

static void SetRecordEmuSettings()
{
    Movie.header.optionFlags = 0;
#   if (defined(WIN32) && !defined(SDL))
    if (theApp.useBiosFile)
		Movie.header.optionFlags |= MOVIE_SETTING_USEBIOSFILE;
    if (theApp.skipBiosFile)
		Movie.header.optionFlags |= MOVIE_SETTING_SKIPBIOSFILE;
    if (rtcIsEnabled())
		Movie.header.optionFlags |= MOVIE_SETTING_RTCENABLE;
    Movie.header.saveType  = theApp.winSaveType;
    Movie.header.flashSize = theApp.winFlashSize;
#   else
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
#   endif
/* // This piece caused THE mysterious desync in GBA movie recording
   #if (defined(WIN32) && !defined(SDL))
        if(GetAsyncKeyState(VK_CONTROL) == 0)
   #endif
 */
    if (!memLagTempEnabled)
		Movie.header.optionFlags |= MOVIE_SETTING_LAGHACK;
    Movie.header.gbEmulatorType = gbEmulatorType;

    Movie.header.optionFlags |= MOVIE_SETTING_GBCFF55FIX;
    extern int32 gbDMASpeedVersion;
    gbDMASpeedVersion = 1;

    Movie.header.optionFlags |= MOVIE_SETTING_GBECHORAMFIX;
    extern int32 gbEchoRAMFixOn;
    gbEchoRAMFixOn = 1;

#   if (defined(WIN32) && !defined(SDL))
    // some GB/GBC games depend on the sound rate, so just use the highest one
    systemSetSoundQuality(1);

    theApp.useOldGBTiming = false;
//    theApp.removeIntros   = false;

    prevBorder	   = gbBorderOn;
    prevWinBorder  = theApp.winGbBorderOn;
    prevBorderAuto = gbBorderAutomatic;
    if (gbEmulatorType == 2 || gbEmulatorType == 5)     // only games played in SGB mode will have a border
    {
        gbBorderOn = true;
        theApp.winGbBorderOn = true;
        gbBorderAutomatic	 = false;
        theApp.updateWindowSize(theApp.videoOption);
	}
    else
    {
        gbBorderOn = false;
        theApp.winGbBorderOn = false;
        gbBorderAutomatic	 = false;
        theApp.updateWindowSize(theApp.videoOption);
	}

#   else
    /// FIXME
#   endif
}

int VBAMovieCreate(const char *filename, const char*authorInfo, uint8 startFlags, uint8 controllerFlags, uint8 typeFlags)
{
    // make sure at least one controller is enabled
    if ((controllerFlags & MOVIE_CONTROLLERS_ANY_MASK) == 0)
		return WRONG_FORMAT;

    if (!emulating)
		return UNKNOWN_ERROR;

    loadingMovie = true;

    FILE*  file;
    STREAM stream;
    int	   fn;

    char movie_filename [_MAX_PATH];
#ifdef WIN32
    _fullpath(movie_filename, filename, _MAX_PATH);
#else
    // FIXME: convert to fullpath
    strncpy(movie_filename, filename, _MAX_PATH);
    movie_filename[_MAX_PATH-1] = '\0';
#endif

    bool alreadyOpen = (Movie.file != NULL && stricmp(movie_filename, Movie.filename) == 0);

    if (alreadyOpen)
		change_state(MOVIE_STATE_NONE);  // have to stop current movie before trying to re-open it

    if (movie_filename[0] == '\0')
    {loadingMovie = false; return FILE_NOT_FOUND; }

    if (!(file = fopen(movie_filename, "wb")))
    {loadingMovie = false; return FILE_NOT_FOUND; }

    if (!alreadyOpen)
		change_state(MOVIE_STATE_NONE);  // stop current movie when we're able to open the other one

    // clear out the current movie
    VBAMovieInit();

    // fill in the movie's header
    Movie.header.uid			 = (uint32)time(NULL);
    Movie.header.magic			 = VBM_MAGIC;
    Movie.header.version		 = VBM_VERSION;
    Movie.header.rerecord_count	 = 0;
    Movie.header.length_frames	 = 0;
    Movie.header.startFlags		 = startFlags;
    Movie.header.controllerFlags = controllerFlags;
    Movie.header.typeFlags		 = typeFlags;
    Movie.header.reservedByte	 = 0;

    // set emulator settings that make the movie more likely to stay synchronized when it's later played back
    SetRecordEmuSettings();

    // set ROM and BIOS checksums and stuff
    CalcROMInfo();

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
        {loadingMovie = false; return FILE_NOT_FOUND; }

        // write the save data:
        if (Movie.header.startFlags & MOVIE_START_FROM_SNAPSHOT)
        {
            // save snapshot
            if (!theEmulator.emuWriteStateToStream(stream))
            {
                utilGzClose(stream);
                {loadingMovie = false; return UNKNOWN_ERROR; }
			}
		}
        else if (Movie.header.startFlags & MOVIE_START_FROM_SRAM)
        {
            // save SRAM
            if (!theEmulator.emuWriteBatteryToStream(stream))
            {
                utilGzClose(stream);
                {loadingMovie = false; return UNKNOWN_ERROR; }
			}

            // 'soft' reset:
            theEmulator.emuReset(false);
		}

        utilGzClose(stream);

        // reopen the file and seek back to the end

        if (!(file = fopen(movie_filename, "rb+")))
        {loadingMovie = false; return FILE_NOT_FOUND; }

        fseek(file, 0, SEEK_END);
	}
    else // no snapshot or SRAM
    {
        HardResetAndSRAMClear();
	}

    Movie.header.offset_to_controller_data = (uint32)ftell(file);

    resetSignaled = false;
    controllersLeftThisFrame = Movie.header.controllerFlags;

    // write controller data
    Movie.file = file;
    Movie.bytesPerFrame	 = bytes_per_frame(Movie);
    Movie.inputBufferPtr = Movie.inputBuffer + Movie.bytesPerFrame;	// + 1 dummy frame

    reserve_buffer_space(Movie.bytesPerFrame);

	// write "baseline" controller data
//    write_frame_controller_data(0); // correct if we can assume the first controller is active, which we can on all GBx/xGB
//                                    // systems
    Movie.currentFrame = 0;

    strcpy(Movie.filename, movie_filename);
    Movie.readOnly = false;
    change_state(MOVIE_STATE_RECORD);

    Movie.RecordedThisSession = true;

    systemScreenMessage("Recording movie...");
    {loadingMovie = false; return SUCCESS; }
}

void VBAUpdateButtonPressDisplay()
{
    uint32 keys = currentButtons[theApp.joypadDefault] & BUTTON_REGULAR_RECORDING_MASK;

    const static char KeyMap[]	 =  {'A', 'B', 's', 'S', '>', '<', '^', 'v', 'R', 'L', '!', '?', '{', '}', 'v', '^'};
    const static int  KeyOrder[] = {5, 6, 4, 7,  0, 1, 9, 8, 3, 2,  12, 15, 13, 14,  10}; // < ^ > v   A B  L R  S s  { = } _  !
    char buffer[256];
    sprintf(buffer, "                       ");

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

    static int lastKeys = 0;

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

                colorList[strlen("    ")+i] = colorNum;
                pressed = true;
			}
            buffer[strlen("    ") + i] = pressed ? KeyMap[j] : ' ';
		}
	}

    lastKeys  = currentButtons[theApp.joypadDefault];
    lastKeys |= theApp.autoHold & BUTTON_REGULAR_RECORDING_MASK;
    lastKeys |= (theApp.autoFire | theApp.autoFire2) & BUTTON_REGULAR_RECORDING_MASK;

    systemScreenMessage(buffer, 2, -1, colorList);
#endif
}

void VBAUpdateFrameCountDisplay()
{
#if (!(defined(WIN32) && !defined(SDL)))
    extern int cartridgeType; // from SDL.cpp
#endif
    const int MAGICAL_NUMBER = 64;  // FIXME: this won't do any better, but only to remind you of sz issues
    char	  frameDisplayString[MAGICAL_NUMBER];
    char	  lagFrameDisplayString[MAGICAL_NUMBER];
    struct EmulatedSystemCounters &emuCounters =
#if (defined(WIN32) && !defined(SDL))
        (theApp.cartridgeType == 0) // GBA
#else
        (cartridgeType == 0) // GBA
#endif
        ? GBASystemCounters : GBSystemCounters;

    switch (Movie.state)
    {
	case MOVIE_STATE_PLAY :
		{
#           if (defined(WIN32) && !defined(SDL))
		    if (theApp.frameCounter)
		    {
		        sprintf(frameDisplayString, "%d / %d", Movie.currentFrame, Movie.header.length_frames);
		        if (theApp.lagCounter)
		        {
		            sprintf(lagFrameDisplayString, " | %d%s", emuCounters.lagCount, emuCounters.laggedLast ? " *" : "");
		            strncat(frameDisplayString, lagFrameDisplayString, MAGICAL_NUMBER);
				}
			}
		    else
		    {
		        frameDisplayString[0] = '\0';
			}
		    systemScreenMessage(frameDisplayString, 1, -1);
#           else
		    /// SDL FIXME
#           endif
		    break;
		}

	case MOVIE_STATE_RECORD:
		{
#       if (defined(WIN32) && !defined(SDL))
		    if (theApp.frameCounter)
		    {
		        sprintf(frameDisplayString, "%d (movie)", Movie.currentFrame);
		        if (theApp.lagCounter)
		        {
		            sprintf(lagFrameDisplayString, " | %d%s", emuCounters.lagCount, emuCounters.laggedLast ? " *" : "");
		            strncat(frameDisplayString, lagFrameDisplayString, MAGICAL_NUMBER);
				}
			}
		    else
		    {
		        frameDisplayString[0] = '\0';
			}
		    systemScreenMessage(frameDisplayString, 1, -1);
#       else
		    /// SDL FIXME
#       endif
		    break;
		}

	default:
		{
#       if (defined(WIN32) && !defined(SDL))
		    if (theApp.frameCounter)
		    {
		        sprintf(frameDisplayString, "%d (no movie)", emuCounters.frameCount);
		        if (theApp.lagCounter)
		        {
		            sprintf(lagFrameDisplayString, " | %d%s", emuCounters.lagCount, emuCounters.laggedLast ? " *" : "");
		            strncat(frameDisplayString, lagFrameDisplayString, MAGICAL_NUMBER);
				}
			}
		    else
		    {
		        frameDisplayString[0] = '\0';
			}
		    systemScreenMessage(frameDisplayString, 1, -1);
#       else
		    /// SDL FIXME
#       endif
		    break;
		}
	}
}

void VBAMovieUpdate(int controllerNum, bool sensor)
{
    bool willRestart = false;
    bool willPause	 = false;

    switch (Movie.state)
    {
	case MOVIE_STATE_PLAY:
		{
		    if ((Movie.header.controllerFlags & MOVIE_CONTROLLER(controllerNum)) == 0)
				break;  // not a controller we're recognizing

		    if (Movie.currentFrame >= Movie.header.length_frames)
		    {
#if (defined(WIN32) && !defined(SDL))
		        if (theApp.movieOnEndBehavior == 1)
		        {
		            willRestart = true;
		            willPause	= theApp.movieOnEndPause;
		            break;
				}
		        else if (theApp.movieOnEndBehavior == 2 && Movie.RecordedThisSession)
#else
		        // SDL FIXME
		        if (Movie.RecordedThisSession)
#endif
		        {
		            // if user has been recording this movie since the last time it started playing,
		            // they probably don't want the movie to end now during playback,
		            // so switch back to recording when it reaches the end
		            if (Movie.readOnly)
		            {
		                VBAMovieToggleReadOnly();
					}

		            VBAMovieSwitchToRecording();
		            willPause = true;
		            break;
//					goto movieUpdateStart;
				}
		        else
		        {
		            // stop movie; it reached the end
		            change_state(MOVIE_STATE_NONE);
		            systemScreenMessage("Movie end");
#if (defined(WIN32) && !defined(SDL))
		            willPause = theApp.movieOnEndPause;
#else
		            willPause = false; // SDL FIXME
#endif
		            break;
				}
			}
		    else
		    {
		        read_frame_controller_data(controllerNum);
		        ++Movie.currentFrame;
			}
		    break;
		}

	case MOVIE_STATE_RECORD:
		{
		    if ((Movie.header.controllerFlags & MOVIE_CONTROLLER(controllerNum)) == 0)
				break;  // not a controller we're recognizing

		    write_frame_controller_data(controllerNum);
		    ++Movie.currentFrame;
		    Movie.header.length_frames = Movie.currentFrame;
		    fwrite((Movie.inputBufferPtr - Movie.bytesPerFrame), 1, Movie.bytesPerFrame, Movie.file);

		    Movie.RecordedThisSession = true;
		    break;
		}

	default:
		break;
	}

    // if the movie's been set to pause at a certain frame
    if (VBAMovieActive() && Movie.pauseFrame >= 0 && Movie.currentFrame >= (uint32)Movie.pauseFrame)
    {
        Movie.pauseFrame = -1;
        willPause		 = true;
	}

    if (willRestart)
    {
        VBAMovieRestart();
	}

    if (willPause)
    {
        extern void systemPauseEmulator(bool = true);
        systemPauseEmulator(true);
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
    flush_movie();

    assert(info != NULL);
    if (info == NULL)
		return -1;

    FILE*	 file;
    int		 result;
    SMovie & local_movie = *info;

    memset(info, 0, sizeof(*info));
    if (filename[0] == '\0')
		return FILE_NOT_FOUND;
    if (!(file = fopen(filename, "rb")))
		return FILE_NOT_FOUND;

    // read header
    if ((result = (read_movie_header(file, local_movie))) != SUCCESS)
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
            (fileSize - local_movie.header.offset_to_controller_data) / local_movie.bytesPerFrame - 1;	// -1 dummy
	}

    fclose(file);

    if (access(filename, W_OK))
		info->readOnly = true;

    return SUCCESS;
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

void VBAMovieFreeze(uint8 **buf, uint32 *size)
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
    size_needed += (uint32)(Movie.bytesPerFrame * (Movie.header.length_frames + 1));	// + 1 dummy frame
    *buf		 = new uint8[size_needed];
    *size		 = size_needed;

    uint8 *ptr = *buf;
    if (!ptr)
    {
        return;
	}

    Write32(Movie.header.uid, ptr);
    Write32(Movie.currentFrame, ptr);
    Write32(Movie.header.length_frames, ptr);

    memcpy(ptr, Movie.inputBuffer, Movie.bytesPerFrame * (Movie.header.length_frames + 1));	// + 1 dummy frame
}

int VBAMovieUnfreeze(const uint8 *buf, uint32 size)
{
    // sanity check
    if (!VBAMovieActive())
    {
        return NOT_FROM_A_MOVIE;
	}

    const uint8 *ptr = buf;
    if (size < sizeof(Movie.header.uid) + sizeof(Movie.currentFrame) + sizeof(Movie.header.length_frames))
    {
        return WRONG_FORMAT;
	}

    uint32 movie_id		 = Read32(ptr);
    uint32 current_frame = Read32(ptr);
    uint32 end_frame	 = Read32(ptr);
    uint32 space_needed	 = Movie.bytesPerFrame * (end_frame + 1);	// + 1 dummy

    if (movie_id != Movie.header.uid)
		return NOT_FROM_THIS_MOVIE;

    if (current_frame > end_frame)
		return SNAPSHOT_INCONSISTENT;

    if (space_needed > size)
		return WRONG_FORMAT;

    if (Movie.readOnly)
    {
        // here, we are going to keep the input data from the movie file
        // and simply rewind to the currentFrame pointer
        // this will cause a desync if the savestate is not in sync // <-- NOT ANYMORE
        // with the on-disk recording data, but it's easily solved
        // by loading another savestate or playing the movie from the beginning

        // don't allow loading a state inconsistent with the current movie
        uint32 space_shared = (Movie.bytesPerFrame * (current_frame + 1));	// + 1 dummy frame
        if (current_frame > Movie.header.length_frames ||
            memcmp(Movie.inputBuffer, ptr, space_shared))
			return SNAPSHOT_INCONSISTENT;

        change_state(MOVIE_STATE_PLAY);
///		systemScreenMessage("Movie rewind");

        Movie.currentFrame = current_frame;
	}
    else
    {
        // here, we are going to take the input data from the savestate
        // and make it the input data for the current movie, then continue
        // writing new input data at the currentFrame pointer
        change_state(MOVIE_STATE_RECORD);
///		systemScreenMessage("Movie re-record");

        Movie.currentFrame		   = current_frame;
        Movie.header.length_frames = end_frame;
        if (!VBALuaRerecordCountSkip())
			++Movie.header.rerecord_count;

        reserve_buffer_space(space_needed);
        memcpy(Movie.inputBuffer, ptr, space_needed);
        flush_movie();
        fseek(Movie.file, Movie.header.offset_to_controller_data + (Movie.bytesPerFrame * (Movie.currentFrame + 1)), SEEK_SET);	// +1
	}

    Movie.inputBufferPtr = Movie.inputBuffer + (Movie.bytesPerFrame * (Movie.currentFrame + 1));	// +1

    return SUCCESS;
}

bool8 VBAMovieSwitchToRecording()
{
    if (Movie.state != MOVIE_STATE_PLAY || Movie.readOnly)
		return false;

    change_state(MOVIE_STATE_RECORD);
    systemScreenMessage("Movie re-record");

    Movie.header.length_frames = Movie.currentFrame;
    if (!VBALuaRerecordCountSkip())
		++Movie.header.rerecord_count;

    flush_movie();

    return true;
}

uint32 VBAGetCurrentInputOf(int controllerNum, bool normalOnly)
{
    if (controllerNum < 0 || controllerNum >= MOVIE_NUM_OF_POSSIBLE_CONTROLLERS)
		return 0;

    return normalOnly ? (currentButtons[controllerNum] & BUTTON_REGULAR_MASK) : currentButtons[controllerNum];
}

uint32 VBAMovieGetState()
{
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
    if (VBAMovieActive() && resetSignaled)
    {
        theEmulator.emuReset(false);
        resetSignaled = false;
	}
}

void VBAMovieSetMetadata(const char *info)
{
    memcpy(Movie.authorInfo, info, MOVIE_METADATA_SIZE); // strncpy would omit post-0 bytes
    Movie.authorInfo[MOVIE_METADATA_SIZE-1] = '\0';

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
        bool8 modified = Movie.RecordedThisSession;

        VBAMovieStop(true);

        char movieName [_MAX_PATH];
        strncpy(movieName, Movie.filename, _MAX_PATH);
        VBAMovieOpen(movieName, Movie.readOnly); // can't just pass in Movie.filename, since VBAMovieOpen clears out Movie's
                                                 // variables

        Movie.RecordedThisSession = modified;

//		systemScreenMessage("movie replay (restart)");
        systemRefreshScreen();
	}
}

void VBAMovieSetPauseAt(int at)
{
    Movie.pauseFrame = at;
}

