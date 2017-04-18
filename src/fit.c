/******************************************************************************

                                    M-Fit
       A suite of test programs for Linux-based ATC and TEES controllers
                Copyright (c) 2015-2017 Siemens Industry, Inc

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

    An electronic copy of the License is also available at
        www.gnu.org/licenses/gpl-2.0.html

    Original authors:
      Jack McCarthy, Andrew Valdez and Jonathan Grant, Siemens Industry, Inc.


*******************************************************************************/

#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h> /* POSIX Threads */
#include <semaphore.h>
#include <libgen.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/mount.h> //
#include <sys/stat.h>
#include <sys/shm.h>
#include <dirent.h> //
#include <net/if.h>
#include "fit.h"

#define SHM_PERM (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#define MAX_RECURS_DEPTH 20

extern size_t strnlen(const char *s, size_t maxlen);

bool   keepGoing = true;
bool   verboseFlag = false;

ftArchUnderTest_t targetARCH = ARCH_UNKNOWN;

static const char *ftVersion = "2.6.2";
static const char *copyright = "Copyright (c) 2015-2017, Siemens Industry, Inc.";

static const char *failLogFilename = "fitFail.log";
static const char *dirList_ATC[]  = {"/home","/opt","/var","/tmp","/sram",""};
static const char *dirList_82xx[] = {"/home","/opt","/var","/tmp","/r0",""};

static const char *sdNames_ATC[]  = {"/media/sd",""};
static const char *sdNames_82xx[] = {"/mnt/sd",""};
static const char *sdNames_none[] = {""};

static const char *usbNames_ATC[]  = {"/media/usb",""};
static const char *usbNames_82xx[] = {"/mnt/usb",""};
static const char *usbNames_none[] = {""};

static bool   logfileFlag = false;
static bool   logFailureFileFlag = false;

static FILE *ftLogfp = NULL;
static FILE *ftFailLogfp = NULL;

static ftResults_t ftResults = {0};
static ftResults_t *ftrp = &ftResults;

static char ftTestName[MUST_BE_BIG_ENOUGH];

/* CRC values per RFC 1662 */
static const u_int16 fcstab[256] =
{
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};


static ftArchUnderTest_t getTargetArch(void)
{
  u_int32  fLen;
  char     *buf;
  FILE     *fp;
  ftArchUnderTest_t ret = ARCH_UNKNOWN;
  const char  *fn = "/proc/device-tree/compatible";
  const char  arch82xx[] = { // fsl,eb8270\0\0
    '\x66','\x73','\x6c','\x2c','\x65','\x62','\x38','\x32',
    '\x37','\x30','\x00','\x00'};
  const char  arch83xx[] = { // fsl,mpc8309atc\0\0
    '\x66','\x73','\x6c','\x2c','\x6d','\x70','\x63','\x38',
    '\x33','\x30','\x39','\x61','\x74','\x63','\x00','\x00'};


  fp = fopen(fn,"rb");

  if(fp == NULL)
  {
    fitPrint(ERROR,"cannot open %s\n",fn);
    return(ret);
  }
  else
  {
    // get file length
    fseek(fp,0,SEEK_END);
    fLen = (u_int32)ftell(fp);
    fseek(fp,0,SEEK_SET);

    buf = malloc(fLen+1u); // clever in case ftell() returns -1
    if(buf == NULL)
    {
      fitPrint(ERROR,"ARCH: cannot malloc %lu bytes\n",fLen);
      fclose(fp);
      return(ret);
    }

    (void) fread(buf,fLen,1,fp);
    fclose(fp);

    if (strncmp(buf,arch82xx,fLen) == 0)
    {
      ret = ARCH_82XX;
    }
    else if (strncmp(buf,arch83xx,fLen) == 0)
    {
      ret = ARCH_83XX;
    }
    else
    {
      ret = ARCH_UNKNOWN; // ATC 6 compliance presumed
    }

    free(buf);
  }

  return(ret);
}

// get time stamp
static void ftGetTs(char *ts)
{
  time_t     ltime;
  char       ntime[32];
  struct tm  rslt;

  ltime = time(NULL);
  localtime_r(&ltime,&rslt);
  asctime_r(&rslt,ntime);
  ntime[strlen(ntime)-1u] ='\0';// remove newline
  strcat(ts,ntime);
}

