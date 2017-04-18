/******************************************************************************
                                serialEchoFit.c

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
#define RXTMO  10 // receive timeout in seconds
#define TXNUM   2 // frames to transmit
#define COMSZ  32 // byte count
#define MAX_COMM_SZ 1024u
#define DEV_NAME_SZ 32
#define TXDEV "/dev/sp8"
#define RXDEV "/dev/sp8"

typedef struct _comm_t
{
  u_char buf_r[MAX_COMM_SZ];
  u_char buf_w[MAX_COMM_SZ];
  char   devName_r[DEV_NAME_SZ];
  char   devName_w[DEV_NAME_SZ];
  u_int32 iter;
  size_t comSz;
} comm_t;

static comm_t comm_data;
static comm_t *commp = &comm_data;

static ftResults_t ftResults = {0};
static ftResults_t *ftrp = &ftResults;

static int32 fd_r,fd_w;               // ville descriptors for read / write
static bool  flowControlFlag = false; // default flow control to disabled

static spProt_t spProtocol = protNone;
static speed_t  spBaudRate = 0;

static ftRet_t rxTypeTwo (char *rx_name,int32 fd_rx,u_char *rx_ucp,
                          char *tx_name,u_char *tx_ucp,size_t sz)
{
  int32   reTry = 0;
  ssize_t bCnt;
  size_t  i = sz;
  int32   retval;
  int32   result;
  fd_set  rfds;
  struct timeval tv;
  u_char *ucp = rx_ucp;

  FD_ZERO(&rfds);
  FD_SET(fd_rx, &rfds);

  // receive timeout interval
  tv.tv_sec = RXTMO;
  tv.tv_usec = 0;

eagain2:
  retval = select(fd_r + 1,&rfds,NULL,NULL,&tv);

  if (retval < 0)
  {
    // Select error
    fitPrint(VERBOSE, "Receiver %s select error %ld\n",rx_name,retval);
    return(ftRxError);
  }

  if (retval == 0)
  {
    // Select timed out
    fitPrint(VERBOSE, "select timed out for %s\n",rx_name);
    return(ftRxTimeout);
  }

  bCnt = read(fd_rx,ucp,i);

  if (bCnt == -1)
  {
    if (EAGAIN == errno)
    {
      if (++reTry > RETRY_COUNT)
      {
        fitPrint(VERBOSE, "nothing to read for %s, err %d\n",rx_name,errno);
      }
      else
      {
        goto eagain2;
      }
    }
    fitPrint(VERBOSE, "cannot read from %s, err %d, %s, sz %4.4u\n",
             rx_name,errno,strerror(errno),i);
  }
  else
  {
    if ((size_t)bCnt != i)
    {
      fitPrint(VERBOSE, "%s returned read byte count %d, expected %u\n",rx_name,bCnt,i);

      /*
      * Check for partial successful read. Update request and pointers.
      */
      if (((size_t)bCnt < i) && (bCnt != 0))
      {
        i -= (size_t)bCnt;
        ucp = &ucp[bCnt];
        reTry = 0;
        goto eagain2; // try to read the rest of the buffer
      }
    }
    else
    {
      /*
      * Received a frame, check payload
      */
      result = memcmp (rx_ucp,tx_ucp,sz);

      if (result != 0)
      {
        /*
        * Something is wrong with the received payload
        */
        fitPrint(VERBOSE, "frame payload problem %s->%s\n",tx_name,rx_name);
        fitPrint(VERBOSE, "expected buffer (%p):",tx_ucp);
        mdmp(tx_ucp,sz,VERBOSE);
        fitPrint(VERBOSE, "\nactual buffer   (%p):",rx_ucp);
        mdmp(rx_ucp,sz,VERBOSE);
        return(ftRxFail);
      }
      else
      {
        /*
        * Frame has been successfully transmitted and received
        */
        return(ftPass);
      }
    }
  }
  fitPrint(ERROR, "%s: unexpected return\n",__func__);
  return(ftRxError);
}

static speed_t mapBaudTermios(u_int32 baud)
{
  speed_t ret;

  switch(baud)
  {
    case 1200:
      ret = (B1200);
      break;
    case 2400:
      ret = (B2400);
      break;
    case 4800:
      ret = (B4800);
      break;
    case 9600:
      ret = (B9600);
      break;
    case 19200:
      ret = (B19200);
      break;
    case 38400:
      ret = (B38400);
      break;
    case 57600:
      ret = (B57600);
      break;
    case 115200:
      ret = (B115200);
      break;
    default:
      // 153600 sdlc
      // 614400 sdlc
      ret = ((speed_t)baud);
      break;
  }
  return ret;
}


