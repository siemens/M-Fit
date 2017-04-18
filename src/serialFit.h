/******************************************************************************
                                   seriaFit.h

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

//lint -library
#define ASYNC_MAX_FRAME_SZ_1200 4
#define ASYNC_MAX_FRAME_SZ 32
#define ONE 1
#define MAX_WAIT_FOR_TX 500
#define MAX_TRANSFER 1024u // 1022 is the real maximum for SYNC, 32 for ASYNC
#define RETRY_CNT 2

#define ATC_SPXS_WRITE_CONFIG 0 // spx.h
#define ATC_SPXS_READ_CONFIG 1  // spx.h

typedef struct _user_spx_ioctl {
  u_int32 x[20]; // command data
} user_spx_ioctl_t;

typedef struct _spx_ioctl_t {
  u_int32 protocol;
  u_int32 baud;
  u_int32 transmit_clock_source;
  u_int32 transmit_clock_mode;
} spx_ioctl_t;

typedef struct atc_spxs_config {
  u_int8 protocol;
  u_int8 baud;
  u_int8 transmit_clock_source;
  u_int8 transmit_clock_mode;
} atc_spxs_config_t;

/*
* Current state of the thread
*/
typedef enum _spState {
  waitingForMon,
  waitingForRx,
  waitingForConcurrent,
  transmitting,
  transmitComplete,
  receiving,
  receiveComplete,
  scheduled,
  unscheduled
} spState_t;

/*
* Identify thread
*/
typedef enum _xfer {
  receiver,
  transmitter
} xfer_t;

/*
* Expected protocol
*/
typedef enum _spProt {
  protAsync,
  protSync,
  protNone
} spProt_t;

static const u_int32 ATC_B[] = {
        1200u,
        2400u,
        4800u,
        9600u,
        19200u,
        38400u,
        57600u,
        76800u,
        115200u,
        153600u,
        614400u
};

typedef struct {
  struct timespec start;
  struct timespec end;
} bRateMsmnt_t;

typedef struct _bRate {
  speed_t       baudRate;
  u_int32       rxOK; // communication statistics
  u_int32       rxNG;
  u_int32       txOK;
  u_int32       txNG;
  bRateMsmnt_t *brmp; // baud rate measurements pointer
} bRate_t;

/*
* Serial Port Test Data Description
*/
typedef struct _spDatDat {
  const char    *testName;                     // test name
  const char    *devName_r;                    // recv device specification
  const char    *devName_w;                    // txmt device specification
  sem_t         *mp, *mp_r, *mp_w, *spLock;    // pointers to semaphores
  pthread_t     *ptp_r, *ptp_w;                // pointers to thread info
  char          *buf_r, *buf_w;                // pointers to read & write buffers
  size_t        comSz;                         // size of the transmission
  size_t        minFrameSize, maxFrameSize;    // size range of the transmissions
  u_int32       iter;                          // iterations of the tests
  spProt_t      protocol;                      // expected protocol
  int32         fd_r, fd_w;                    // device descriptors
  spState_t     spSt_r, spSt_w;                // state of reader and writer
  bRate_t       *brp, *lbrp;                   // baud rate table pointers
} spDatDat_t;

extern void initSerialPort(int32 fd,speed_t baudRate,spProt_t protocol,bool flowCntrl);
