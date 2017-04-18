/******************************************************************************
                                 powerdownFit.c

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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "fit.h"

static ftResults_t ftResults = {0};
static ftResults_t *ftrp = &ftResults;
static const char *powerdownDev = "/dev/powerdown";

ftRet_t powerdownFit(plint argc,char * const argv[])
{
  int32   fd;
  bool    loopOn = true;
  ssize_t bCnt;
  char    pbuff = '9';

  if (argc > 1)
  {
    fitPrint(USER, "Usage: %s\nOptions: none are permitted.\n\n",argv[0]);
    fitPrint(USER, "Waits until the powerdown signal is received from the power supply.\n");
    fitPrint(USER, "To complete the test, interrupt power for less than 500 ms.\n\n");
    fitLicense();
    return (ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }


  fd = open(powerdownDev,O_RDONLY);
  if(fd == -1)
  {
    fitPrint(ERROR,"%s: cannot open /dev/powerdown\n",__func__);
    ftUpdateTestStatus(ftrp,ftError,"open error");
    ftUpdateTestStatus(ftrp,ftComplete,NULL);
  }
  else
  {
    while(loopOn)
    {
      bCnt = read(fd,&pbuff,1); // blocking system call

      if(bCnt == 1)
      {
        if (pbuff == '\0')
        {
          fitPrint(VERBOSE, "Power interruption detected.\n");
          ftUpdateTestStatus(ftrp,ftPass,NULL);
          
        }
        else
        {
          fitPrint(VERBOSE, "Power restoration detected.\n");
          ftUpdateTestStatus(ftrp,ftFail,NULL);
          ftUpdateTestStatus(ftrp,ftComplete,"power down and up");
          loopOn = false;
        }
      }
      else if(bCnt == -1)
      {
        fitPrint(VERBOSE, "powerdown: read failed - %s.\n",strerror(errno));
        ftUpdateTestStatus(ftrp,ftInterrupt,NULL);
        ftUpdateTestStatus(ftrp,ftComplete,"read interrupted");
        loopOn = false;
      }
      else
      {
        // loop back around and try again
      }
    }

    close(fd);
  }

  return(ftComplete);
}