// get mac address
static void ftGetMac(char *macaddr)
{
  int32 fd;
  struct ifreq ifr;
  struct sockaddr addr;

  fd = socket(PF_INET, SOCK_DGRAM, 0);
  if(fd < 0)
  {
    fitPrint(ERROR, "Could not open socket to get MAC Address: %s\n", strerror(errno));
    return;
  }

  strcpy(ifr.ifr_name, "eth0");

  if(ioctl (fd, SIOCGIFHWADDR, &ifr) < 0)
  {
    fitPrint(ERROR, "Error getting MAC Address: %s\n", strerror(errno));
    strcpy(macaddr, "MAC ERROR");
    close(fd);
    return;
  }

  addr = ifr.ifr_ifru.ifru_hwaddr;

  sprintf(macaddr, "%02X:%02X:%02X:%02X:%02X:%02X", addr.sa_data[0], addr.sa_data[1],
          addr.sa_data[2], addr.sa_data[3], addr.sa_data[4], addr.sa_data[5]);

  close(fd);
  return;
}


/*
*
* Generate CRC on memory range
* Note: ending address is 1 byte AFTER the range to be checked.
* Per HDLC 16-bit FCS Computation in RFC 1662 C.2
*
*/

u_int16 genCrc(const void *saddr,const void *eaddr)
{
  const u_int8  *cp;
  u_int16  fcs = 0xffffu;

  for(cp=saddr;cp < (const u_int8*)eaddr;cp++)
  {
    fcs = (fcs >> 8u) ^ fcstab[(fcs ^ *cp) & 0xffu];
  }
  return(fcs);
}

/*
*
* Memory dump
*
*/

void mdmp(const void *vp, size_t sz, ftPrintLevels_t printLevel)
{
  size_t i;
  const u_int8 *ucp;

  ucp = vp;

  for(i=0u;i<sz;i++){
    if ((i%32u) == 0u) {
      fitPrint(printLevel, "\n%8.8x:",i);
    }
    if((i%16u) == 0u){
      fitPrint(printLevel, " ");
    }
    fitPrint(printLevel, "%2.2x",*ucp++);
  }

  fitPrint(printLevel, "\n");
}

void mdmp1(const void *vp, size_t sz, ftPrintLevels_t printLevel,
           const char *fp, const char *fn, int32 ln, const char *us)
{
  size_t i;
  const u_int8 *ucp;

  ucp = vp;
  fitPrint(printLevel,"\nLine %lu of File %s: %s\n%s\n",ln,fp,fn,us);

  for(i=0;i<sz;i++){
    if ((i%32u) == 0u) {
      fitPrint(printLevel, "\n%8.8x:",i);
    }
    else if((i%4u) == 0u){
      fitPrint(printLevel, " ");
    }
    else
    {
      // don't print anything
    }

    fitPrint(printLevel, "%2.2x",*ucp++);
  }

  fitPrint(printLevel, "\n");
}


void *createSharedMemory (const char *fileName, int32 *shmid, size_t mem_size)
{
  key_t key;
  void *shMemPtr = NULL;

  // Create file before we run ftok() otherwise ftok will fail
  FILE *fp = fopen (fileName, "a+");

  if (fp != NULL)
  {
    fclose(fp);
  }

  key = ftok (fileName, (int32)'R');

  if (key != -1)
  {
    *shmid = shmget (key, mem_size, SHM_PERM | IPC_CREAT);

    if (*shmid != -1)
    {
      /* The get succeeded */
      shMemPtr = shmat (*shmid, (void *)0, 0);

      if (shMemPtr == NULL)
      { /* Attach has failed for some reason; this should never happen */
        /* clean up by removing the key */
        fprintf (stderr, "u_os_link: shmat failed \n");
        (void)shmctl (*shmid, IPC_RMID,NULL);
      }
    }
    else
    {
      fprintf (stderr, "*** shared memory get failed %ld err: %d\n", *shmid, errno);
    }
  }
  else
  {
    fprintf (stderr, "ftok() failed for file: %s", fileName);
  }

  return shMemPtr;
}

/*
*
* utility to dump function arguments
*
*/

void dmpargs(plint argc,char * const argv[],char *cp)
{
  int32 i;

  if (*cp != '\0')
  {
    fitPrint(VERBOSE, "%s\n",cp);
  }

  for(i=0;i<argc;i++){
    fitPrint(VERBOSE, "%2.2ld:%s\n",i,argv[i]);
  }
}

