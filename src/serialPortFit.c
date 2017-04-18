/******************************************************************************
                                 serialPortFit.c

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
#include <sys/select.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <semaphore.h>
#include <sys/stat.h>

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
  char    buf_r[MAX_COMM_SZ];
  char    buf_w[MAX_COMM_SZ];
  char    devName_r[DEV_NAME_SZ];
  char    devName_w[DEV_NAME_SZ];
  u_int32 iter;
  size_t  comSz;
} comm_t;

static comm_t comm_data;
static comm_t *commp = &comm_data;

static ftResults_t ftResults = {0};
static ftResults_t *ftrp = &ftResults;

static int32 fd_r,fd_w;

static spProt_t spProtocol = protNone;
static speed_t spBaudRate;


static void usage(char * cp)
{
  const char *us =
"\
This software tests the MPC82xx and MPC8309 serial ports with a simple test\n\
at the default baud rate only. Asynchronous and synchronous protocols are\n\
supported at\n\
\n\
The test requires a point to point serial connection topology, a payload size\n\
and the number of test iterations to be run. This test ignores the minimum\n\
packet size specification and sends all packets at the maximum size.\n\
\n\
A configuration file is scanned to communicate the expected connections.\n\
This is an ascii file in the following format:\n\
\n\
recieve_port transmit_port min_payload_size max_payload_size test_iterations\n\
\n\
A typical file might look like:\n\
\n\
sp2 sp1 1 32 10\n\
\n\
The arguments and functionality are defined as:\n\
\t-i specifies the number of overall test iterations.\n\
\t-c configuration file name\n\
\t-h this help\n\
";
  fitPrint(USER,  "usage: %s [i[iteration]c[config file]h]\n%s\n",cp,us);
  fitLicense();
  exit(0);
}

static ftRet_t rxTypeTwo (void)
{
  int32   reTry = 0;
  int32   retval;
  ssize_t bCnt;
  size_t  tempsize;
  fd_set  rfds;
  struct timeval tv;
  char *cp = commp->buf_r;

  tempsize = commp->comSz;
  bCnt = -1;
  FD_ZERO(&rfds);

  // receive timeout interval
  tv.tv_sec = RXTMO;
  tv.tv_usec = 0;

eagain2:

  errno = 0;
  FD_SET(fd_r, &rfds);
  retval = select(fd_r + 1,&rfds,NULL,NULL,&tv);

  if(retval < 0)
  {
    // Select timed out or error
    fitPrint(VERBOSE, "Receiver %s select failed %ld, err = %d\n",
             commp->devName_r,retval,errno);
    return(ftRxError);
  }
  else if (retval == 0)
  {
    // Select timed out
    fitPrint(VERBOSE, "select timed out for %s\n",commp->devName_r);
    return(ftRxTimeout);
  }
  else  // retval > 0
  {
    bCnt = read(fd_r,cp,tempsize);

    if (bCnt == -1)
    {
      if (EAGAIN == errno)
      {
        if (++reTry > RETRY_COUNT)
        {
          fitPrint(VERBOSE, "nothing to read for %s, err %d\n",
                   commp->buf_r,errno);
        }
        else
        {
          goto eagain2;
        }
      }
      fitPrint(VERBOSE, "cannot read from %s: %s, sz %4.4u\n",
               commp->devName_r,strerror(errno),commp->comSz);
    }
    else if ((size_t)bCnt != tempsize)
    {
      fitPrint(VERBOSE, "%s returned read byte count %d, expected %u\n",
               commp->devName_r,bCnt,tempsize);

      /*
      * Check for partial successful read. Update request and pointers.
      */
      if (((size_t)bCnt < tempsize) && (bCnt!=0))
      {
        tempsize -= (size_t)bCnt;
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
      if (memcmp (commp->buf_r,commp->buf_w,commp->comSz) != 0)
      {
        /*
        * Something is wrong with the received payload
        */
        fitPrint(VERBOSE, "frame payload problem\n");
        fitPrint(VERBOSE, "expected buffer (%p):",commp->buf_w);
        mdmp(commp->buf_w,commp->comSz,VERBOSE);
        fitPrint(VERBOSE, "\nactual buffer   (%p):",commp->buf_r);
        mdmp(commp->buf_r,commp->comSz,VERBOSE);
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

ftRet_t serialPortFit(plint argc, char *const argv[])
{
  u_int32 i;
  int32   bCnt,idx,c;
  u_int32 unused_min_size;
  char    lcl_devName_r[DEV_NAME_SZ],lcl_devName_w[DEV_NAME_SZ];
  FILE    *fp;

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
  c = getopt(argc,argv,"c:i:h");

  while(c != -1){
    switch((char)c){
      default:
        if(isprint(optopt)){
          fitPrint(ERROR, "unknown option `-%c`.\n",optopt);
        } else {
          fitPrint(ERROR, "unknown option character `\\X%X`.\n",optopt);
        }
        usage(argv[0]);
        break;
      case 'h':
        usage(argv[0]);
        break;
      case 'c': // configuration file name
        fp = fopen(optarg,"r");

        if(fp == NULL){
          fitPrint(ERROR, "cannot open config file %s\n",optarg);
          ftUpdateTestStatus(ftrp,ftError,NULL);
          return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
        }

        fscanf(fp,"%s %s %lu %u %lu",
               lcl_devName_r,lcl_devName_w,&unused_min_size,&commp->comSz,&commp->iter);

        if(strlen(lcl_devName_r) == 4u){ // sp[12358]s
          // synchronous
          spProtocol = protSync;
          spBaudRate = 153600;
        } else { // sp[123468]
          // asynchronous
          spProtocol = protAsync;
          spBaudRate = B115200;
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
    } // switch()

    c = getopt(argc,argv,"c:i:h");
  } // while()

  for(idx = optind; idx < argc; idx++){
    fitPrint(ERROR, "Non-option argument %s\n",argv[idx]);
  }

  fitPrint(VERBOSE, "Using config TX %s, RX %s, packet size %u, iteration %lu\n",
           commp->devName_w,commp->devName_r,commp->comSz,commp->iter);

  /*
  * Open special devices
  */
  if(strcmp(commp->devName_r,commp->devName_w) == 0)
  { // loopback: same port for transmit and receive
    fd_w = open(commp->devName_w,O_RDWR);

    if(fd_w == -1){
      fitPrint(ERROR, "%s: cannot open %s, err %d, %s\n",
               __func__,commp->devName_w,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
      return(ftUpdateTestStatus(ftrp,ftComplete,commp->devName_w));
    }
    fd_r = fd_w;

    initSerialPort(fd_r,spBaudRate,spProtocol,false);
  }
  else
  { // port to port: different port for transmit and receive
    fd_w = open(commp->devName_w,O_WRONLY);

    if(fd_w == -1){
      fitPrint(ERROR, "%s: cannot open transmitter %s, err %d, %s\n",
               __func__,commp->devName_w,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftTxError,NULL);
      return(ftUpdateTestStatus(ftrp,ftComplete,commp->devName_w));
    }

    fd_r = open(commp->devName_r,O_RDONLY);

    if(fd_r == -1){
      fitPrint(ERROR, "%s: cannot open receiver %s, err %d, %s\n",
               __func__,commp->devName_r,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftRxError,NULL);
      return(ftUpdateTestStatus(ftrp,ftComplete,commp->devName_r));
    }

    initSerialPort(fd_r,spBaudRate,spProtocol,false);
    initSerialPort(fd_w,spBaudRate,spProtocol,false);
  }

  for(i=0;i < commp->iter;i++){
    // initialize receive buffer
    memset(commp->buf_r,0xff,commp->comSz);
    // initialize transmit buffer to packet size
    memset(commp->buf_w,(plint)commp->comSz,commp->comSz);

    /*
    * Transmit
    */
    bCnt = write(fd_w,commp->buf_w,commp->comSz);

    if(bCnt == -1){
      fitPrint(ERROR, "cannot write from %s, err %d, fd %ld, sz %4.4u, %s\n",
               commp->devName_w,errno,fd_w,commp->comSz,strerror(errno));
      ftUpdateTestStatus(ftrp,ftTxError,NULL);
      break;
    } else {
      if((size_t)bCnt != commp->comSz){
        fitPrint(ERROR, "%s returned write byte count %ld, expected %u\n",
                 commp->devName_w,bCnt,commp->comSz);
        ftUpdateTestStatus(ftrp,ftTxFail,NULL);
        break;
      }
    }
    fitPrint(VERBOSE, "Transmitter %s has sent frame %lu\n",
             commp->devName_w,i + 1u);

    /*
    * Receive
    */
    ftUpdateTestStatus(ftrp,rxTypeTwo(),NULL);

  }

  close(fd_r);
  close(fd_w);

  return(ftUpdateTestStatus(ftrp,ftComplete,commp->devName_w));
}

#if 0
static int32
set_interface_attribs (int32 fd, int32 speed, int32 parity)
{
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0)
  {
    printf("error %d from tcgetattr\n", errno);
    return -1;
  }

  cfsetospeed (&tty, speed);
  cfsetispeed (&tty, speed);

  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
  // disable IGNBRK for mismatched speed tests; otherwise receive break
  // as \000 chars
  tty.c_iflag &= ~IGNBRK;         // disable break processing
  tty.c_lflag = 0;                // no signaling chars, no echo,
                                  // no canonical processing
  tty.c_oflag = 0;                // no remapping, no delays
  tty.c_cc[VMIN]  = 0;            // read doesn't block
  tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

  tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                  // enable reading
  tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
  tty.c_cflag |= parity;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr (fd, TCSANOW, &tty) != 0)
  {
    printf("error %d from tcsetattr\n", errno);
    return -1;
  }
  return 0;
}

static void
set_blocking (int32 fd, int32 should_block)
{
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0)
  {
    printf("error %d from tggetattr\n",errno);
    return;
  }

  tty.c_cc[VMIN]  = should_block ? 1 : 0;
  tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

  if (tcsetattr (fd, TCSANOW, &tty) != 0)
    printf("error %d setting term attributes\n",errno);
}

ftRet_t spRouteFit(plint argc, char * argv[])
{
  const char *portname = "/dev/sp8";
  char buf[1024];
  int32 cnt;
  int32 fd=open(portname,O_RDWR|O_NOCTTY|O_SYNC);

  if (fd < 0)
  {
    printf("error %d opening %s: %s\n",errno,portname,strerror(errno));
    ftUpdateTestStatus(ftrp,ftError,NULL);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  set_interface_attribs (fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
  set_blocking (fd, 1);

  for(;;){
    printf("calling read...\n");
    cnt = read (fd, buf, sizeof buf);
    printf("read complete %d\n",cnt);
    if(cnt){
      //usleep(1024);
      printf("calling write...\n");
      write (fd, buf, cnt);
      printf("write complete\n");
    }
  }
  close(fd);

  ftUpdateTestStatus(ftrp,ftPass,NULL);
  return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
}
#endif

