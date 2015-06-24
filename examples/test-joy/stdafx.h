// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include "windows.h"
#include "mmsystem.h"
#include "libvisca.h"
#include <math.h>

#define DEBUG 0
#define D30ONLY 0

#define MAX_MEMS 16
#define MAX_CAMS 8

struct SAVED_POINT {
	int16_t pan_pos;
	int16_t tilt_pos;
	uint16_t zoom_pos;
	bool backlight; 
	int camera;
	bool saved;
	uint16_t exposure;
};

#define BUTTON1  0x0001
#define BUTTON2  0x0002
#define BUTTON3  0x0004
#define BUTTON4  0x0008
#define BUTTON5  0x0010
#define BUTTON6  0x0020
#define BUTTON7  0x0040
#define BUTTON8  0x0080
#define BUTTON9  0x0100
#define BUTTON10  0x0200
#define BUTTON11  0x0400
#define BUTTON12  0x0800
#define BUTTON13  0x1000
#define BUTTON14  0x2000
#define BUTTON15  0x4000
#define BUTTON16  0x8000


#define POV_N 0
#define POV_NE  4500
#define POV_E  9000
#define POV_SE  13500
#define POV_S  1800
#define POV_SW  22500
#define POV_W 27000
#define POV_NW 31500

#define JOY_MIDP 32768
#define JOY_DEADZONE 1025

#define PANRIGHT 1
#define PANLEFT 2
#define PANSTOP 0
#define TILTUP 1
#define TILTDOWN 2
#define TILTSTOP 0
#define ZOOMSTOP 0
#define ZOOMTELE 1
#define ZOOMWIDE 2




void pantilt( int pandir, int panspeed, int tiltdir, int tiltspeed,VISCAInterface_t *iface, VISCACamera_t *camera );
void stopcamera(VISCAInterface_t *iface, VISCACamera_t *camera );
BOOL CtrlHandler( DWORD fdwCtrlType );
int process_commandline(int argc, char **argv);
void close_interface(VISCAInterface_t *pIface);
void open_interface(VISCAInterface_t *pIface);
void print_usage();

// TODO: reference additional headers your program requires here
