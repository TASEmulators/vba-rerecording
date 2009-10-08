#ifndef VBA_MOVIE_H
#define VBA_MOVIE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string>

#include "../Port.h"

#ifndef SUCCESS
#  define SUCCESS 1
#  define WRONG_FORMAT (-1)
#  define WRONG_VERSION (-2)
#  define FILE_NOT_FOUND (-3)
#  define NOT_FROM_THIS_MOVIE (-4)
#  define NOT_FROM_A_MOVIE (-5)
#  define SNAPSHOT_INCONSISTENT (-6)
#  define UNKNOWN_ERROR (-7)
#endif

#define VBM_MAGIC (0x1a4D4256) // VBM0x1a
#define VBM_VERSION (1)
#define VBM_HEADER_SIZE (64)
#define CONTROLLER_DATA_SIZE (2)
#define BUFFER_GROWTH_SIZE (4096)
#define MOVIE_METADATA_SIZE (192)
#define MOVIE_METADATA_AUTHOR_SIZE (64)

#define MOVIE_START_FROM_SNAPSHOT   (1<<0)
#define MOVIE_START_FROM_SRAM       (1<<1)

#define MOVIE_CONTROLLER(i)         (1<<(i))
#define MOVIE_CONTROLLERS_ANY_MASK  (MOVIE_CONTROLLER(0)|MOVIE_CONTROLLER(1)|MOVIE_CONTROLLER(2)|MOVIE_CONTROLLER(3))
#define MOVIE_NUM_OF_POSSIBLE_CONTROLLERS   (4)

#define MOVIE_TYPE_GBA              (1<<0)
#define MOVIE_TYPE_GBC              (1<<1)
#define MOVIE_TYPE_SGB              (1<<2)

#define MOVIE_SETTING_USEBIOSFILE   (1<<0)
#define MOVIE_SETTING_SKIPBIOSFILE  (1<<1)
#define MOVIE_SETTING_RTCENABLE     (1<<2)
#define MOVIE_SETTING_UNSUPPORTED1  (1<<3)
#define MOVIE_SETTING_LAGHACK       (1<<4)
#define MOVIE_SETTING_GBCFF55FIX    (1<<5)
#define MOVIE_SETTING_GBECHORAMFIX  (1<<6)

#define ZLIB

///#ifdef ZLIB
#ifndef WIN32
#include "zlib.h"
#endif
#define STREAM gzFile
/*#define READ_STREAM(p,l,s) gzread (s,p,l)
 #define WRITE_STREAM(p,l,s) gzwrite (s,p,l)
 #define OPEN_STREAM(f,m) gzopen (f,m)
 #define REOPEN_STREAM(f,m) gzdopen (f,m)
 #define FIND_STREAM(f)	gztell(f)
 #define REVERT_STREAM(f,o,s)  gzseek(f,o,s)
 #define CLOSE_STREAM(s) gzclose (s)
 #else
 #define STREAM FILE *
 #define READ_STREAM(p,l,s) fread (p,1,l,s)
 #define WRITE_STREAM(p,l,s) fwrite (p,1,l,s)
 #define OPEN_STREAM(f,m) fopen (f,m)
 #define REOPEN_STREAM(f,m) fdopen (f,m)
 #define FIND_STREAM(f)	ftell(f)
 #define REVERT_STREAM(f,o,s)	 fseek(f,o,s)
 #define CLOSE_STREAM(s) fclose (s)
 #endif*/

enum MovieState
{
	MOVIE_STATE_NONE = 0,
	MOVIE_STATE_PLAY,
	MOVIE_STATE_RECORD
};

struct SMovieFileHeader
{
	uint32 magic;       // VBM0x1a
	uint32 version;     // 1
	int32  uid;         // used to match savestates to a particular movie
	uint32 length_frames;
	uint32 rerecord_count;
	uint8  startFlags;
	uint8  controllerFlags;
	uint8  typeFlags;
	uint8  optionFlags;
	uint32 saveType;        // emulator setting value
	uint32 flashSize;       // emulator setting value
	uint32 gbEmulatorType;  // emulator setting value
	char   romTitle [12];
	uint8  reservedByte;
	uint8  romCRC;          // the CRC of the ROM used while recording
	uint16 romOrBiosChecksum;       // the Checksum of the ROM used while recording, or a CRC of the BIOS if GBA
	uint32 romGameCode;     // the Game Code of the ROM used while recording, or "\0\0\0\0" if not GBA
	uint32 offset_to_savestate;         // offset to the savestate or SRAM inside file, set to 0 if unused
	uint32 offset_to_controller_data;   // offset to the controller data inside file
};

struct SMovie
{
	enum MovieState state;
	char  filename [/*_MAX_PATH*/ 260]; // FIXME: should use a string instead
	FILE* file;
	uint8 readOnly;
	int   pauseFrame;

	SMovieFileHeader header;
	char authorInfo [MOVIE_METADATA_SIZE];

	uint32 currentFrame;    // should = length_frames when recording, and be < length_frames when playing
	uint32 bytesPerFrame;
	uint8* inputBuffer;
	uint32 inputBufferSize;
	uint8* inputBufferPtr;

	bool8 RecordedThisSession;
};

// methods used by the user-interface code
int VBAMovieOpen(const char*filename, bool8 read_only);
int VBAMovieCreate(const char*filename, const char*authorInfo, uint8 startFlags, uint8 controllerFlags, uint8 typeFlags);
int VBAMovieGetInfo(const char*filename, SMovie*info);
void VBAMovieStop(bool8 suppress_message);
const char *VBAChooseMovieFilename(bool8 read_only);

// methods used by the emulation
void VBAMovieInit();
void VBAMovieUpdate(int controllerNum = 0);
void VBAUpdateFrameCountDisplay();
//bool8 VBAMovieRewind (uint32 at_frame);
void VBAMovieFreeze(uint8**buf, uint32*size);
int VBAMovieUnfreeze(const uint8*buf, uint32 size);
void VBAMovieRestart();

// accessor functions
bool8 VBAMovieActive();
bool8 VBAMovieLoading();
bool8 VBAMoviePlaying();
bool8 VBAMovieRecording();
// the following accessors return 0/false if !VBAMovieActive()
uint8 VBAMovieReadOnly();
uint32 VBAMovieGetId();
uint32 VBAMovieGetLength();
uint32 VBAMovieGetFrameCounter();
uint32 VBAMovieGetState();
uint32 VBAMovieGetRerecordCount ();
uint32 VBAMovieSetRerecordCount (uint32 newRerecordCount);
std::string VBAMovieGetAuthorInfo();
std::string VBAMovieGetFilename();

uint32 VBAGetCurrentInputOf(int controllerNum, bool normalOnly = true);
void VBAMovieSignalReset();
void VBAMovieResetIfRequested();
void VBAMovieSetMetadata(const char *info);
void VBAMovieToggleReadOnly();
bool8 VBAMovieSwitchToRecording();
void VBAMovieSetPauseAt();
void VBAMovieSetPauseAt(int at);

extern bool8 loadedMovieSnapshot;

#endif // VBA_MOVIE_H
