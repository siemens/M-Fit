/******************************************************************************
                                  memoryFit.c

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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <mntent.h>  // getmntent(), hasmntent()

#include "fit.h"

static ftResults_t ftResults = {0};
static ftResults_t *ftrp = &ftResults;

// global data memory for testing
static int32 idat[0x80000]; // initialized to 0's, check this

/*
*
* destructive memory test
*
*/

static u_int32 dmtest(void *vp,size_t sz)
{
  size_t  bwcnt;
  u_int32 lbit;
  u_int16 sbit;
  u_int8  cbit;
  u_int32 *lp = vp;
  u_int16 *sp = vp;
  u_int8  *cp = vp;

  // Walking 1's test using 32-bit, 16-bit and 8-bit bus accesses
  // long bus access
  for(bwcnt=sz/4u;bwcnt!=0u;bwcnt--){
    for(lbit=1;lbit!=0u;lbit<<=1){
      *lp = lbit;
      if(*lp != lbit){
        fitPrint(VERBOSE, "walking 1's read 0x%08lx; expected 0x%08lx at loop %u of %u\n",
                 lbit,*lp,bwcnt,sz/4u);
        return(lbit);
      }
    }
    lp++;
  }

  // short bus access
  for(bwcnt=sz/2u;bwcnt!=0u;bwcnt--){
    for(sbit=1;sbit!=0u;sbit<<=1){
      *sp = sbit;
      if(*sp != sbit){
        fitPrint(VERBOSE, "walking 1's read 0x%04x; expected 0x%04x at loop %u of %u\n",
                 sbit,*sp,bwcnt,sz/2u);
        return(sbit);
      }
    }
    sp++;
  }

  // char bus access
  for(bwcnt=sz;bwcnt!=0u;bwcnt--){
    for(cbit=1;cbit!=0u;cbit<<=1){
      *cp = cbit;
      if(*cp != cbit){
        fitPrint(VERBOSE, "walking 1's read 0x%02x; expected 0x%02x at loop %u of %u\n",
                 cbit,*sp,bwcnt,sz);
        return(cbit);
      }
    }
    cp++;
  }

  // these loops are intentionally non-optimized to check for cache-mis and address alias
  cp=vp;
  for (bwcnt=sz;bwcnt!=0u;bwcnt--)
  {
    *cp = (u_int8)0xaa;
    cp++;
  }

  cp=vp;
  for (bwcnt=sz;bwcnt!=0u;bwcnt--)
  {
    *cp |= (u_int8)0x55;
    cp++;
  }

  cp=vp;
  for(bwcnt=sz;bwcnt!=0u;bwcnt--)
  {
    if (*cp != (u_int8)0xff)
    {
      fitPrint(VERBOSE, "0xaa|0x55=>0xff test failed with 0x%02x at %p\n",*cp, cp);
      return((u_int32)cp);  //lint !e9078 convert pointer to proper type for return
    }
    cp++;
  }

  return(0);
}


