/******************************************************************************
                                  displayLib.c

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

/* displayLib.c
 *
 * displayLib is a library of basic front panel primitives.  These include
 * writing to the screen, cursor motion, visual character attributes, tab stop
 * manipulation and clearing the screen.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "fit.h"
#include "displayLib.h"

void writeOut(const char * string)
{
  ssize_t bCnt;

  if(fpd != 0)
  {
    bCnt = write(fpd, string, strlen(string));
    if(bCnt == -1)
    {
      fitPrint(ERROR, "%s: write failed, err %d, %s\n",__func__,errno,strerror(errno));
    }
    else
    {
      if(bCnt != (int32)strlen(string))
      {
        fitPrint(ERROR, "%s: write returned %d bytes, expected %u bytes\n",
                 __func__,bCnt,strlen(string));
      }
    }
  }
  else
  {
    fitPrint(ERROR, "ERROR: Invalid connection to /dev/sp6\n");
  }
}


void initDisplay(void)
{
  // set all the display stuff
  usleep(100000);
  setAutoScroll(false);
  setAutoWrap(false);
  setCursor(false);
  setReverseVideo(false);
  setUnderline(false);
  setCharBlink(false);
  clearScreen();
}


void clearScreen()
{
  // clear all
  writeOut("\x1b" "[2J");
  // move cursor home
  moveCursorHome();
}


void moveCursorHome(void)
{
  writeOut("\x1b" "[H");
}


void moveCursor (u_int32 x, u_int32 y)
{
  char string[12];

  snprintf(string, sizeof(string), "\x1b" "[%lu;%luf", y, x);

  writeOut(string);
}


void setAutoScroll(bool enable)
{
  if (enable)
  {
    writeOut("\x1b" "[<47h");
  }
  else
  {
    writeOut("\x1b" "[<47l");
  }
}


void setCursor(bool enable)
{
  if (enable)
  {
    writeOut("\x1b" "[?25h");
  }
  else
  {
    writeOut("\x1b" "[?25l");
  }
}


void setReverseVideo(bool enable)
{
  if (enable)
  {
    writeOut("\x1b" "[27h");
  }
  else
  {
    writeOut("\x1b" "[27l");
  }
}


void setUnderline(bool enable)
{
  if (enable)
  {
    writeOut("\x1b" "[24h");
  }
  else
  {
    writeOut("\x1b" "[24l");
  }
}


void setCharBlink(bool enable)
{
  if (enable)
  {
    writeOut("\x1b" "[25h");
  }
  else
  {
    writeOut("\x1b" "[25l");
  }
}


void setAutoWrap(bool enable)
{
  if (enable)
  {
    writeOut("\x1b" "[?7h");
  }
  else
  {
    writeOut("\x1b" "[?7l");
  }
}


void setTabStop(void)
{
  writeOut("\x1b" "H");
}


void clearTabStop(int32 num)
{
  char string[10];

  if ((num >= 0) && (num < 4))
  {
    snprintf(string, sizeof(string), "\x1b" "[%ldg", num);
    writeOut(string);
  }
}


void setBacklightTime(int32 mins)
{
  char string[10];

  if ((mins >= 0) && (mins < 64))
  {
    snprintf(string, sizeof(string), "\x1b" "[<%ldS", mins);
    writeOut(string);
  }
}


void backlightOn(void)
{
  writeOut("\x1b" "[<5h");
}


void backlightOff(void)
{
  writeOut("\x1b" "[<5l");

}


void resetDisplay(void)
{
  char buf[50];

  writeOut("\x1b" "c");
  usleep(250000);
  (void)read(fpd,buf,sizeof(buf));
}