static void usage(char * cp)
{
  const char *us =
"\
This software tests the MPC82xx and MPC8309 serial ports. Asynchronous and\n\
synchronous protocols are supported at standard baud rates.\n\
\n\
The test requires a point to point serial connection topology, a minimum\n\
and maximum payload size that defines the range of packet sizes being tested\n\
and the number of test iterations to be run. This test ignores the minimum\n\
packet size specification and sends a single packet at the maximum size.\n\
\n\
A configuration file is scanned to communicate the expected connections.\n\
This is an ascii file in the following format:\n\
\n\
recieve_port transmit_port min_payload_size max_payload_size test_iteration\n\
\n\
A typical file might look like:\n\
\n\
sp2 sp1 1 32 10\n\
\n\
A single line is scanned. The test executes in a single user space process.\n\
In this case, a single packet of size 32 bytes is transmitted from serial\n\
port 1 to serial port 2. Upon successful reception at serial port 2, the\n\
packet is then turned around and re-transmitted from serial port 2 to\n\
serial port 1. This operation will iterate 9 more times.\n\
\n\
The arguments and functionality are defined as:\n\
\t-f hardware signals for RTS/CTS/DCD are used to implement flow control.\n\
\t-b locks all testing to a single standard baud rate .e.g 38400.\n\
\t   If no baud rate is specified, then asynchronous ports will default\n\
\t   to 115200\n\
\t   and synchronous ports will default to 153600 baud.\n\
\t-i specifies the number of overall test iterations.\n\
\t-c configuration file name\n\
\t-h this help\n\
";
  fitPrint(USER,  "usage: %s [fb[baud rate]i[iteration]c[config file]h]\n%s\n",cp,us);
  fitLicense();
  exit(0);
}