static u_int32 smtest(const char *sm)
{
  int32   smFd;
  u_int32 result = 0;
  ssize_t bCnt;
  u_int32 smSize,i;
  u_int16 crc1,crc2,crc3;
  off_t   smOffset;
  void   *mp_orig,*mp;
  char   *cp;

  if (sm == NULL)
  {
    // Don't know how to test the SRAM directely so exit as a success.
    fitPrint(VERBOSE, "don't know how to test sram memory\n");
    ftUpdateTestStatus(ftrp,ftPass,NULL);
    return result;
  }

  fitPrint(VERBOSE, "testing sram memory\n");

  smFd = open(sm,O_RDWR);

  if(smFd == -1){
    fitPrint(ERROR, "\ttest cannot open %s, err %d: %s\n",
             sm,errno,strerror(errno));
    ftUpdateTestStatus(ftrp,ftError,NULL);
    ftUpdateTestStatus(ftrp,ftComplete,NULL);
    return(1);
  }

  smOffset = lseek(smFd,0,SEEK_END); // get the size of device

  if(smOffset < 1){
    fitPrint(ERROR, "\test cannot seek to end of %s, err %d: %s\n",
             sm,errno,strerror(errno));
    ftUpdateTestStatus(ftrp,ftError,NULL);
    ftUpdateTestStatus(ftrp,ftComplete,NULL);
    return(2);
  }

  /*
  *
  * Malloc buffer to store original copy of SRAM
  *
  */

  /* buffer used to store the unmodified sram data contents */
  smSize = (u_int32)smOffset;
  mp_orig = malloc(smSize);

  if(mp_orig == NULL){
    fitPrint(ERROR, "\ttest cannot malloc %lu bytes, err %d: %s\n",
             smSize,errno,strerror(errno));
    close(smFd);
    ftUpdateTestStatus(ftrp,ftError,NULL);
    ftUpdateTestStatus(ftrp,ftComplete,NULL);
    return(3);
  }

  fitPrint(VERBOSE, "\tsaving %s, size %lu, ",sm,smSize);

  (void)lseek(smFd,0,SEEK_SET);
  bCnt = read(smFd,mp_orig,smSize);

  if((size_t)bCnt != smSize){
    if(bCnt == -1){
      fitPrint(ERROR, "\n\tcannot read from %s, err %d: %s\n",
               sm,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      fitPrint(ERROR, "\n\tfailed reading from %s, expected %lu actual %d\n",
               sm,smSize,bCnt);
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    free(mp_orig);
    close(smFd);
    ftUpdateTestStatus(ftrp,ftComplete,NULL);
    return(4);
  }

  cp = mp_orig;
  cp = &cp[smSize];
  crc1 = genCrc(mp_orig,cp); //lint !e826 Suspicious pointer-to-pointer conversion

  fitPrint(VERBOSE, "CRC %u, complete\n", crc1);

  /*
  *
  * Malloc buffer to store temporary copy of SRAM and fill with random numbers
  *
  */

  /* buffer used to store the modified sram data contents */
  mp = malloc(smSize);

  if(mp == NULL){
    fitPrint(ERROR, "\ttest cannot malloc %lu bytes, err %d: %s\n",
             smSize,errno,strerror(errno));
    free(mp_orig);
    close(smFd);
    ftUpdateTestStatus(ftrp,ftError,NULL);
    ftUpdateTestStatus(ftrp,ftComplete,NULL);
    return(5);
  }

  cp=mp;
  for(i=0;i<smSize;i++){
    *cp = (char)rand();
    cp++;
  }

  crc2 = genCrc(mp,cp);

  /*
  *
  * Write the random number buffer to the SRAM
  *
  */

  fitPrint(VERBOSE, "\twriting %s, size %lu, CRC %u, ",sm,smSize,crc2);
  (void)lseek(smFd,0,SEEK_SET);
  bCnt = write(smFd,mp,smSize);

  if((size_t)bCnt != smSize){
    if(bCnt == -1){
      fitPrint(ERROR, "\n\tcannot write to %s, err %d: %s\n",
               sm,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      fitPrint(ERROR, "\n\tfailed writing to %s, expected %lu actual %d\n",
               sm,smSize,bCnt);
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    free(mp_orig);
    free(mp);
    close(smFd);
    ftUpdateTestStatus(ftrp,ftComplete,NULL);
    return(6);
  }
  fitPrint(VERBOSE, "complete\n");

  /*
  *
  * Compare the SRAM with the random number buffer
  *
  */

  memset(mp,0,smSize); // clear out buffer and re-read randomized SRAM

  fitPrint(VERBOSE, "\tcomparing %s, size %lu, ",sm,smSize);
  lseek(smFd,0,SEEK_SET);

  bCnt = read(smFd,mp,smSize);

  if((size_t)bCnt != smSize){
    if(bCnt == -1){
      fitPrint(ERROR, "\n\tcannot read from %s, err %d: %s\n",
               sm,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      fitPrint(ERROR, "\n\tfailed reading from %s, expected %lu actual %d\n",
               sm,smSize,bCnt);
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    free(mp_orig);
    free(mp);
    close(smFd);
    ftUpdateTestStatus(ftrp,ftComplete,NULL);
    return(7);
  }

  cp=mp;
  cp=&cp[smSize];
  crc3 = genCrc(mp,cp); //lint !e826 Suspicious pointer-to-pointer conversion

  if(crc2 != crc3){
    fitPrint(ERROR, "\n\tfailed CRC from %s, expected 0x%x actual 0x%x\n",
             sm,crc2,crc3);
    ftUpdateTestStatus(ftrp,ftError,NULL);
  }

  fitPrint(VERBOSE, "complete\n");

  /*
  *
  * Restore the original data
  *
  */

  fitPrint(VERBOSE, "\trestoring %s, size %lu, ",sm,smSize);
  lseek(smFd,0,SEEK_SET);

  bCnt = write(smFd,mp_orig,smSize);

  if((size_t)bCnt != smSize){
    if(bCnt == -1){
      fitPrint(ERROR, "\n\tcannot write to %s, err %d: %s\n",
               sm,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      fitPrint(ERROR, "\n\tfailed writing to %s, expected %lu actual %d\n",
               sm,smSize,bCnt);
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    free(mp_orig);
    free(mp);
    close(smFd);
    ftUpdateTestStatus(ftrp,ftComplete,NULL);
    return(8);
  }

  // Check restoration
  lseek(smFd,0,SEEK_SET);

  bCnt = read(smFd,mp,smSize);

  if((size_t)bCnt != smSize){
    if(bCnt == -1){
      fitPrint(ERROR, "\n\tcannot read restoration from %s, err %d: %s\n",
               sm,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      fitPrint(ERROR, "\n\tfailed reading restoration from %s, expected %lu actual %d\n",
               sm,smSize,bCnt);
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    free(mp_orig);
    free(mp);
    close(smFd);
    ftUpdateTestStatus(ftrp,ftComplete,NULL);
    return(9);
  }

  cp=mp;
  cp=&cp[smSize];
  crc3 = genCrc(mp,cp); //lint !e826 Suspicious pointer-to-pointer conversion

  if (crc1 == crc3)
  {
    fitPrint(VERBOSE, "CRC %u, complete\n", crc3);
    ftUpdateTestStatus(ftrp,ftPass,NULL);
  }
  else
  {
    fitPrint(ERROR, "\n\trestoration CRCs did not match: original %u vs %u\n",
             crc1,crc3);
    ftUpdateTestStatus(ftrp,ftFail,NULL);
    result = 10;
  }

  free(mp_orig);
  free(mp);
  close(smFd);

  return result; //(ftUpdateTestStatus(ftrp,ftComplete,NULL));
}

/*
*
* Memory test entry
*
*/

ftRet_t memoryFit(plint argc,char * const argv[])
{
  int32   iresult;
  u_int32 result;
  u_int32 sdat[512] = { 0 };     // stack data memory for testing; all 0
  char    mnt_buf[128] = {'\0'};  // SRAM mount options
  bool    found;
  bool    remount = true;
  FILE    *mntFp;
  void    *mp;
  const char  *sm;           // SRAM MTDBlock device (/dev/mtdblock7)
  const char  *smnt;         // SRAM mount point (/dev/sram)
  const char  *mounts = "/proc/mounts";
  struct mntent mnt_info = {NULL,NULL,NULL,NULL,0,0}; // SRAM mount information

  if (argc > 1)
  {
    fitPrint(USER, "Usage: %s\nOptions: none are permitted.\n",argv[0]);
    fitPrint(USER, "Tests DRAM with global, stack and heap allocations.\n");
    fitPrint(USER, "Tests SRAM via file system.\n\n");
    fitLicense();
    return(ftComplete);
  }

  fitPrint(VERBOSE, "testing stack memory\n");
  result = dmtest(sdat,sizeof(sdat));

  if(result != 0u){
    fitPrint(VERBOSE, "stack memory test failed\n");
    ftUpdateTestStatus(ftrp,ftFail,NULL);
  }
  else
  {
    ftUpdateTestStatus(ftrp,ftPass,NULL);
  }

  fitPrint(VERBOSE, "testing initialized data memory\n");
  result = dmtest(idat,sizeof(idat));

  if(result != 0u){
    fitPrint(VERBOSE, "initialized data memory test failed\n");
    ftUpdateTestStatus(ftrp,ftFail,NULL);
  }
  else
  {
    ftUpdateTestStatus(ftrp,ftPass,NULL);
  }

  mp = malloc(0x2000000u);

  if(mp == NULL)
  {
    fitPrint(ERROR, "%s: cannot malloc\n",argv[0]);
    ftUpdateTestStatus(ftrp,ftError,NULL);
  }
  else
  {
    fitPrint(VERBOSE, "testing heap memory\n");
    result = dmtest(mp,0x2000000u);
    if(result != 0u)
    {
      fitPrint(VERBOSE, "heap memory test failed\n");
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    else
    {
     // ftUpdateTestStatus(ftrp,ftPass,NULL);
    }

    free(mp);
  }


  /*
  *
  *  SRAM tested as an mtdblock device
  *
  */

  fitPrint(VERBOSE, "preparing sram memory\n");

  switch (targetARCH)
  {
    default: // ATC 6.24 compliant
    case ARCH_UNKNOWN:
      smnt = "/sram";
      break;

    case ARCH_83XX:
    case ARCH_82XX:
      smnt = "/r0";
      break;
  }

  /*
  *
  * See if SRAM filesystem is present in /proc/mounts for later remounting
  *
  */

  errno = 0;
  found = false;
  mntFp = setmntent(mounts, "r");

  if (mntFp == NULL)
  {
    fitPrint(VERBOSE, "\tfailed to open %s: %s\n",mounts,strerror(errno));
    fitPrint(VERBOSE, "\tSRAM memory test will be skipped without error.\n");
  }
  else
  {
    fitPrint(VERBOSE, "\tunmounting %s for test\n", smnt);

    while ((feof(mntFp) == 0) && !found)
    {
      if (getmntent_r(mntFp, &mnt_info, mnt_buf, (int)sizeof(mnt_buf)) != NULL)
      {
        if (strcmp(mnt_info.mnt_dir, smnt) == 0)
        {
          found = true;
        }
      }
    }

    (void) endmntent(mntFp);

    /*
    *
    * Attempt to unmount SRAM filesystem so test Linux filesystem manager
    * doesn't inadvertently corrupt the test and so the test doesn't
    * corrupt the filesystem.
    *
    */

    if (!found)
    {
      fitPrint(ERROR, "\tunmount aborted: failed to find %s in %s\n",
               smnt,mounts);
      ftUpdateTestStatus(ftrp,ftError,NULL);
    }
    else
    {
      sm = mnt_info.mnt_fsname;
      iresult = umount(smnt);
      if (iresult == -1)
      {
        fitPrint(VERBOSE,"\t%s did not unmount, %s\n",smnt,strerror(errno));
        fitPrint(VERBOSE,"\tremount will not be attempted\n");
        remount = false;
      }
      else
      {
        fitPrint(VERBOSE, "\t%s unmounted from %s ok\n",smnt,sm);
      }

      (void)smtest(sm);

      if (remount)
      {
        iresult = mount(sm,smnt,mnt_info.mnt_type,0,mnt_buf);

        if (iresult == -1)
        {
          fitPrint(VERBOSE, "\tremount of %s on %s failed: %s\n",
                   smnt, sm, strerror(errno));
        }
        else
        {
          fitPrint(VERBOSE, "\tremount of %s on %s succeeded\n",
                   smnt, sm);
        }
      }
      else
      {
        fitPrint(VERBOSE, "\tnot remounting %s on %s\n",smnt, sm);
      }
    } // else <found>
  } // else <mntFp != NULL>

  return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
}
