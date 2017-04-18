/******************************************************************************
                                    fsFit.c

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
/*
*
* fsFit
* A set of basic filesystem tests.
*
*/

#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/mount.h>

#include "fit.h"

#define FSIZE 4096


/*
*
* Flash and DRAM filesystem test entry
*
*/

ftRet_t fsFit(plint argc, char * const argv[], const char * const dirNames[])
{
  int32   fsFd;
  off_t   fsOffset;
  ssize_t bCnt;
  size_t  fsSize;
  void    *mp, *mp_orig;
  const char * const * dp;
  const char *cp;
  const char *procKcore = "/proc/kcore"; // data source
  char fullPathFileName[200];
  ftRet_t ret = ftFail;

  fsFd = open(procKcore,O_RDWR);
  if(fsFd == -1){
    fitPrint(ERROR, "%s: cannot open %s, err %d,%s\n",
             argv[0],procKcore,errno,strerror(errno));
    return(ftError);
  }

  fsSize = FSIZE;

  /*
  *
  * Malloc receive buffers.
  *
  */

  /* buffer used to store the unmodified data contents */
  mp_orig = malloc(fsSize);
  if(mp_orig == NULL){
    fitPrint(ERROR, "%s: cannot malloc %u bytes, err %d, %s\n",
             argv[0],fsSize,errno,strerror(errno));
    close(fsFd);
    return(ftError);
  }

  /* buffer used for comparision testing of data */
  mp = malloc(fsSize);
  if(mp == NULL){
    fitPrint(ERROR, "%s: cannot malloc %u bytes, err %d, %s\n",
             argv[0],fsSize,errno,strerror(errno));
    free(mp_orig);
    close(fsFd);
    return(ftError);
  }

  fsOffset = lseek(fsFd,0,SEEK_SET);

  if(fsOffset != 0){
    fitPrint(ERROR, "%s cannot seek to head of file %s, err %d: %s\n",
             argv[0],procKcore,errno,strerror(errno));
    ret = ftError;
    free(mp_orig);
    free(mp);
    close(fsFd);
    return(ret);
  }

  /*
  *
  * Read /proc/kcore as data source and close
  *
  */

  bCnt = read(fsFd,mp_orig,fsSize);

  if((size_t)bCnt != fsSize){
    if(bCnt == -1){
      fitPrint(ERROR, "%s cannot read from %s, err %d: %s\n",
               argv[0],procKcore,errno,strerror(errno));
      ret = ftError;
    } else {
      fitPrint(ERROR, "%s failed reading from %s, expected %u actual %d\n",
               argv[0],procKcore,fsSize,bCnt);
      ret = ftFail;
    }
    free(mp_orig);
    free(mp);
    close(fsFd);
    return(ret);
  }

  close(fsFd);

  /*
  *
  * Open files, write the random data and close.
  *
  */
  dp = dirNames;

  for(cp = *dp; *cp != '\0'; cp = *dp){
    fullPathFileName[0] = '\0';
    strcat(fullPathFileName,cp);
    strcat(fullPathFileName,"/fitFile"); // create full pathname to file under test
    fitPrint(VERBOSE, "%s: %s\n",argv[0],fullPathFileName);

    //lint -e{9027} MISRA hates bit ops on signed, but can't be helped
    fsFd = open (fullPathFileName,O_CREAT|O_APPEND|O_WRONLY);

    if(fsFd == -1){
      fitPrint(ERROR, "%s test cannot open %s, err %d: %s\n",
               argv[0],fullPathFileName,errno,strerror(errno));
      free(mp_orig);
      free(mp);
      return(ftError);
    }

    bCnt = write(fsFd,mp_orig,fsSize);

    if((size_t)bCnt != fsSize){
      if(bCnt == -1){
        fitPrint(ERROR, "%s cannot write to %s, err %d: %s\n",
                 argv[0],fullPathFileName,errno,strerror(errno));
        ret = ftError;
      } else {
        fitPrint(ERROR, "%s failed writing to %s, expected %u actual %d\n",
                 argv[0],fullPathFileName,fsSize,bCnt);
        ret = ftFail;
      }
      free(mp_orig);
      free(mp);
      close(fsFd);
      return(ret);
    }

    close(fsFd);
    dp++;
  }

  /*
  *
  * Open files, read files, compare original random data, close and remove
  *
  */

  dp = dirNames;
  for(cp = *dp; *cp != '\0'; cp = *dp){
    fullPathFileName[0] = '\0';
    strcat(fullPathFileName,cp);
    strcat(fullPathFileName,"/fitFile"); // create full pathname to file under test
    fitPrint(VERBOSE, "%s: %s\n",argv[0],fullPathFileName);

    fsFd = open (fullPathFileName,O_RDWR);
    if(fsFd == -1){
      fitPrint(ERROR, "%s test cannot open %s, err %d: %s\n",
               argv[0],fullPathFileName,errno,strerror(errno));
      free(mp_orig);
      free(mp);
      return(ftError);
    }

    memset(mp,0,fsSize);// clear out memory

    bCnt = read(fsFd,mp,fsSize);

    if((size_t)bCnt != fsSize){
      if(bCnt == -1){
        fitPrint(ERROR, "%s cannot read from %s, err %d: %s\n",
                 argv[0],fullPathFileName,errno,strerror(errno));
        ret = ftError;
      } else {
        fitPrint(ERROR, "%s failed reading from %s, expected %u actual %d\n",
                 argv[0],fullPathFileName,fsSize,bCnt);
        ret = ftFail;
      }
      free(mp_orig);
      free(mp);
      close(fsFd);
      return(ret);
    }

    bCnt = memcmp(mp_orig,mp,fsSize);

    if(bCnt == 0){
      ret = ftPass;
    } else {
      fitPrint(ERROR, "%s: %s is corrupt\n",argv[0],fullPathFileName);
      ret = ftFail;
    }

    close(fsFd);
    unlink(fullPathFileName);// remove file
    dp++;
  }

  free(mp_orig);
  free(mp);
  return(ret);
}