ftRet_t serialEchoFit(plint argc, char * const argv[])
{
  int32    idx,c,unused_min_size;
  ssize_t  bCnt;
  u_int32  i;
  ftRet_t  ftResult;
  char lcl_devName_r[DEV_NAME_SZ],lcl_devName_w[DEV_NAME_SZ];
  FILE *fp;

  /*
  * Set defaults
  */
  memset((char *)commp,0,sizeof(comm_t));
  strcpy(commp->devName_r,RXDEV);
  strcpy(commp->devName_w,TXDEV);
  commp->comSz = COMSZ;
  commp->iter = TXNUM;

  /*
  * Parse command line arguments
  */
  opterr = 0;
  c = getopt(argc,argv,"fc:i:b:h");

  while(c != -1)
  {
    switch((char)c)
    {
      default:
        if(isprint(optopt))
        {
          fitPrint(ERROR, "unknown option `-%c`.\n",optopt);
        }
        else
        {
          fitPrint(ERROR, "unknown option character `\\x%X`.\n",optopt);
        }
        usage(argv[0]);
        break;
      case 'h':
        usage(argv[0]);
        break;
      case 'c': // configuration file name
        fp = fopen(optarg,"r");
        if(fp == NULL)
        {
          fitPrint(ERROR, "cannot open config file %s\n",optarg);
          ftUpdateTestStatus(ftrp,ftError,NULL);
          return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
        }

        fscanf(fp,"%s %s %ld %u %lu",
               lcl_devName_r,lcl_devName_w,&unused_min_size,&commp->comSz,&commp->iter);

        if(strlen(lcl_devName_r) == 4u)
        { // sp[12358]s
          // synchronous
          spProtocol = protSync;
          if (spBaudRate == 0u)
          {
            spBaudRate = 153600;
          }
        }
        else
        { // sp[123468]
          // asynchronous
          spProtocol = protAsync;
          if (spBaudRate == 0u)
          {
            spBaudRate = B115200;
          }
        }

        strcpy(commp->devName_r,"/dev/");
        strcat(commp->devName_r,lcl_devName_r);

        strcpy(commp->devName_w,"/dev/");
        strcat(commp->devName_w,lcl_devName_w);

        if (commp->comSz > MAX_COMM_SZ)
        {
           commp->comSz = MAX_COMM_SZ;
        }

        fclose(fp);
        break;
      case 'i': // number of test iterations
        commp->iter = strtoul(optarg,NULL,10);
        break;
      case 'f': // enable flow control
        flowControlFlag = true;
        break;
      case 'b': // run test at specified rate
        spBaudRate = strtoul(optarg,NULL,10);
        if (spProtocol == protAsync)
        {
          spBaudRate = mapBaudTermios(spBaudRate);
        }
        break;

    }

    c = getopt(argc,argv,"fc:i:b:h");
  }

  for(idx = optind; idx < argc; idx++)
  {
    fitPrint(ERROR, "Non-option argument %s\n",argv[idx]);
    usage(argv[0]);
  }

  /*
  * Open special devices
  */
  if(strcmp(commp->devName_r,commp->devName_w) == 0)
  {
    fd_w = open(commp->devName_w,O_RDWR);

    if(fd_w == -1)
    {
      fitPrint(ERROR, "%s: cannot open %s, err %d, %s\n",
               __func__,commp->devName_w,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
      return(ftUpdateTestStatus(ftrp,ftComplete,commp->devName_w));
    }
    fd_r = fd_w;

    initSerialPort(fd_r, spBaudRate, spProtocol, flowControlFlag);
  }
  else
  {
    fd_w = open(commp->devName_w,O_RDWR);

    if(fd_w == -1)
    {
      fitPrint(ERROR, "%s: cannot open transmitter %s, err %d, %s\n",
               __func__,commp->devName_w,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftTxError,NULL);
      return(ftUpdateTestStatus(ftrp,ftComplete,commp->devName_w));
    }

    fd_r = open(commp->devName_r,O_RDWR);

    if(fd_r == -1)
    {
      fitPrint(ERROR, "%s: cannot open receiver %s, err %d, %s\n",
               __func__,commp->devName_r,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftRxError,NULL);
      return(ftUpdateTestStatus(ftrp,ftComplete,commp->devName_r));
    }

    initSerialPort(fd_r, spBaudRate, spProtocol, flowControlFlag);
    initSerialPort(fd_w, spBaudRate, spProtocol, flowControlFlag);
  }

  for(i=0;i < commp->iter;i++)
  {
    memset(commp->buf_r,0,commp->comSz);            // initialize receive buffer to pattern
    memset(commp->buf_w,(int32)commp->comSz,commp->comSz); // iniz xmit buffer to packet size

    /*
    * Transmit
    */
    bCnt = write(fd_w,commp->buf_w,commp->comSz);

    if(bCnt == -1)
    {
      fitPrint(ERROR, "cannot write from %s, err %d, fd %ld, sz %4.4u, %s\n",
               commp->devName_w,errno,fd_w,commp->comSz,strerror(errno));
      ftUpdateTestStatus(ftrp,ftTxError,commp->devName_w);
      break;
    }
    else
    {
      if((size_t)bCnt != commp->comSz)
      {
        fitPrint(ERROR, "%s returned write byte count %d, expected %u\n",
                 commp->devName_w,bCnt,commp->comSz);
        ftUpdateTestStatus(ftrp,ftTxFail,commp->devName_w);
        break;
      }
    }
    fitPrint(VERBOSE, "%s->%s\n",commp->devName_w,commp->devName_r);

    /*
    * Receive
    */
    ftResult = rxTypeTwo(commp->devName_r,fd_r,commp->buf_r,
                         commp->devName_w,commp->buf_w,commp->comSz);
    ftResult = ftUpdateTestStatus(ftrp,ftResult,commp->devName_r);
    if(ftResult == ftPass)
    {
      /*
      * reinitialize buffers and transmit back
      * note: the _r and _w convention for receive and transmit respectively is now reversed
      * consider refactoring this block
      */
      memset(commp->buf_w,0,commp->comSz);            // initialize receive buffer to pattern
      memset(commp->buf_r,(int32)commp->comSz,commp->comSz); // initialize transmit buffer to packet size

      /*
      * Transmit
      */
      bCnt = write(fd_r,commp->buf_r,commp->comSz);

      if(bCnt == -1)
      {
        fitPrint(ERROR, "cannot write from %s, err %d, fd %ld, sz %4.4u, %s\n",
                 commp->devName_r,errno,fd_r,commp->comSz,strerror(errno));
        ftUpdateTestStatus(ftrp,ftTxError,commp->devName_r);
        break;
      }
      else
      {
        if((size_t)bCnt != commp->comSz)
        {
          fitPrint(ERROR, "%s returned write byte count %d, expected %u\n",
                   commp->devName_r,bCnt,commp->comSz);
          ftUpdateTestStatus(ftrp,ftTxFail,commp->devName_r);
          break;
        }
      }
      fitPrint(VERBOSE, "%s->%s\n",commp->devName_r,commp->devName_w);

      /*
      * Receive
      */
      ftResult = rxTypeTwo(commp->devName_w,fd_w,commp->buf_w,commp->devName_r,commp->buf_r,commp->comSz);
      ftUpdateTestStatus(ftrp,ftResult,commp->devName_w);
    }
  }

  close(fd_r);
  close(fd_w);

  return(ftUpdateTestStatus(ftrp,ftComplete,commp->devName_w));
}

