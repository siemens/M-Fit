/******************************************************************************
                                      fit.h

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
#ifndef FIT_H
  #define FIT_H

  #ifndef _STDBOOL_H
    #include <stdbool.h>
  #endif

  #ifndef FTYPES_H
    #include "ftypes.h"
  #endif

  #define MUST_BE_BIG_ENOUGH 512u

  // Custom error codes for the M-Fit suite
  #define ENOARGS -20
  #define ENOFILE -21
  #define ENOBUF  -22


  #ifndef MIN
    # define MIN(a,b) (((a) < (b)) ? (a) : (b))
  #endif
  #ifndef MAX
    # define MAX(a,b) (((a) > (b)) ? (a) : (b))
  #endif


  typedef enum { ftStart, ftComplete, ftPass, ftFail, ftError, ftSignal,
         ftTxTimeout, ftTxError, ftTxFail, ftRxError, ftRxTimeout, ftRxFail,
         ftNewline, ftInformation, ftUnimplemented, ftInterrupt
  } ftRet_t;

  typedef struct {u_int32
         ftStartCnt, ftCompleteCnt, ftPassCnt, ftFailCnt, ftErrorCnt, ftSignalCnt,
         ftTxErrorCnt, ftTxFailCnt, ftRxErrorCnt, ftRxTimeoutCnt, ftRxFailCnt,
         ftNewlineCnt, ftInformationCnt, ftUnimplementedCnt, ftInterruptCnt;
  } ftResults_t;

  typedef enum { ERROR, VERBOSE, USER, PASSFAIL, LOG_FAILURE, LOG } ftPrintLevels_t;

  typedef enum { ARCH_UNKNOWN, ARCH_82XX, ARCH_83XX } ftArchUnderTest_t;


  extern ftArchUnderTest_t targetARCH;
  extern bool verboseFlag;
  extern bool keepGoing;


  extern ftRet_t datakeyFit(plint argc, char * const argv[]);
  extern ftRet_t displayFit(plint argc, char * const argv[]);
  extern ftRet_t eepromFit(plint argc, char * const argv[]);
  extern ftRet_t ethernetFit(plint argc, char * const argv[]);
  extern ftRet_t fioMonitorFit(plint argc, char * const argv[]);
  extern ftRet_t fsFit(plint argc, char * const argv[], const char * const dirNames[]);
  extern ftRet_t mediaFit(plint argc,char * const argv[], const char * const dirNames[]);
  extern ftRet_t memoryFit(plint argc, char * const argv[]);
  extern ftRet_t powerdownFit(plint argc, char * const argv[]);
  extern ftRet_t rtcFit(plint argc, char * const argv[]);
  extern ftRet_t serialFit(plint argc, char * const argv[]);
  extern ftRet_t serialEchoFit(plint argc, char * const argv[]);
  extern ftRet_t serialPortFit(plint argc, char * const argv[]);
  //extern ftRet_t spRouteFit(plint argc, char * const argv[]);
  extern ftRet_t todFit(plint argc, char * const argv[]);

  extern ftRet_t ftUpdateTestStatus(ftResults_t *ftres,ftRet_t ret,const char *descr);

  extern void *createSharedMemory (const char *fileName, int32 *shmid, size_t mem_size);
  extern void mdmp(const void *vp, size_t sz, ftPrintLevels_t printLevel);
  extern void mdmp1(const void *vp, size_t sz, ftPrintLevels_t printLevel,
                    const char *fp, const char *fn, int32 ln, const char *us);
  extern void dmpargs(plint argc,char * const argv[],char *cp);
  extern u_int16 genCrc(const void *saddr, const void *eaddr);


  extern void fitPrint(ftPrintLevels_t printLevel, const char *format, ...);
  extern void fitLicense(void);
  extern int32 readTimeout(int32 fd, void *buf, size_t nbytes, int32 timeoutSeconds);

#endif // FIT_H
