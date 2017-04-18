/******************************************************************************
                                    todFit.c

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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ioctl.h>


#include "fit.h"

// Time of Day Driver Definitions from atc.h (ATC 6.24)
//---------------------
#define ATC_TIMING_TOD_DEV            "/dev/tod"
#define ATC_TOD_SET_TIMESRC           _IOW('T', 1, unsigned long)
#define ATC_TOD_GET_TIMESRC           _IO('T', 2)
#define ATC_TOD_GET_INPUT_FREQ        _IO('T', 3)
#define ATC_TOD_REQUEST_TICK_SIG      _IOW('T', 4, unsigned long)
#define ATC_TOD_CANCEL_TICK_SIG       _IO('T', 5)
#define ATC_TOD_REQUEST_ONCHANGE_SIG  _IOW('T', 6, unsigned long)
#define ATC_TOD_CANCEL_ONCHANGE_SIG   _IO('T', 7)

enum timesrc_enum
{
  ATC_TIMESRC_LINESYNC,
  ATC_TIMESRC_RTCSQWR,
  ATC_TIMESRC_CRYSTAL,
  ATC_TIMESRC_EXTERNAL1,
  ATC_TIMESRC_EXTERNAL2
};
//-----------------------

/* Types of Time Source */
#define NOSRC (-1)
#define LINESYNC 0
#define RTCSQRW 1
#define CRYSTAL 2
#define EXT1 3
#define EXT2 4
#define SIG_DEFAULT 44

//-----------------------
// From SDLC Device Driver ioctl commands from atc_spxs.h (ATC 6.24)
#define ATC_SPXS_WRITE_CONFIG  0
#define ATC_SPXS_READ_CONFIG   1
//-----------------------


static u_int32 count = 0;
static pthread_mutex_t lock;

static void receiveData(int32 n, siginfo_t *info, void *unused) {
    printf("received value %i\n", info->si_int);
    if (info->si_int == 1234) {
      //Consolelog(ATC_TIMING_TOD_DEV,"Receive data on event","Pass");
    }
    //Consolelog(ATC_TIMING_TOD_DEV,"Receive data on event","Fail");
    pthread_mutex_unlock(&lock);
}

ftRet_t todFit ( plint argc, char * const argv[] )
{
    /* setup the signal handler for SIG_TEST
     * SA_SIGINFO -> we want the signal handler function with 3 arguments
     */
    int32   fd;
    int32   result;
    u_int32 buf = 0;
    struct sigaction sig;

    sig.sa_sigaction = receiveData;
    sig.sa_flags = SA_SIGINFO;
    sigaction(SIG_DEFAULT, &sig,NULL);

    pthread_mutex_init(&lock,NULL);

    fd = open(ATC_TIMING_TOD_DEV, O_RDONLY);
    if(fd == -1) {
  //Consolelog(ATC_TIMING_TOD_DEV,"Open Device","Fail");
        perror("open");
        return -1;
    } else {
  //Consolelog(ATC_TIMING_TOD_DEV,"Open Device","Pass");
    }

    buf = LINESYNC;
    result = ioctl(fd, ATC_TOD_SET_TIMESRC, &buf);
    if (result != 0) {
        perror("SET TIME SRC");
  //Consolelog(ATC_TIMING_TOD_DEV,"ATC_TOD_SET_TIMESRC using ioctl","Fail");
        close(fd);
        return -1;
    } else {
  //Consolelog(ATC_TIMING_TOD_DEV,"ATC_TOD_SET_TIMESRC using ioctl","Pass");
    }

    u_int32 timesrc = ioctl(fd, ATC_TOD_GET_TIMESRC, &buf);
    printf("Current Time Source is %ld, timesrc %ld.\n", buf,timesrc);
    if (buf == LINESYNC) {
  //Consolelog(ATC_TIMING_TOD_DEV,"ATC_TOD_GET_TIMESRC using ioctl","Pass");
    } else {
  //Consolelog(ATC_TIMING_TOD_DEV,"ATC_TOD_GET_TIMESRC using ioctl","Fail");
    }

    buf = SIG_DEFAULT;
    result = ioctl(fd, ATC_TOD_REQUEST_TICK_SIG, &buf);
    if (result != 0) {
  //Consolelog(ATC_TIMING_TOD_DEV,"ATC_TOD_REQUEST_TICK_SIG using ioctl","Fail");
        perror("Request TICK SIG");
        close(fd);
        return -1;
    } else {
  //Consolelog(ATC_TIMING_TOD_DEV,"ATC_TOD_REQUEST_TICK_SIG using ioctl","Pass");
    }

    usleep(100000);
    pthread_mutex_destroy(&lock);

    while (count <= 100) {
        count++;
        sig.sa_sigaction = receiveData;
        sig.sa_flags = SA_SIGINFO;
        sigaction(SIG_DEFAULT, &sig,NULL);
        pthread_mutex_lock(&lock);
    }

    usleep(1000000);
    if (ioctl(fd, ATC_TOD_CANCEL_TICK_SIG)) {
  //Consolelog(ATC_TIMING_TOD_DEV,"ATC_TOD_CANCEL_TICK_SIG using ioctl","Fail");
        perror("Cancel TICK SIG");
        close(fd);
        return -1;
    } else {
  //Consolelog(ATC_TIMING_TOD_DEV,"ATC_TOD_CANCEL_TICK_SIG using ioctl","Pass");
    }

    pthread_mutex_destroy(&lock);
#if 0
    if (close(fd) < 0 )
      //Consolelog(ATC_TIMING_TOD_DEV, "Close Device", "Fail");
    else
      //Consolelog(ATC_TIMING_TOD_DEV, "Close Device", "Pass");
#endif

    return 0;
}
