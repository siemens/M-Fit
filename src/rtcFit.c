/******************************************************************************
                                    rtcFit.c

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

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>

#include "fit.h"

static ftResults_t ftResults = {0};
static ftResults_t *ftrp = &ftResults;

/*
*
* Test the I2C bus by communicating with system Real Time Clock
*
*/

ftRet_t rtcFit(plint argc,char * const argv[])
{
  int32 err, rtcFd;
  struct rtc_time tm1,tm2;
  const char *rtcName = "/dev/rtc";
  ftRet_t ret = ftFail;

  rtcFd = open(rtcName,O_RDWR);

  if(rtcFd == -1){
    fitPrint(ERROR, "%s test cannot open %s, err %d: %s\n",
             argv[0],rtcName,errno,strerror(errno));
    ftUpdateTestStatus(ftrp,ftError,NULL);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  err = ioctl(rtcFd,RTC_RD_TIME,&tm1);

  if(err!=0){
    fitPrint(ERROR, "%s can't read %s(1), err = %d, %s\n",
             argv[0],rtcName,errno,strerror(errno));
    close(rtcFd);
    ftUpdateTestStatus(ftrp,ftError,NULL);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  sleep(5);

  err = ioctl(rtcFd,RTC_RD_TIME,&tm2);

  if(err!=0){
    fitPrint(ERROR, "%s can't read %s(2), err = %d, %s\n",
             argv[0],rtcName,errno,strerror(errno));
    close(rtcFd);
    ftUpdateTestStatus(ftrp,ftError,NULL);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  // try to set using the most recent get
  err = ioctl(rtcFd,RTC_SET_TIME,&tm2);

  if(err!=0){
    fitPrint(ERROR, "%s can't write %s(2), err = %d, %s\n",
             argv[0],rtcName,errno,strerror(errno));
    close(rtcFd);
    ftUpdateTestStatus(ftrp,ftError,NULL);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  err = memcmp(&tm1,&tm2,sizeof(tm1));

  if(err!=0){// times are different, assume RTC is running
    ret = ftPass;
  } else {
    ret = ftFail;
  }

  close(rtcFd);

  ftUpdateTestStatus(ftrp,ret,NULL);// disposition is based on the existance of errors and fails
  return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
}