/*
*
* Test Wrappers
*
*/

static ftRet_t datakey(plint argc,char * const argv[])
{
  return(datakeyFit(argc,argv));
}

static ftRet_t display(plint argc,char * const argv[])
{
  return(displayFit(argc,argv));
}

static ftRet_t eeprom(plint argc,char * const argv[])
{
  return(eepromFit(argc,argv));
}

static ftRet_t ethernet(plint argc,char *const argv[])
{
  return(ethernetFit(argc,argv));
}

static ftRet_t fioMonitor(plint argc,char *const argv[])
{
  return(fioMonitorFit(argc,argv));
}

static ftRet_t powerdown(plint argc,char *const argv[])
{
  return(powerdownFit(argc,argv));
}

static ftRet_t fs(plint argc,char * const argv[])
{
  ftRet_t retVal;
  const char * const *dirList;

  if (argc > 1)
  {
    fitPrint(USER, "Usage: %s\nOptions: none are permitted.\n\n",argv[0]);
    fitLicense();
    return(ftComplete);
  }

  switch (targetARCH)
  {
    case ARCH_82XX:
    case ARCH_83XX:
      dirList = dirList_82xx;
      break;
    case ARCH_UNKNOWN:
    default:
      dirList = dirList_ATC;
      break;
  };

  retVal = fsFit(argc,argv,dirList);
  ftUpdateTestStatus(ftrp,retVal,NULL);

  return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
}

static ftRet_t sd(plint argc,char * const argv[])
{
  /* Ensure that /mnt/sd is mounted prior to
   * calling fsFit for the sd card.
   */
  ftRet_t retVal = ftFail;
  const char * const *dirList;

  if (argc > 1)
  {
    fitPrint(USER, "Usage: %s\nOptions: none are permitted.\n\n",argv[0]);
    fitLicense();
    return(ftComplete);
  }

  switch(targetARCH){
    case ARCH_82XX:
      dirList = sdNames_82xx;
      break;
    case ARCH_83XX:
    case ARCH_UNKNOWN:
      dirList = sdNames_ATC;
      break;
    default:
      dirList = sdNames_none;
      ftUpdateTestStatus(ftrp,retVal,NULL);
      retVal = ftUpdateTestStatus(ftrp,ftComplete,NULL);
      break;
  }

  if (retVal == ftComplete)
  {
    return retVal;
  }

  retVal = mediaFit(argc,argv,dirList);
  ftUpdateTestStatus(ftrp,retVal,NULL);

  return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
}

static ftRet_t usb(plint argc,char * const argv[])
{
  /* Select device to mount, /mnt/usb or /media/usb, is mounted prior to
   * calling fsFit for the usb card.
   */
  ftRet_t retVal = ftFail;
  const char * const *dirList;

  if (argc > 1)
  {
    fitPrint(USER, "Usage: %s\nOptions: none are permitted.\n\n",argv[0]);
    fitLicense();
    return(ftComplete);
  }

  switch(targetARCH){
    case ARCH_82XX:
      dirList = usbNames_82xx;
      break;
    case ARCH_83XX:
    case ARCH_UNKNOWN:
      dirList = usbNames_ATC;
      break;
    default:
      dirList = usbNames_none;
      ftUpdateTestStatus(ftrp,retVal,NULL);
      retVal = ftUpdateTestStatus(ftrp,ftComplete,NULL);
      break;
  }

  if (retVal == ftComplete)
  {
    return retVal;
  }

  retVal = mediaFit(argc,argv,dirList);
  ftUpdateTestStatus(ftrp,retVal,NULL);

  return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
}

static ftRet_t memory(plint argc,char * const argv[])
{
  return(memoryFit(argc,argv));
}

static ftRet_t rtc(plint argc,char * const argv[])
{
  return(rtcFit(argc,argv));
}

static ftRet_t serial(plint argc,char * const argv[])
{
  return(serialFit(argc,argv));
}

static ftRet_t serial_echo(plint argc,char * const argv[])
{
  return(serialEchoFit(argc,argv));
}

static ftRet_t serial_port(plint argc,char * const argv[])
{
  return(serialPortFit(argc,argv));
}

#if 0
static ftRet_t spRoute(plint argc,char * const argv[])
{
  return(spRouteFit(argc,argv));
}

