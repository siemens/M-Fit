/******************************************************************************
                                fioMonitorFit.c

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
#include <time.h>
#include <signal.h>
#include <wait.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h> /* POSIX Threads */
#include <semaphore.h>
#include <string.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>

#include "fit.h"
#include "serialFit.h"

#define RETRY_COUNT 10
#define RXTMO       10 // receive timeout in seconds
#define MONITOR_REG 17u
#define ZEROSx8     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
#define ZEROSx16    ZEROSx8 , ZEROSx8
#define ZEROSx32    ZEROSx16 , ZEROSx16
#define ZEROSx64    ZEROSx32 , ZEROSx32
#define ZEROSx128   ZEROSx64 , ZEROSx64
#define ZEROSx256   ZEROSx128 , ZEROSx128
#define ZEROSx512   ZEROSx256 , ZEROSx256

/*
* global data
*/
static u_int8 rxBuf[1024] = {0}; // global response buffer
static const char *devName = "/dev/sp5s";
static int32 sp5s_fd;
static u_int8 modId = 0;

typedef struct _fioComm {
  size_t txsz,rxsz;
  u_int8 txCmd[1024];
} fioComm_t;

static fioComm_t getModuleIdCmd[] = {
  {3,4,{0x14,0x83,0x3c  //  command to get module ID
  ,0x00,0x00,0x00,0x00,0x00
  ,ZEROSx8,ZEROSx16,ZEROSx32,ZEROSx64,ZEROSx128,ZEROSx256,ZEROSx512}},
  {0,0,{0}}
};

static fioComm_t getRawInputsCmd2[] = {
  {3,15,{0x14,0x83,0x34   // command to get -2 raw inputs
  ,0x00,0x00,0x00,0x00,0x00
  ,ZEROSx8,ZEROSx16,ZEROSx32,ZEROSx64,ZEROSx128,ZEROSx256,ZEROSx512}},
  {0,0,{0}}
};

static fioComm_t getRawInputsCmd8[] = {
  {3,22,{0x14,0x83,0x34   // command to get -8 raw inputs
  ,0x00,0x00,0x00,0x00,0x00
  ,ZEROSx8,ZEROSx16,ZEROSx32,ZEROSx64,ZEROSx128,ZEROSx256,ZEROSx512}},
  {0,0,{0}}
};

#if 0
static fioComm_t getFilteredInputsCmd[] = {
  {3,22,{0x14,0x83,0x35   // command to get filtered inputs
  ,0x00,0x00,0x00,0x00,0x00
  ,ZEROSx8,ZEROSx16,ZEROSx32,ZEROSx64,ZEROSx128,ZEROSx256,ZEROSx512}},
  {0,0,{0}}
};

