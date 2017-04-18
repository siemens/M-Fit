/******************************************************************************
                                  datakeyFit.c

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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "fit.h"

#define DK_BULK_ERASE_ATC _IO('D',1)
#define DK_BULK_ERASE 1

static ftResults_t ftResults = {0};
static ftResults_t *ftrp = &ftResults;

/*
*
* Datakey test entry
*
*/

ftRet_t datakeyFit(plint argc, char * const *argv)
{
  int32   dkFd;
  ssize_t bCnt;
  u_int32 tempSize;
  u_int32 erase_cmd;
  u_int32 prev_bad_addr;
  int32   merr = 0;
  int32   err;
  void   *mp;
  void   *mp_orig;
  const u_int8  *cp;
  const char    *sd = "/dev/datakey";
  off_t dkOfffset;
  size_t dkSize;

  if (argc > 1)
  {
    fitPrint(USER, "Usage: %s\nOptions: none are permitted.\n\n",argv[0]);
    fitLicense();
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  if (targetARCH >= ARCH_83XX)
  {
    erase_cmd = DK_BULK_ERASE_ATC;
  }
  else
  {
    erase_cmd = DK_BULK_ERASE;
  }

  /*
  *
  * Open datakey and get size
  *
  */

  dkFd = open(sd,O_RDWR);

  if(dkFd == -1){
    fitPrint(ERROR, "%s test cannot open %s, err %d: %s\n",
             argv[0],sd,errno,strerror(errno));
    ftUpdateTestStatus(ftrp,ftError,NULL);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  dkOfffset = lseek(dkFd,0,SEEK_END);

  if (dkOfffset <=0)  // a zero-length device is also a failure
  {
    fitPrint(ERROR, "%s test cannot detect size of %s; err %d: %s\n",
             argv[0],sd,errno,strerror(errno));
    ftUpdateTestStatus(ftrp,ftError,NULL);
    close(dkFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }
  else
  {
    dkSize = (size_t)dkOfffset;
    fitPrint(VERBOSE, "%s test detects %s, size %u bytes (%5.3f Mb).\n",
             argv[0],sd,dkSize,((double)dkSize/131072.0));
  }

  /*
  *
  * Malloc receive buffers to save original content and use temporary data
  *
  */

  /* buffer used to store the unmodified datakey contents */
  mp_orig = malloc(dkSize);
  if(mp_orig == NULL){
    fitPrint(ERROR, "%s test cannot malloc %u bytes, err %d: %s\n",
             argv[0],dkSize,errno,strerror(errno));
    close(dkFd);
    ftUpdateTestStatus(ftrp,ftError,NULL);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  memset(mp_orig,0,dkSize);// clear out memory

  /* buffer used for destructive testing of datakey data */
  mp = malloc(dkSize);
  if(mp == NULL){
    fitPrint(ERROR, "%s test cannot malloc %u bytes, err %d: %s\n",
             argv[0],dkSize,errno,strerror(errno));
    free(mp_orig);
    close(dkFd);
    ftUpdateTestStatus(ftrp,ftError,NULL);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  /*
  *
  * Save the datakey device
  *
  */

  dkOfffset = lseek(dkFd,0,SEEK_SET);

  if (dkOfffset < 0)
  {
    fitPrint(ERROR, "\n%s test cannot seek to beginning of %s; err %d: %s\n",
             argv[0],sd,errno,strerror(errno));
    ftUpdateTestStatus(ftrp,ftError,NULL);
    free(mp_orig);
    free(mp);
    close(dkFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }
  // further calls to lseek(dkFd,0,SEEK_SET) are presumed to work without error

  fitPrint(VERBOSE, "%s saving original contents %s, size %u ...\n",argv[0],sd,dkSize);
  bCnt = read(dkFd,mp_orig,dkSize);

  if(bCnt != (int32)dkSize){
    if(bCnt == -1){
      fitPrint(ERROR, "\n%s cannot read from %s, err %d: %s\n",
               argv[0],sd,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      fitPrint(ERROR, "\n%s failed reading from %s, expected %u, actual %d\n",
               argv[0],sd,dkSize,bCnt);
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    free(mp_orig);
    free(mp);
    close(dkFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  fitPrint(VERBOSE, "\t\tsave complete.\n");

  /*
  *
  * Bulk erase the datakey device
  *
  */

  fitPrint(VERBOSE, "%s bulk erasing %s ...\n",argv[0],sd);
  err = ioctl(dkFd,erase_cmd,0);

  if(err < 0){
    fitPrint(ERROR, "\nBulk erase: err %ld: %s\n",err,strerror(errno));
    ftUpdateTestStatus(ftrp,ftError,NULL);
    free(mp_orig);
    free(mp);
    close(dkFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  fitPrint(VERBOSE, "\t\terasure complete.\n");

  /*
  *
  * Read the datakey device
  *
  */

  (void) lseek(dkFd,0,SEEK_SET);

  fitPrint(VERBOSE, "%s reading erased device %s ...\n",argv[0],sd);

  bCnt = read(dkFd,mp,dkSize);

  if(bCnt != (int32)dkSize){
    if(bCnt == -1){
      fitPrint(ERROR, "\n%s cannot read from %s, err %d: %s\n",
               argv[0],sd,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      fitPrint(ERROR, "\n%s failed reading from %s, expected %u, actual %d\n",
               argv[0],sd,dkSize,bCnt);
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    free(mp_orig);
    free(mp);
    close(dkFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  fitPrint(VERBOSE, "\t\tread complete.\n");

  /*
  *
  * Verify that datakey device has successfully bulk erased
  *
  */

  fitPrint(VERBOSE, "%s verifying erasure of %s ...\n",argv[0],sd);
  cp=(u_int8*)mp;
  prev_bad_addr=0xffffffffu;

  for(tempSize=0;tempSize<dkSize ;tempSize++){
    if(*cp != (u_int8)0xff){
      prev_bad_addr = tempSize;
      fitPrint(ERROR, "%s device is not erased, addr %lu, merr %ld, " \
               "val 0x%2.2x\n",argv[0],tempSize,++merr,*cp);
      break; // comment this break out to see errors at all addresses
    }
    cp++;
  }

  if((tempSize!=dkSize) || (prev_bad_addr!=0xffffffffu)){
    fitPrint(ERROR, "\n%s failed verify bulk erase from %s, dkSize %u address %lu, " \
             "prev_bad_addr %lu\n",argv[0],sd,dkSize,tempSize,prev_bad_addr);
    ftUpdateTestStatus(ftrp,ftFail,NULL);
  } else {
    fitPrint(VERBOSE, "\t\terasure verified.\n");
    ftUpdateTestStatus(ftrp,ftPass,NULL);
  }

  /*
  *
  * Write the datakey device
  *
  */

  (void) lseek(dkFd,0,SEEK_SET);

  memset(mp,0,dkSize);// clear out memory

  fitPrint(VERBOSE, "%s writing zeros to %s ...\n",argv[0],sd);
  bCnt = write(dkFd,mp,dkSize);

  if(bCnt != (int32)dkSize){
    if(bCnt == -1){
      fitPrint(ERROR, "%s cannot write to %s, err %d: %s\n",
               argv[0],sd,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      fitPrint(ERROR, "%s failed write to %s, expected %u, actual %d\n",
               argv[0],sd,dkSize,bCnt);
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    free(mp_orig);
    free(mp);
    close(dkFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  fitPrint(VERBOSE, "\t\twrite complete.\n");


  /*
  *
  * Read the datakey device
  *
  */

  (void) lseek(dkFd,0,SEEK_SET);

  fitPrint(VERBOSE, "%s reading zeros from %s ...\n",argv[0],sd);

  bCnt = read(dkFd,mp,dkSize);
  if(bCnt != (int32)dkSize){
    if(bCnt == -1){
      fitPrint(ERROR, "%s cannot read from %s, err %d: %s\n",
               argv[0],sd,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      fitPrint(ERROR, "%s failed reading from %s, expected %u, actual %d\n",
               argv[0],sd,dkSize,bCnt);
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    free(mp_orig);
    free(mp);
    close(dkFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  fitPrint(VERBOSE, "\t\tread complete.\n");

  /*
  *
  * Verify that datakey device has successfully cleared
  *
  */

  fitPrint(VERBOSE, "%s verifying zeros on %s ...\n",argv[0],sd);
  cp=(u_int8*)mp;
  prev_bad_addr=0xffffffffu;

  for(tempSize=0;tempSize<dkSize;tempSize++){
    if(*cp != 0u){
      prev_bad_addr = tempSize;
      fitPrint(ERROR, "%s device is not erased, addr %lu, daddr %lu, merr %ld, " \
               "val 0x%2.2x\n",argv[0],tempSize,tempSize - prev_bad_addr,++merr,*cp);
      break; // comment this break out to see errors at all addresses
    }
    cp++;
  }

  if((tempSize!=dkSize) || (prev_bad_addr!=0xffffffffu)){
    fitPrint(ERROR, "\n%s failed verify not cleared from %s, dkSize %u address %lu, " \
             "prev_bad_addr %lu\n",argv[0],sd,dkSize,tempSize,prev_bad_addr);
    ftUpdateTestStatus(ftrp,ftFail,NULL);
  } else {
    fitPrint(VERBOSE, "\t\tzeros verified.\n");
    ftUpdateTestStatus(ftrp,ftPass,NULL);
  }

  /*
  *
  * Restore the datakey device to original content
  *
  */

  /*
  *
  * Bulk erase the datakey device
  *
  */

  (void) lseek(dkFd,0,SEEK_SET);

  fitPrint(VERBOSE, "%s bulk erasing %s ...\n",argv[0],sd);

  err = ioctl(dkFd,erase_cmd,0);
  if(err < 0){
    fitPrint(ERROR, "\nBulk erase: err %ld: %s\n",err,strerror(errno));
    ftUpdateTestStatus(ftrp,ftError,NULL);
    free(mp_orig);
    free(mp);
    close(dkFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  fitPrint(VERBOSE, "\t\terasure complete.\n");

  /*
  *
  * Read the datakey device
  *
  */

  (void) lseek(dkFd,0,SEEK_SET);

  fitPrint(VERBOSE, "%s reading erased device %s ...\n",argv[0],sd);

  bCnt = read(dkFd,mp,dkSize);
  if(bCnt != (int32)dkSize){
    if(bCnt == -1){
      fitPrint(ERROR, "%s cannot read from %s, err %d: %s\n",
               argv[0],sd,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      fitPrint(ERROR, "%s failed reading from %s, expected %u actual %d\n",
               argv[0],sd,dkSize,bCnt);
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    free(mp_orig);
    free(mp);
    close(dkFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  fitPrint(VERBOSE, "\t\tread complete.\n");

  /*
  *
  * Verify that datakey device has successfully bulk erased
  *
  */

  fitPrint(VERBOSE, "%s verifying erasure of %s ...\n",argv[0],sd);
  cp=(u_int8*)mp;
  prev_bad_addr=0xffffffffu;

  for(tempSize=0;tempSize<dkSize;tempSize++){
    if(*cp != (u_char)0xff){
      prev_bad_addr = tempSize;
      fitPrint(ERROR, "%s device is not erased, addr %lu, daddr %lu, merr %ld, " \
               "val 0x%2.2x\n",argv[0],tempSize,tempSize - prev_bad_addr,++merr,*cp);
      break; // comment this break out to see errors at all addresses
    }
    cp++;
  }

  if((tempSize!=dkSize) || (prev_bad_addr!=0xffffffffu)){
    fitPrint(ERROR, "\n%s failed verify bulk erase from %s, dkSize %u address %lu, " \
             "prev_bad_addr %lu\n", argv[0],sd,dkSize,tempSize,prev_bad_addr);
    ftUpdateTestStatus(ftrp,ftFail,NULL);
  } else {
    fitPrint(VERBOSE, "\t\terasure verified.\n");
    ftUpdateTestStatus(ftrp,ftPass,NULL);
  }

  /*
  *
  * Write the datakey device with original content
  *
  */

  (void) lseek(dkFd,0,SEEK_SET);

  fitPrint(VERBOSE, "%s restoring original data to %s ...\n",argv[0],sd);

  bCnt = write(dkFd,mp_orig,dkSize);
  if(bCnt != (int32)dkSize){
    if(bCnt == -1){
      fitPrint(ERROR, "\n%s cannot write to %s, err %d: %s\n",
               argv[0],sd,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      fitPrint(ERROR, "\n%s failed write to %s, expected %u, actual %d\n",
               argv[0],sd,dkSize,bCnt);
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    free(mp_orig);
    free(mp);
    close(dkFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  fitPrint(VERBOSE, "\t\twrite complete.\n");

  /*
  *
  * Read the datakey device
  *
  */

  (void) lseek(dkFd,0,SEEK_SET);

  fitPrint(VERBOSE, "%s reading restored data from %s ...\n",argv[0],sd);

  bCnt = read(dkFd,mp,dkSize);
  if(bCnt != (int32)dkSize){
    if(bCnt == -1){
      fitPrint(ERROR, "%s cannot read from %s, err %d: %s\n",
               argv[0],sd,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      fitPrint(ERROR, "%s failed reading from %s, expected %u, actual %d\n",
               argv[0],sd,dkSize,bCnt);
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    free(mp_orig);
    free(mp);
    close(dkFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  fitPrint(VERBOSE, "\t\tread complete.\n");

  /*
  *
  * Verify that datakey device has successfully restored
  *
  */

  fitPrint(VERBOSE, "%s verifying original data on %s ...\n",argv[0],sd);

  if(memcmp(mp,mp_orig,dkSize) != 0){
    fitPrint(ERROR, "\n%s failed verify, restored from %s but restore failed\n",
             argv[0],sd);
    ftUpdateTestStatus(ftrp,ftFail,NULL);
  } else {
    fitPrint(VERBOSE, "\t\tdata verified.\n");
    ftUpdateTestStatus(ftrp,ftPass,NULL);
  }

  fitPrint(VERBOSE, "%s test complete.\n",argv[0]);

  free(mp_orig);
  free(mp);
  close(dkFd);
  return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
 }