static ftRet_t tod(plint argc,char * const argv[])
{
  return(todFit(argc,argv));
}
#endif

/*
*
* Initialized data for test calling interface
*
*/

typedef struct _ft {
  const char *ftName;
  ftRet_t (*ftfp)(plint argc,char * const argv[]);
} ft;


static const ft ftTest[] = {
  { "datakey",datakey },
  { "display",display },
  { "eeprom",eeprom },
  { "ethernet",ethernet },
  { "fio_monitor",fioMonitor },
  { "filesystem",fs },
  { "sd",sd },
  { "usb",usb },
  { "memory",memory },
  { "powerdown",powerdown },
  { "rtc",rtc },
  { "serial",serial },
  { "serial_echo",serial_echo },
  { "serial_port",serial_port },
#if 0
  { "serial_route",spRoute },
  { "tod",tod },
#endif
  { "",NULL }
};

/*
*
* Helper print function for ftUpdateTestStatus().
*
*/
static void printCountIfNonzero(u_int32 cnt, const char *name, char *tbuf,
                                char *lbuf, int32 *pfCnt)
{
  if(cnt != 0u)
  {
    sprintf(tbuf,"                         %-9s %4.4lu\n",name,cnt);
    strcat(lbuf,tbuf);
    (*pfCnt)++;
  }
}

/*
*
* Updates the status of the overall test.
* This function always returns the value passed in for "ret"
*
*/
ftRet_t ftUpdateTestStatus(ftResults_t *ftres, ftRet_t ret, const char *descr)
{
  char lbuf[MUST_BE_BIG_ENOUGH],tbuf[MUST_BE_BIG_ENOUGH];
  int32 i;
  int32 passCnt=0;
  int32 failCnt=0;
  bool  getOut = false;
  bool  testFailed = false;

  lbuf[0] = '\0'; tbuf[0] = '\0';

  ftGetTs(lbuf); // get the time stamp
  strcat(lbuf, ": ");
  ftGetMac(tbuf); // get the MAC address
  strcat(lbuf, tbuf);
  strcat(lbuf, ": ");

  switch(ret){
    case ftStart:
      ftResults.ftStartCnt++;
      strcat(lbuf,"STARTING ");
      break;
    case ftComplete:
      /*
      *
      * Dump minimal results and report pass/fail status in the log
      * along with pass/error counts
      *
      */
      if ((ftres->ftFailCnt != 0u)   || (ftres->ftErrorCnt != 0u)   ||
          (ftres->ftSignalCnt != 0u) || (ftres->ftTxErrorCnt != 0u) ||
          (ftres->ftTxFailCnt != 0u) || (ftres->ftRxErrorCnt != 0u) ||
          (ftres->ftRxTimeoutCnt != 0u) || (ftres->ftRxFailCnt != 0u) )
      {
        if (descr == NULL)
        {
          sprintf (tbuf, "%s TEST HAS FAILED\n",ftTestName);
        }
        else
        {
          sprintf (tbuf, "%s TEST HAS FAILED FOR %s\n",ftTestName, descr);
        }
        strcat (lbuf, tbuf);
        fitPrint(PASSFAIL, "%s",lbuf);
        testFailed = true;
      }
      else if (ftres->ftPassCnt != 0u)
      {
        sprintf (tbuf, "%s TEST HAS PASSED\n",ftTestName);
        strcat (lbuf, tbuf);
        fitPrint(PASSFAIL, "%s",lbuf);
      }
      else
      {
        sprintf (tbuf, "%s TEST DID NOT RUN CORRECTLY\n",ftTestName);
        strcat (lbuf, tbuf);
        fitPrint(PASSFAIL, "%s",lbuf);
        testFailed = true;
      }

      ftres->ftCompleteCnt++;

      printCountIfNonzero(ftres->ftPassCnt,     "Pass",     tbuf,lbuf,&passCnt);
      printCountIfNonzero(ftres->ftFailCnt,     "Fail",     tbuf,lbuf,&failCnt);
      printCountIfNonzero(ftres->ftErrorCnt,    "Error",    tbuf,lbuf,&failCnt);
      printCountIfNonzero(ftres->ftSignalCnt,   "Signal",   tbuf,lbuf,&failCnt);
      printCountIfNonzero(ftres->ftTxErrorCnt,  "TxError",  tbuf,lbuf,&failCnt);
      printCountIfNonzero(ftres->ftTxFailCnt,   "TxFail",   tbuf,lbuf,&failCnt);
      printCountIfNonzero(ftres->ftRxErrorCnt,  "RxError",  tbuf,lbuf,&failCnt);
      printCountIfNonzero(ftres->ftRxTimeoutCnt,"RxTimeout",tbuf,lbuf,&failCnt);
      printCountIfNonzero(ftres->ftRxFailCnt,   "RxFail",   tbuf,lbuf,&failCnt);

     // FIT signal handler catches this, use this result buffer
      if(ftResults.ftInterruptCnt != 0u){
        strcat(lbuf, "                         Test Interrupted\n");
      }

      break;
    case ftPass:
      ftres->ftPassCnt++;
      getOut = true;
      break;
    case ftFail:
      ftres->ftFailCnt++;
      getOut = true;
      break;
    case ftError:
      ftres->ftErrorCnt++;
      getOut = true;
      break;
    case ftSignal:
      ftres->ftSignalCnt++;
      getOut = true;
      break;
    case ftTxError:
      ftres->ftTxErrorCnt++;
      getOut = true;
      break;
    case ftTxTimeout:
      ftres->ftTxFailCnt++;
      getOut = true;
      break;
    case ftTxFail:
      ftres->ftTxFailCnt++;
      getOut = true;
      break;
    case ftRxError:
      ftres->ftRxErrorCnt++;
      getOut = true;
      break;
    case ftRxTimeout:
      ftres->ftRxTimeoutCnt++;
      getOut = true;
      break;
    case ftRxFail:
      ftres->ftRxFailCnt++;
      getOut = true;
      break;
    case ftNewline:
      ftres->ftNewlineCnt++;
      getOut = true;
      break;
    case ftInformation:
      ftres->ftInformationCnt++;
      break;
    case ftInterrupt:
      ftResults.ftInterruptCnt++;
      getOut = true;
      break;
    case ftUnimplemented:
    default:
      ftResults.ftUnimplementedCnt++;
      getOut = true;
      break;
  }

  if (getOut)
  {
    return ret;
  }

  for(i=0;lbuf[i]!='\0';i++){// convert client string to upper case
    lbuf[i] = (char)toupper((int32)lbuf[i]);
  }

  if (testFailed)
  {
    fitPrint(LOG_FAILURE, "%s\n", lbuf); // output to failure logfile
  }

  fitPrint(LOG, "%s\n", lbuf); // output to logfile

  return ret;
}