static fioComm_t fioCommSequence[] = {
{3,4,{0x14,0x83,0x3c //  command to get module ID
  ,0x00,0x00,0x00,0x00,0x00
  ,ZEROSx8,ZEROSx16,ZEROSx32,ZEROSx64,ZEROSx128,ZEROSx256,ZEROSx512}},

{4,10,{0x14,0x83,0x31,0x00
  ,0x00,0x00,0x00,0x00
  ,ZEROSx8,ZEROSx16,ZEROSx32,ZEROSx64,ZEROSx128,ZEROSx256,ZEROSx512}},
{4,10,{0x14,0x83,0x31,0xff
  ,0x00,0x00,0x00,0x00
  ,ZEROSx8,ZEROSx16,ZEROSx32,ZEROSx64,ZEROSx128,ZEROSx256,ZEROSx512}},
{7,4,{0x14,0x83,0x32,0x00,0x00,0x00,0x00
  ,0x00
  ,ZEROSx8,ZEROSx16,ZEROSx32,ZEROSx64,ZEROSx128,ZEROSx256,ZEROSx512}},
{358,4,
{0x14,0x83,0x33,0x76,0x00,0x05,0x05,0x01,0x05,0x05,0x02,0x05,0x05,0x03,0x05,0x05
,0x04,0x05,0x05,0x05,0x05,0x05,0x06,0x05,0x05,0x07,0x05,0x05,0x08,0x05,0x05,0x09
,0x05,0x05,0x0a,0x05,0x05,0x0b,0x05,0x05,0x0c,0x05,0x05,0x0d,0x05,0x05,0x0e,0x05
,0x05,0x0f,0x05,0x05,0x10,0x05,0x05,0x11,0x05,0x05,0x12,0x05,0x05,0x13,0x05,0x05
,0x14,0x05,0x05,0x15,0x05,0x05,0x16,0x05,0x05,0x17,0x05,0x05,0x18,0x05,0x05,0x19
,0x05,0x05,0x1a,0x05,0x05,0x1b,0x05,0x05,0x1c,0x05,0x05,0x1d,0x05,0x05,0x1e,0x05
,0x05,0x1f,0x05,0x05,0x20,0x05,0x05,0x21,0x05,0x05,0x22,0x05,0x05,0x23,0x05,0x05
,0x24,0x05,0x05,0x25,0x05,0x05,0x26,0x05,0x05,0x27,0x05,0x05,0x28,0x05,0x05,0x29
,0x05,0x05,0x2a,0x05,0x05,0x2b,0x05,0x05,0x2c,0x05,0x05,0x2d,0x05,0x05,0x2e,0x05
,0x05,0x2f,0x05,0x05,0x30,0x05,0x05,0x31,0x05,0x05,0x32,0x05,0x05,0x33,0x05,0x05
,0x34,0x05,0x05,0x35,0x05,0x05,0x36,0x05,0x05,0x37,0x05,0x05,0x38,0x05,0x05,0x39
,0x05,0x05,0x3a,0x05,0x05,0x3b,0x05,0x05,0x3c,0x05,0x05,0x3d,0x05,0x05,0x3e,0x05
,0x05,0x3f,0x05,0x05,0x40,0x05,0x05,0x41,0x05,0x05,0x42,0x05,0x05,0x43,0x05,0x05
,0x44,0x05,0x05,0x45,0x05,0x05,0x46,0x05,0x05,0x47,0x05,0x05,0x48,0x05,0x05,0x49
,0x05,0x05,0x4a,0x05,0x05,0x4b,0x05,0x05,0x4c,0x05,0x05,0x4d,0x05,0x05,0x4e,0x05
,0x05,0x4f,0x05,0x05,0x50,0x05,0x05,0x51,0x05,0x05,0x52,0x05,0x05,0x53,0x05,0x05
,0x54,0x05,0x05,0x55,0x05,0x05,0x56,0x05,0x05,0x57,0x05,0x05,0x58,0x05,0x05,0x59
,0x05,0x05,0x5a,0x05,0x05,0x5b,0x05,0x05,0x5c,0x05,0x05,0x5d,0x05,0x05,0x5e,0x05
,0x05,0x5f,0x05,0x05,0x60,0x05,0x05,0x61,0x05,0x05,0x62,0x05,0x05,0x63,0x05,0x05
,0x64,0x05,0x05,0x65,0x05,0x05,0x66,0x05,0x05,0x67,0x05,0x05,0x68,0x05,0x05,0x69
,0x05,0x05,0x6a,0x05,0x05,0x6b,0x05,0x05,0x6c,0x05,0x05,0x6d,0x05,0x05,0x6e,0x05
,0x05,0x6f,0x05,0x05,0x70,0x05,0x05,0x71,0x05,0x05,0x72,0x05,0x05,0x73,0x05,0x05
,0x74,0x05,0x05,0x75,0x05,0x05}},
{4,22,{0x14,0x83,0x36,0x01
  ,0x00,0x00,0x00,0x00
  ,ZEROSx8,ZEROSx16,ZEROSx32,ZEROSx64,ZEROSx128,ZEROSx256,ZEROSx512}},
}},
{3,22,{0x14,0x83,0x34  // raw inputs
  ,0x00,0x00,0x00,0x00,0x00
  ,ZEROSx8,ZEROSx16,ZEROSx32,ZEROSx64,ZEROSx128,ZEROSx256,ZEROSx512}},
{3,22,{0x14,0x83,0x35  // filtered inputs
  ,0x00,0x00,0x00,0x00,0x00
  ,ZEROSx8,ZEROSx16,ZEROSx32,ZEROSx64,ZEROSx128,ZEROSx256,ZEROSx512}},
{0,0,{0}}
};
#endif