/* int32 mountMedia(const char *target)
 *
 * target should be /mnt/usb or /mnt/sd
 * returns 0 on success, -1 on failure
 *
 * Process:
 * Read /sys/block and look for usb2 or usb1 in the symlinks.
 * Get the sdx name.
 * Look up that name in /dev with the partition number.
 * Try mounting all those that match the sdx name until it succeeds.
 */
static int32 mountMedia(const char *target)
{
  int32 retVal = 0;
  ssize_t targetLen;
  DIR * dir;
  struct dirent *blockDirItem;
  char symlinkTarget[512];
  char symlinkPath[256];
  char devBaseName[128] = {'\0'};
  char identifier[128];
  bool success = false;

  /* make sure the target isn't already mounted */
  if (umount(target) == -1)
  {
    if (errno != EINVAL)
    {
      /* We expect a failure if the target was not
       * ever mounted. For any other reason, abort */
      fitPrint(ERROR, "%s: can't umount %s, err %d %s\n",__func__,target,errno,
               strerror(errno));
      return -1;
    }
  }

  dir = opendir("/sys/block");

  if (dir == NULL)
  {
    fitPrint(ERROR, "Critical: can't open /sys/block\n");
    retVal = -1;
  }
  else
  {
    switch (targetARCH)
    {
      case ARCH_82XX:
        /* determine what to look for in /sys/block */
        if (strstr(target, "usb") != NULL)
        {
          /* flash drive */
          strcpy(identifier, "usb1");
        } else {
          /* SD device */
          strcpy(identifier, "usb2");
        }
        break;
      case ARCH_83XX:
        /* determine what to look for in /sys/block */
        if (strstr(target, "usb") != NULL)
        {
          /* flash drive */
          strcpy(identifier, "usb1");
        } else {
          /* SD device */
          strcpy(identifier, "mmcblk");
        }
        break;
      case ARCH_UNKNOWN:
      default:
        retVal = -1;
        break;
    }
  }

  if (retVal == -1)
  {
    return retVal;
  }

  fitPrint(VERBOSE, "mountMedia: identifier %s, target %s\n",identifier,target);

  /* go through the items in /sys/block and find a
   * symlink who's target has identifier in it.
   */
  blockDirItem = readdir(dir);

  while (blockDirItem != NULL)
  {
    fitPrint(VERBOSE, "mountMedia: identifier %s, d_name %s\n",
             identifier,blockDirItem->d_name);
    if (blockDirItem->d_type == (u_int8)DT_LNK) {
      /* we found a symlink; now see if it has the right identifier (mmcblkx) */
      snprintf(symlinkPath, sizeof(symlinkPath), "/sys/block/%s",
               blockDirItem->d_name);

      targetLen = readlink(symlinkPath, symlinkTarget, sizeof(symlinkTarget));
      if (targetLen > 0)
      {
        symlinkTarget[targetLen] = '\0'; // readlink does not append '\0'
        fitPrint(VERBOSE, "mountMedia: symlinkPath %s,\n" \
                 "\t\tsymlinkTarget %s,\n" \
                 "\t\tidentifier %s\n",
                 symlinkPath,symlinkTarget,identifier);
        if (strstr(symlinkTarget, identifier) != NULL)
        {
          /* we've found what we're looking for */
          strncpy(devBaseName, blockDirItem->d_name, sizeof(devBaseName));
          fitPrint(VERBOSE, "mountMedia: found match path %s, baseName %s\n",
                   symlinkPath,devBaseName);
          success = true;
          break;
        }
      }
    }
    blockDirItem = readdir(dir);
  }

  closedir(dir);

  if (success)
  {
    success = false;

    /* now look in /dev for the actual device */
    dir = opendir("/dev");

    if (dir == NULL)
    {
      fitPrint(ERROR, "Critical: cant open /dev\n");
      return -1;
    }

    blockDirItem = readdir(dir);

    while (blockDirItem != NULL)
    {
      /* find the device with devBaseName in it, but make sure
       * it's a partitioned device.
       */
      char *devName = blockDirItem->d_name;

      if (strstr(devName, devBaseName) != NULL)
      {
        /* make sure it has a partition number at the end */
        char lastChar = devName[strlen(devName) - 1u];

        if (isdigit(lastChar))
        {
          /* mount it */
          char devPath[128];

          snprintf(devPath, sizeof(devPath), "/dev/%s", devName);

          fitPrint(VERBOSE, "mountMedia: mounting %s on %s\n",devPath,target);
          if (mount (devPath, target, "vfat", MS_SYNCHRONOUS,NULL) < 0)
          {
            /* Don't give up just yet, this could be the other partitions,
             * which would NOT succeed (sdb2, sdb3, ...)
             */
            fitPrint(VERBOSE, "mountMedia: failed to mount %s on %s, trying another...\n",
                     devPath,target);
          }
          else
          {
            /* mounted the proper device! */
            fitPrint(VERBOSE, "mountMedia: mounted %s on %s\n",devPath,target);
            success = true;
            break;
          }
        }
      }
      blockDirItem = readdir(dir);
    }

    closedir(dir);
  }

  if (!success)
  {
    fitPrint(ERROR, "Could not find device. Check if present.\n");
    return -1;
  }

  return 0;
}

/*
*
* Removable media (USB and SDHC card) filesystem test entry
*
*/

ftRet_t mediaFit(plint argc,char * const argv[], const char * const dirNames[])
{
  /* Ensure that the removable device is mounted prior to
   * calling fsFit for the sd card.
   */

  ftRet_t ret = ftFail;

  fitPrint(VERBOSE, "%s: looking for %s\n",__func__,*dirNames);

  if(mountMedia(dirNames[0]) == 0){
    ret = fsFit(argc,argv,dirNames);
  }

  return ret;
}