/*
*
* Compact NULL argument vectors
*
* WARN: obligatory recursion warning
*
*/

static int32 compactArgv(int32 argCount,char *avp[],int32 depth)
{
  int32 idx,nullFound=0;

  if(depth >= MAX_RECURS_DEPTH){
    fitPrint(ERROR, "compactArgv: recursion limit reached %ld\n",depth);
    return(-1);
  }

  for(idx=0;idx<argCount;idx++){
    if(avp[idx] == NULL){ // found embedded NULL in argv
      avp[idx] = avp[idx + 1];
      avp[idx + 1] = NULL;
      nullFound++;
    }
  }

  if(nullFound != 0){
    if((argCount > 1) && (avp[argCount - 1] == NULL)){
      argCount--;
    }
    return(compactArgv(argCount,avp,++depth)); // FIX -- JSG XXXXX
  }

  return(argCount);
}

/*
*
* FIT Usage
*
*/

static void fitUsage(void){
  int32 i;

  fitPrint(USER, "usage: fit [client] [fit options] [client options]\n");

  fitPrint(USER,"fit is a multi-call binary and the head of the M-Fit suite.\n" \
           "It supports the following M-Fit clients:\n");

  for(i=0;*ftTest[i].ftName != '\0';i++)
  {
    fitPrint(USER, "\t%s\n",ftTest[i].ftName);
  }

  fitPrint(USER, "\n\tClients that are configurable will support -h option to display\n" \
           "\tindividual usage information\n\n");
  fitPrint(USER, "The fit infrastructure supports options:\n" \
           "\t-F to enable fail-only logging\n" \
           "\t-L to enable logging\n" \
           "\t-I count to run the test iteratively (default 1; 0 for continuous)\n" \
           "\t-V to allow verbose printout\n\n");
  fitLicense();
}


/*
*
* FIT GPL version 1 license
*
*/