static void mdmpv(const u_int8 *cp,size_t sz,const char *mp)
{
  const u_int8 *tcp;
  u_int8 zero=0;
  int32 res;
  size_t i;
  fitPrint(VERBOSE,"\n%s\n",mp);

  tcp=cp;
  for(i=0;i<sz;i++)
  {
    if ((i%32u) == 0u)
    {
      fitPrint(VERBOSE,"%8.8x: ",i);
    }
    else if ((i%16u) == 0u)
    {
      fitPrint(VERBOSE," ");
    }
    else if ((i%8u) == 0u)
    {
      fitPrint(VERBOSE," ");
    }
    else
    {
      //do nothing
    }

    res = memcmp(tcp,&zero,1);

    if ((res != 0) || (i == MONITOR_REG)) // register 14 plus 3 byte header
    {
      fitPrint(VERBOSE,"\x1b" "[7m%2.2x\x1b" "[m",*tcp);
    }
    else
    {
      fitPrint(VERBOSE,"%2.2x",*tcp);
    }

    tcp++;
  }
  fitPrint(VERBOSE,"\n");
}

static ftRet_t txFio(const u_int8 *tx_cp,size_t sz,struct timespec *tx_tp)
{
  ssize_t bCnt;
  ftRet_t ftRet = ftPass;

  mdmpv(tx_cp,sz,"Transmit buffer");

  /*
  * Transmit
  */
  (void) clock_gettime(CLOCK_REALTIME, tx_tp);
  bCnt = write(sp5s_fd,tx_cp,sz);
  if(bCnt == -1)
  {
    fitPrint(ERROR,"%s: cannot write from %s, err %d, fd %ld, sz %4.4d, %s\n",
             __func__,devName,errno,sp5s_fd,3,strerror(errno));
    ftRet = ftTxFail;
  }
  else
  {
    if((size_t)bCnt != sz)
    {
      fitPrint(ERROR,"%s: %s returned write byte count %d, expected %u\n",
               __func__,devName,bCnt,sz);
      ftRet = ftTxFail;
    }
  }

  return(ftRet);
}

static ftRet_t rxFio(u_int8 *rx_cp,size_t sz, struct timespec *rx_tp)
{
  int32   reTry = 0;
  int32   retval;
  ssize_t bCnt;
  size_t  szRem;
  fd_set  rfds;
  struct  timeval tv;
  u_int8 *cp = rx_cp;

  szRem = sz;
  FD_ZERO(&rfds);
  FD_SET(sp5s_fd,&rfds);

  memset(rx_cp,0,sz); // clear receive buffer

  // receive timeout interval
  tv.tv_sec = RXTMO;
  tv.tv_usec = 0;

eagain2:
  retval = select(sp5s_fd + 1,&rfds,NULL,NULL,&tv);

  if(retval < 0)
  {
    // Select error
    fitPrint(VERBOSE, "Receiver %s select error %ld\n",devName,retval);
    return(ftRxError);
  }

  if(retval == 0)
  {
    // Select timed out
    fitPrint(VERBOSE, "select timed out for %s\n",devName);
    return(ftRxTimeout);
  }

  bCnt = read(sp5s_fd,cp,szRem);
  if(bCnt == -1)
  {
    if(EAGAIN == errno)
    {
      if(++reTry > RETRY_COUNT)
      {
        fitPrint(VERBOSE, "nothing to read for %s, err %d\n",devName,errno);
      }
      else
      {
        goto eagain2;
      }
    }
    fitPrint(VERBOSE,"%s: cannot read from %s, err %d, %s, sz %4.4u\n",
             __func__,devName,errno,strerror(errno),szRem);
  }
  else
  {
    if((size_t)bCnt != szRem)
    {
      fitPrint(VERBOSE,"%s: %s returned read byte count %d, expected %u\n",
               __func__,devName,bCnt,szRem);

      /*
      * Check for partial successful read. Update request and pointers.
      */
      if(((size_t)bCnt < szRem) && (bCnt != 0))
      {
        szRem  -= (size_t)bCnt;
        //cp += bCnt;
        cp = &cp[bCnt];
        reTry = 0;
        goto eagain2; // try to read the rest of the buffer
      }
    }
    else
    {
      /*
      * Received a frame, check payload
      */
      (void) clock_gettime(CLOCK_REALTIME, rx_tp);
      mdmpv(rx_cp,sz,"Receive buffer");

      return(ftPass);
    }
  }

  fitPrint(ERROR,"%s: unexpected return\n",__func__);

  return(ftRxError);
}

static void usage(char * cp)
{
  const char *us = "not implemented!";

  fitPrint(USER,  "usage: %s %s\n",cp,us);
  fitLicense();
  exit(-1);
}

