/******************************************************************************
                                 displayLib.h

    Copyright (c) 2015-2017 Siemens Industry, Inc.
    Original authors: Jack McCarthy, Andrew Valdez and Jonathan Grant

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
        Free Software Foundation, Inc.
        51 Franklin Street, Fifth Floor
        Boston MA  02110-1301 USA.

*******************************************************************************/

#ifndef DISPLAYLIB_H
  #define DISPLAYLIB_H
//lint -library

  #ifndef FTYPES_H
    #include "ftypes.h"
  #endif

extern int32 fpd;  // file descriptor for the display

void writeOut(const char * string);
void clearScreen(void);
void moveCursorHome(void);
void moveCursor(u_int32 x, u_int32 y);
void initDisplay(void);
void resetDisplay(void);

void setCursor(bool enable);
void setAutoScroll(bool enable);
void setReverseVideo(bool enable);
void setUnderline(bool enable);
void setAutoWrap(bool enable);
void setCharBlink(bool enable);

void setBacklightTime(int32 mins);
void backlightOn(void);
void backlightOff(void);

void setTabStop(void);
void clearTabStop(int32 num);

#endif