void fitLicense(void){
  fitPrint(USER, "M-Fit Suite v%s, %s\n" \
           "\tLicense: GPL-2 -- https://www.gnu.org/licenses/gpl-2.0.html\n" \
           "\tThis is free software, and you are welcome to redistribute it\n" \
           "\tunder certain conditions; see the terms of the Licence.\n" \
           "\tThis software comes with ABSOLUTELY NO WARRANTY.\n",
           ftVersion,copyright);
}


/*
 * Signal handler to handle Ctrl-C exits
 */
static void sig_handler(int sig)
{
  if (sig == SIGINT)
  {
    keepGoing = false;
    ftResults.ftInterruptCnt++;
  }
}

/*
*
* FIT Entry
*
*/

int main(int argc, char * argv[]) // argv[] must NOT be const in this case
{
  char *testName ;
  char lbuf[MUST_BE_BIG_ENOUGH];
  struct timespec start_tm,end_tm;
  double t_s = 0.0;
  int32 i,j,iter = 1;
  int32 c;
  bool  testFound = false;
  bool  runContinuous = false;
  struct sigaction sigact;

  c = strcmp((char *)basename(argv[0]),"fit");

  if((argc == 1) && (c == 0))
  {
    fitUsage();// fit called with no arguments
    return(ENOARGS);
  }

  // install the signal handler
  sigact.sa_handler = sig_handler;
  sigemptyset (&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction (SIGINT, &sigact,NULL);

  // caller can be 'fit ftTest' or 'ftTest', adjust argv index
  if(strcmp((char *)basename(argv[0]),"fit") == 0){
    j=1;
  } else {
    j=0;
  }
  testName = argv[j];

  /*
  *
  * Parse command line arguments
  *
  */

  opterr = 0;
  c = getopt(argc,argv,"-LFVI:");

  while(c != -1)
  {
    switch((char)c){
      case 'L':
        logfileFlag = true;
        argv[optind - 1] = NULL;
        break;
      case 'F':
        logFailureFileFlag = true;
        argv[optind - 1] = NULL;
        break;
      case 'I':
        iter = strtol(optarg,NULL,10);

        if (iter == 0)
        {
          runContinuous = true; // run forever
        }

        argv[optind - 1] = NULL;
        argv[optind - 2] = NULL;
        break;
      case 'V':
        verboseFlag = true;
        argv[optind - 1] = NULL;
        break;
      default:
        // nothing to do
        break;
    }
    c = getopt(argc,argv,"-LFVI:");
  }

  optind = 0; // for reentrancy

  /*
  * Try to get a valid architecture so the ensuing tests
  * can self configure appropriately.
  */

  targetARCH = getTargetArch(); // run with whatever is found

  /*
  *
  * HACK: sharing argument list between fit infra-structure and client
  *       tests creates an unfortunate one to many issue. The attempt
  *       to NULL fit specific argv (see above while statement), causes
  *       the subsequent getopt() call from the test to segment fault.
  *
  *       Compact argv and adjust argc.
  *
  */

  argc = compactArgv(argc,argv,0);
  if (argc < 0)
  {
    return argc;
  }

  /*
  *
  * Find the test
  *
  */

  for(i=0;*ftTest[i].ftName != '\0';i++){
    if(strcmp(ftTest[i].ftName,testName) == 0){
      testFound = true;
      if(logfileFlag)
      {
        size_t tlen;

        if(strlen(testName) > sizeof(lbuf))
        {
          fitPrint(ERROR, "%s cannot copy arg size %u to buf size %u\n",
                   testName,strlen(testName),sizeof(lbuf));
          return(ENOBUF);
        }

        lbuf[0] = '\0';
        strcat(lbuf,testName);
        tlen = strnlen(lbuf, sizeof(lbuf));
        tlen += strlen(".log");

        if(tlen >= sizeof(lbuf))
        {
          fitPrint(ERROR, "%s cannot copy arg size %u to buf size %u\n",
                   testName,tlen,sizeof(lbuf));
          return(ENOBUF);
        }

        // open test specific logfile for append
        ftLogfp = fopen(strcat(lbuf,".log"),"a");
        if(ftLogfp == NULL){
          fitPrint(ERROR, "%s: cannot open logfile %s, err %d, %s\n",
                   testName,lbuf,errno,strerror(errno));
          return(ENOFILE);
        }
      }

      strcpy(ftTestName,ftTest[i].ftName);
      ftUpdateTestStatus(ftrp,ftStart,NULL);

      /*
      *
      * Call the test
      *
      */

      clock_gettime(CLOCK_MONOTONIC,&start_tm); // start interval timing

      do {

        memset(&ftResults, 0, sizeof(ftResults)); // reset the test counts
        (void) ftTest[i].ftfp(argc - j,&argv[j]); // call the test
        --iter;
      } while( keepGoing && (runContinuous || (iter > 0)) );

      clock_gettime(CLOCK_MONOTONIC,&end_tm); // end interval timing

      /*
      *
      * Calculate elapsed time
      *
      */

      end_tm.tv_sec -= start_tm.tv_sec;
      end_tm.tv_nsec -= start_tm.tv_nsec;
      if(end_tm.tv_nsec < 0){
        end_tm.tv_sec--;
        end_tm.tv_nsec += 1000000000;
      }

      t_s = (double)end_tm.tv_sec + ((double)end_tm.tv_nsec * 1.0e-9);

      fitPrint(LOG, "                         ELAPSED TIME %16.2f S\n\n",t_s);

      if(logfileFlag){
        fclose(ftLogfp);// close logfile file pointer
      }

      break; // this test has finished
    }
  }

  if(!testFound){
    fitPrint(ERROR, "%s test not supported\n",testName);
    fitUsage();
  }

  return(0);
}

void fitPrint(ftPrintLevels_t printLevel, const char *format, ...)
{
  va_list args;

  va_start (args, format); //lint !e530 Initialization of 'args' is not required

  switch (printLevel)
  {
    case LOG:
      if (logfileFlag)
      {
        if (ftLogfp != NULL)
        {
          vfprintf (ftLogfp, format, args);
        }
      }
      break;

    case LOG_FAILURE:
      if (logFailureFileFlag)
      {
        ftFailLogfp = fopen(failLogFilename, "a");

        if (ftFailLogfp != NULL)
        {
          // Open successful
          vfprintf (ftFailLogfp, format, args);
          fclose (ftFailLogfp);
        }
        else
        {
          fitPrint(ERROR, "Cannot open logfile %s, err %d, %s\n",
                   failLogFilename, errno, strerror(errno));
        }
      }
      break;

    case PASSFAIL: // Always print out to stdout
      vprintf (format, args);
      break;

    case ERROR: // Always print out to stderr
      vfprintf (stderr, format, args);
      break;

    case VERBOSE: // Only print out to stdout if verbose
      if (verboseFlag)
      {
        vprintf (format, args);
      }
      break;

    case USER: // Always print out to stdout
      vprintf (format, args);
      break;

    default:
      // can't happen
      break;
  }

  va_end (args);
}

/*
 * Same as calling read() but with a given timeout
 *
 * Returns the same as standard read() call with the exception of on select()
 * timeout returns -1 and sets errno to ETIMEDOUT,
 * -2 for read() error, and -3 for select() error.
 */
int32 readTimeout(int32 fd,void *buf,size_t nbytes,int32 timeoutSeconds)
{
  int32  retval;
  fd_set rfds;
  struct timeval tv;

  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);

  tv.tv_sec = timeoutSeconds;
  tv.tv_usec = 0;
  retval = select(fd + 1, &rfds,NULL, NULL, &tv);

  if(retval == 0)
  {
    // Select timed out
    fitPrint(VERBOSE, "%s: %s select timed out reading from fd %ld, sz %4.4u\n",
             __func__,ftTestName,fd,nbytes);
    ftUpdateTestStatus(ftrp,ftRxTimeout,NULL);
    retval = -1;
    errno = ETIMEDOUT;
  }
  else if(retval > 0)
  {
    //sleep(4);//unfortunate
    retval = read(fd,buf,nbytes);
    if (retval < 0)
    {
      fitPrint(ERROR,"%s: Error reading for test %s: %s\n",__func__,ftTestName,
               strerror(errno));
      ftUpdateTestStatus(ftrp,ftRxError,NULL);
      retval = -2;
    }
  }
  else // retval < 0
  {
    fitPrint(ERROR,"%s: select error for test %s: %s\n",
             __func__,ftTestName,strerror(errno));
    ftUpdateTestStatus(ftrp,ftRxError,NULL);
    retval = -3;
  }

  return retval;
}