/*
* Run an FIO command
*/
static ftRet_t runFioCmd(const fioComm_t *fioCp)
{
  ftRet_t ftRet;
  struct timespec tx_t;
  struct timespec rx_t;
  double transTime;


  /*
  * Transmit
  */
  ftRet = txFio(fioCp->txCmd,fioCp->txsz,&tx_t);
  if(ftRet != ftPass)
  {
    fitPrint(ERROR,"%s: cannot transmit %4.4u bytes\n",__func__,fioCp->rxsz);
  }
  else
  {
    transTime = (double)tx_t.tv_sec + ((double)tx_t.tv_nsec / 1.0e9);
    /*
    * Receive
    */
    ftRet = rxFio(rxBuf,fioCp->rxsz,&rx_t);
    if(ftRet != ftPass)
    {
      fitPrint(ERROR,"%s: cannot read %4.4u bytes\n",__func__,fioCp->rxsz);
    }
    else
    {
      transTime = (double)rx_t.tv_sec + ((double)rx_t.tv_nsec / 1.0e9);
      transTime -= (double)tx_t.tv_sec + ((double)tx_t.tv_nsec / 1.0e9);
      fitPrint(VERBOSE,"Transaction time = %f sec.\n",transTime);
    }
  }

  return(ftRet);
}

#if 0
/*
* Run a sequence of FIO commands
*/
static ftRet_t runFioSeq(fioComm_t *fioCp)
{
  ftRet_t ftRet = ftPass;

  for(;fioCp->txsz != (size_t)0;fioCp++)
  {
    if((ftRet = runFioCmd(fioCp)) != ftPass)
    {
      fitPrint(ERROR,"%s: cannot read %4.4d bytes\n",__func__,fioCp->rxsz);
      break;
    }
  }

  return(ftRet);
}
#endif

/*
* get FIO module ID
*/
static ftRet_t getModuleId(void)
{
  ftRet_t ftRet;

  fitPrint(VERBOSE,"====================\nGet Module Id\n");
  ftRet = runFioCmd(getModuleIdCmd);

  if(ftRet == ftPass)
  {
    modId = rxBuf[getModuleIdCmd[0].rxsz - 1u];

    switch (modId)
    {
      case 1: // 2070-2A/E/E+
        fitPrint(USER,"%s: FIO type %d - 2070-2\n",__func__,modId);
        break;
      case 2: // 2070-8/NEMA TS 1 or TS 2-Type 2 (A, B, C and D conn)
        fitPrint(USER,"%s: FIO type %d - 2070-8\n",__func__,modId);
        break;
      case 3: // 2070-2N/NEMA TS 2-Type 1
        fitPrint(USER,"%s: FIO type %d - 2070-2N\n",__func__,modId);
        break;
      case 4: // NEMA TS 1 or TS 2-Type 2 (A, B, C conn only)
        fitPrint(USER,"%s: FIO type %d - A-B-C conn only\n",__func__,modId);
        break;
      case 5: // NEMA TS 1 or TS 2-Type 2 (A, B, C and custom conn)
        fitPrint(USER,"%s: FIO type %d - A-B-C + custom conn\n",__func__,modId);
        break;
      case 6: // Custom
        fitPrint(USER,"%s: FIO type %d - Custom device\n",__func__,modId);
        break;
      default:
        fitPrint(ERROR,"%s: FIO type %d - not recognized\n",__func__,modId);
        ftRet = ftError;
        break;
    }
  }

  return(ftRet);
}

/*
*
* get monitor byte containing voltage monitor and fault monitor bits
* loop forever until the following transition occurs:
*
* voltage monitor asserts when bit 7 goes from high to low
*
* fault monitor asserts when bit 6 goes from low to high
*
*/
static ftRet_t getRawInputs(void)
{
  u_int8 normal_monitor = 0x80, curr_monitor; // normal bits
  ftRet_t ftRet;

  fitPrint(VERBOSE,"====================\nGet Raw Inputs\n");

    switch (modId)
    {
      case 1: // 2070-2A/E/E+
        ftRet = runFioCmd(getRawInputsCmd2);
        break;
      case 2: // 2070-8/NEMA TS 1 or TS 2-Type 2 (A, B, C and D conn)
      case 3: // 2070-2N/NEMA TS 2-Type 1
        ftRet = runFioCmd(getRawInputsCmd8);
        break;
      case 4: // NEMA TS 1 or TS 2-Type 2 (A, B, C conn only)
      case 5: // NEMA TS 1 or TS 2-Type 2 (A, B, C and custom conn)
      case 6: // Custom
      default:
        ftRet = ftPass; // don't attempt to read imputs; module ID is enough
        break;
    }

  if(ftRet != ftPass)
  {
    return(ftRet);
  }

  curr_monitor = rxBuf[MONITOR_REG];
  fitPrint(ERROR,"%s: normal monitor 0x%2.2x, current monitor 0x%2.2x\n",
             __func__,normal_monitor,curr_monitor);

  if(normal_monitor != curr_monitor)
  {
    fitPrint(ERROR,"%s: monitor bits have changed 0x%2.2x\n",
             __func__,curr_monitor);
  }

  return(ftRet);
}

/*
* Monitor entry
*/
ftRet_t fioMonitorFit(plint argc, char * const argv[])
{
  //u_int8 moduleId = 0xff, monitorReg = 0xff;
  int32 idx;
  int32 c;
  ftRet_t ftRet;
  ftResults_t ftResults = {0};
  ftResults_t *ftrp = &ftResults;

  int32       sched_err;
  plint       old_policy;
  pid_t       kernel_pid;
  pthread_t   pthread_id;
  struct sched_param proc_param;



  /*
  * Parse command line arguments
  */
  opterr = 0;
  c = getopt(argc,argv,"h");

  while(c != -1)
  {
    if ((char)c != 'h')
    {
      if(isprint(optopt))
      {
        fitPrint(ERROR, "unknown option `-%c`.\n",optopt);
      }
      else
      {
        fitPrint(ERROR, "unknown option character `\\x%X`.\n",optopt);
      }
    }

    usage(argv[0]);
    c = getopt(argc,argv,"h");
  }

  for(idx = optind; idx < argc; idx++)
  {
    fitPrint(ERROR, "Non-option argument %s\n",argv[idx]);
    usage(argv[0]);
  }

  /*
  * Open special device
  */
  //lint -e{9027} MISRA hates bitwise ops on signed values, but can't be helped
  sp5s_fd = open(devName,O_RDWR|O_NONBLOCK);

  if(sp5s_fd == -1)
  {
    fitPrint(ERROR,"%s: cannot open %s, err %d, %s\n",
             __func__,devName,errno,strerror(errno));
    ftUpdateTestStatus(ftrp,ftError,NULL);
    return(ftUpdateTestStatus(ftrp,ftComplete,"open error"));
  }

  /*
  * Boost priority to Round Robin 1 for better times
  */

  kernel_pid = getpid ();
  pthread_id = pthread_self ();
  (void) pthread_getschedparam (pthread_id, &old_policy, &proc_param);
  proc_param.sched_priority = 1;
  sched_err = pthread_setschedparam (pthread_id, SCHED_RR, &proc_param);
  fitPrint(VERBOSE,"Set proc %d to Round Robin priority %d, err=%ld\n",
           kernel_pid, proc_param.sched_priority, sched_err);
  sched_yield();

  ftRet = getModuleId();

  if(ftRet != ftPass)
  {
    ftUpdateTestStatus(ftrp,ftRet,NULL);
    return(ftUpdateTestStatus(ftrp,ftComplete,"getModuleId failed"));
  }

  sched_yield();
  ftRet = getRawInputs();

  if(ftRet != ftPass)
  {
    ftUpdateTestStatus(ftrp,ftRet,NULL);
    return(ftUpdateTestStatus(ftrp,ftComplete,"getRawInputs failed"));
  }

#if 0
  for(fioCp = fioCommSequence;fioCp->txsz != (size_t)0;fioCp++)
  {
    memset(rxBuf,0,sizeof(rxBuf)); // clear receive buffer

    /*
    * Transmit
    */
    if(txFio(devName,sp5s_fd,fioCp->txCmd,fioCp->txsz) != ftPass)
    {
      fitPrint(ERROR, "cannot transmit from %s, err %u, fd %d, sz %4.4d, %s\n",
               devName,errno,sp5s_fd,fioCp->txsz,strerror(errno));
      break;
    }

    /*
    * Receive
    */
    if(rxFio(devName,sp5s_fd,rxBuf,fioCp->rxsz) != ftPass)
    {
      fitPrint(ERROR, "cannot read from %s, err %u, fd %d, sz %4.4d, %s\n",
               devName,errno,sp5s_fd,fioCp->rxsz,strerror(errno));
      break;
    }
  }
#endif

  close(sp5s_fd);

  ftUpdateTestStatus(ftrp,ftRet,NULL);
  return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
}
