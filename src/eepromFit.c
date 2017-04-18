/******************************************************************************
                                  eepromFit.c

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
* This software provides a non-destructive functional test of the serial EEPROM
* device on the ATC-1C carrier board.  The device is presumed to be the minimum
* size, 256 bytes.
*
* The test is aware of the ATC board inventory layout and will preserve a good
* data layout while also providing a utility to initialize good default data.
* The test layout and default data will reflect Siemens ATC 5.2b usage.
*
*/

#include <stddef.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h> /* POSIX Threads */
#include <semaphore.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "fit.h"
#include "eepromFit.h"

/*
*
* ATC 5.2b EEPROM header operations
*
*/

#define EE_INIT        0x0001u
#define EE_DEFAULT_HDR 0x0002u
#define EE_EDIT_HDR    0x0004u
#define EE_LOAD_HDR    0x0008u
#define EE_SAVE_HDR    0x0010u
#define EE_VERIFY_HDR  0x0020u
#define EE_PRINT_HDR   0x0040u

static ftResults_t ftResults = {0};
static ftResults_t *ftrp = &ftResults;
static size_t eeSize;

/*
*
* Initialize a default header
*
*/

static void eeDefaultHdr(void *vp)
{
  /*
  *
  * initialize Siemens default EEPROM data
  *
  */

  // Module 1 - Host
  mod_v module_version1 =
  {
    .year_bcd             = 0x2014,
    .mon_bcd              = 1,
    .day_bcd              = 1,
    .major_bcd            = 1,
    .minor_bcd            = 1,
    .patch_bcd            = 0
  };

  mod_d module_description1 =
  {
    .location             = 2, // host
    .make                 = 3, // Siemens
    .model                = "ATC-M60", // 7 chars max + nul
    //.version
    .type                 = 2 // hardware
  };


  // Module 2 - Display
  mod_v module_version2 =
  {
    .year_bcd             = 0x2014,
    .mon_bcd              = 1,
    .day_bcd              = 1,
    .major_bcd            = 1,
    .minor_bcd            = 1,
    .patch_bcd            = 0
  };

  mod_d module_description2 =
  {
    .location             = 3, // Display
    .make                 = 3, // Siemens
    .model                = "ATC-16L", // ATC 16-line display
    //.version
    .type                 = 2 // hardware
  };


  // Module 3 - Field I/O
  mod_v module_version3 =
  {
    .year_bcd             = 0x2014,
    .mon_bcd              = 1,
    .day_bcd              = 1,
    .major_bcd            = 1,
    .minor_bcd            = 1,
    .patch_bcd            = 0
  };

  mod_d module_description3 =
  {
    .location             = 1, // Other
    .make                 = 3, // Siemens
    .model                = "M52-USB", // NEMA Field I/O
    //.version
    .type                 = 2 // hardware
  };


  // Module 4 - Slot A2
  mod_v module_version4 =
  {
    .year_bcd             = 0x2014,
    .mon_bcd              = 1,
    .day_bcd              = 1,
    .major_bcd            = 1,
    .minor_bcd            = 1,
    .patch_bcd            = 0
  };

  mod_d module_description4 =
  {
    .location             = 6, // Slot A2
    .make                 = 3, // Siemens
    .model                = "A2-Comm", // Ethernet/USB/Datakey/SP8 expansion
    //.version
    .type                 = 2 // hardware
  };


   // Both Ethernet Phys
  eth_d ethernet_device =
  {
    .type                 = 4,
    .ip_addr              = 0,
    .mac_addr             = {0},
    .ip_mask              = 0,
    .ip_gate              = 0,
    .interface            = 2
  };

  port_d port_descriptor =
  {
    .id                   =      0,
    .mode                 =      0,
    .speed                =      0
  };

  eeprom_v1 eepromDefault =
  {
    .version              =      1, // ATC v1 header (ATC 5.2b)
    .size                 =    203, // Beginning through CRC1
    .modules              =      4, // always
    //mod_d
    .t_lines              =     16, // 16-line display
    .t_cols               =     40,
    .g_rows0              =      0,
    .g_cols0              =      0,
    .ethdevs              =      2, // always
    //eth_d
    .spi3use              =      2, // Ethswitch 1
    .spi4use              =      1, // Ethswitch 0
    .ports_used.ports     = 0x07e5, // sp: 8s, 8, 6, 5s, 4, 3s, 2, 1
    .ports                =     11, // always
    //port_d
    .ports_pres.ports     = 0x07e5, // sp: 8s, 8, 6, 5s, 4, 3s, 2, 1
    .sb1.ports            = 0x0080, // sp5s
    .sb2.ports            = 0x0020, // sp3s
    .ts2.ports            = 0x0020, // sp3s
    .expbus               =      0,
    //.crc1
    .latitude             =   0.0f,
    .longitude            =   0.0f,
    .contr_id             = 0xffff,
    .com_drop             = 0xffff,
    .res_agency           = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                             0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                             0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                             0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                             0xff, 0xff, 0xff},
    //.crc2
    .user_data            = {0xff,0xff,0xff,0xff}
  };

  /*
  *
  * Module Data
  *
  */
  memcpy(&module_description1.version,&module_version1,sizeof(mod_v));
  memcpy(&module_description2.version,&module_version2,sizeof(mod_v));
  memcpy(&module_description3.version,&module_version3,sizeof(mod_v));
  memcpy(&module_description4.version,&module_version4,sizeof(mod_v));

  memcpy(&eepromDefault.module[0],&module_description1,sizeof(mod_d));
  memcpy(&eepromDefault.module[1],&module_description2,sizeof(mod_d));
  memcpy(&eepromDefault.module[2],&module_description3,sizeof(mod_d));
  memcpy(&eepromDefault.module[3],&module_description4,sizeof(mod_d));

  /*
  *
  * Ethernet Data
  *
  */

  memcpy(&eepromDefault.ethdev[0],&ethernet_device,sizeof(eth_d));
  memcpy(&eepromDefault.ethdev[1],&ethernet_device,sizeof(eth_d));

  /*
  *
  * Port Data
  *
  */

  memcpy(&eepromDefault.port[ 0],&port_descriptor,sizeof(port_d));
  memcpy(&eepromDefault.port[ 1],&port_descriptor,sizeof(port_d));
  memcpy(&eepromDefault.port[ 2],&port_descriptor,sizeof(port_d));
  memcpy(&eepromDefault.port[ 3],&port_descriptor,sizeof(port_d));
  memcpy(&eepromDefault.port[ 4],&port_descriptor,sizeof(port_d));
  memcpy(&eepromDefault.port[ 5],&port_descriptor,sizeof(port_d));
  memcpy(&eepromDefault.port[ 6],&port_descriptor,sizeof(port_d));
  memcpy(&eepromDefault.port[ 7],&port_descriptor,sizeof(port_d));
  memcpy(&eepromDefault.port[ 8],&port_descriptor,sizeof(port_d));
  memcpy(&eepromDefault.port[ 9],&port_descriptor,sizeof(port_d));
  memcpy(&eepromDefault.port[10],&port_descriptor,sizeof(port_d));

  eepromDefault.ports_used.ports = 0x7e5;

  eepromDefault.port[ 0].id = 1;
  eepromDefault.port[ 0].mode = 1;
  eepromDefault.port[ 0].speed = 9600;

  eepromDefault.port[ 1].id = 2;
  eepromDefault.port[ 1].mode = 1;
  eepromDefault.port[ 1].speed = 9600;

  eepromDefault.port[ 2].id = 3;
  eepromDefault.port[ 2].mode = 3;
  eepromDefault.port[ 2].speed = 153600;

  eepromDefault.port[ 3].id = 4;
  eepromDefault.port[ 3].mode = 1;
  eepromDefault.port[ 3].speed = 115200;

  eepromDefault.port[ 4].id = 5;
  eepromDefault.port[ 4].mode = 3;
  eepromDefault.port[ 4].speed = 614400;

  eepromDefault.port[ 5].id = 6;
  eepromDefault.port[ 5].mode = 1;
  eepromDefault.port[ 5].speed = 38400;

  eepromDefault.port[ 6].id = 8;
  eepromDefault.port[ 6].mode = 1;
  eepromDefault.port[ 6].speed = 9600;

  eepromDefault.port[ 7].id = 9;
  eepromDefault.port[ 7].mode = 0;
  eepromDefault.port[ 7].speed = 0;

  eepromDefault.port[ 8].id = 10;
  eepromDefault.port[ 8].mode = 0;
  eepromDefault.port[ 8].speed = 0;

  eepromDefault.port[ 9].id = 12;
  eepromDefault.port[ 9].mode = 2;
  eepromDefault.port[ 9].speed = 0;

  eepromDefault.port[10].id = 13;
  eepromDefault.port[10].mode = 0;
  eepromDefault.port[10].speed = 0;

  memset(eepromDefault.res_agency,0xff,sizeof(eepromDefault.res_agency));
  memset(eepromDefault.user_data,0xff,sizeof(eepromDefault.user_data));

  /*
  *
  * Generate CRCs
  *
  */

  eepromDefault.crc1 = genCrc(&eepromDefault,&eepromDefault.crc1);
  eepromDefault.crc2 = genCrc(&eepromDefault.latitude,&eepromDefault.crc2);

  mdmp((char *)&eepromDefault,sizeof(eeprom_v1), VERBOSE);

  memcpy(vp,&eepromDefault,sizeof(eeprom_v1));
}

/*
*
* Get user input
*
*/

static char *get_line(char *buf,size_t len,FILE *fp)
{
  char *p;
  size_t last;

  p=fgets(buf,(int32)len,fp);

  if(p != NULL){
    last = strlen(buf) - 1u;
    if(buf[last] == '\n'){
      buf[last] = '\0';
    } else {
      fscanf(fp,"%*[^\n]");
      (void) fgetc(fp);
    }
  }
  return(p);
}


/*
*
* Convert binary to 2-digit BCD
*
*/
static u_int8 to2digitBCD(u_int32 input)
{
  u_int8 result;
  u_int8 binary;

  binary = (u_int8)input;
  result = ((binary%100u)/10u) <<4u;
  result += (binary%10u);
  return result;
}


/*
*
* Convert binary to 4-digit BCD
*
*/
static u_int16 to4digitBCD(u_int32 input)
{
  u_int16 result;
  u_int16 binary;

  binary = (u_int16)input;
  result =  ((binary%10000u)/1000u) <<4u;
  result += ((binary%1000u)/100u) <<4u;
  result += ((binary%100u)/10u) <<4u;
  result += (binary%10u);
  return result;
}


/*
*
* Get's and Set's for header members
*
*/

static int32  getVersion    (const char *cp,const eeprom_v1 *eehp)
{
  char lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};

  sprintf(lbuf,"%s: ",cp); // copy member name

  switch(eehp->version){
    case 1:
      strcat(lbuf,"v5.2");
      break;
    case 2:
      strcat(lbuf,"v6.10");
      break;
    default:
      strcat(lbuf,"unexpected");
      break;
  }

  fitPrint(USER, "%s %d\n",lbuf,eehp->version);

  return((int32)eehp->version);
}

static void setVersion    (const char *cp,eeprom_v1 *eehp)
{
  char lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  char *keypress;

  fitPrint(USER, "Select number for %s:\n",cp);

  fitPrint(USER, "\t1: v5.2\n\t2: v6.10\n-> ");

  keypress = get_line(lbuf,sizeof(lbuf),stdin);

  switch(*keypress){
    case '1':
    case '2':
      eehp->version = (u_int8)strtoul(keypress,NULL,10);
      break;
    default:
      fitPrint(USER, "unexpected %s\n",keypress);
      break;
  }
}

static int32  getSize       (const char *cp,const eeprom_v1 *eehp)
{ // Size of data from beginning of EEPROM through CRC1
  char lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};

  sprintf(lbuf,"%s: ",cp); // copy member name

  switch(eehp->size){
    case 0:
      strcat(lbuf,"default");
      break;
    default:
      // actual size is programmed
      break;
  }

  fitPrint(USER, "%s %u, header %u\n",lbuf,eehp->size,sizeof(eeprom_v1));

  return((int32)eehp->size);
}

static void setSize       (const char *cp,eeprom_v1 *eehp)
{ // Size of data from beginning of EEPROM through CRC1
  char        lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;
  u_int16     dsize;

  dsize = (u_int16)(offsetof(eeprom_v1,crc1) + sizeof(eehp->crc1)); // default size

  if (eehp->size == 0u)
  {
    fitPrint(USER, "%s: Size to CRC1 is currently default: %u\n-> ",cp,eehp->size);
  }
  else
  {
    fitPrint(USER, "%s: Size to CRC1 is currently: %u\n-> ",cp,eehp->size);
  }

  fitPrint(USER, "Enter new size or <cr> to use recommended size %u):\n-> ",dsize);
  keypress = get_line(lbuf,sizeof(lbuf),stdin);
  if (*keypress == '\0')
  {
    eehp->size = dsize;
  }
  else
  {
    eehp->size = (u_int16)strtoul(keypress,NULL,10);
  }
}

static int32  getModules    (const char *cp,const eeprom_v1 *eehp)
{
  fitPrint(USER, "The number of modules is fixed at %d\n",eehp->modules);
  return((int32)eehp->modules); // always 4
  //lint --e{715}  pointer cp must be passed in but is not referenced.
}

static void setModules    (const char *cp,eeprom_v1 *eehp)
{
  fitPrint(USER, "The number of modules is fixed at 4\n");
  eehp->modules = 4;
  //lint --e{715}  pointer cp must be passed in but is not referenced.
}

static int32  getModule     (const char *cp,const eeprom_v1 *eehp)
{
  char        lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char  *keypress;
  int32        idx;
  bool         loopOn = true;
  const mod_d *mdp;
  const mod_v *mvp;

  fitPrint(USER, "%s: \n",cp);

  while(loopOn)
  {
    fitPrint(USER, "Enter module index for %s:\n\t0:\n\t1:\n\t2:\n\t3:\n" \
                   "\t*: any other number to quit\n-> ",cp);

    keypress = get_line(lbuf,sizeof(lbuf),stdin);

    idx = strtol(keypress,NULL,10);
    if (idx >= 4)
    {
      loopOn = false;
      continue;
    }

    mdp = &eehp->module[idx];

    fitPrint(USER, "Module %ld Description:\n",idx);

    fitPrint(USER, "location: ");
    switch(mdp->location){
      case 1:
        fitPrint(USER, "Other\n");
        break;
      case 2:
        fitPrint(USER, "Host\n");
        break;
      case 3:
        fitPrint(USER, "Display\n");
        break;
      case 4:
        fitPrint(USER, "Power Supply\n");
        break;
      case 5:
        fitPrint(USER, "Slot A1\n");
        break;
      case 6:
        fitPrint(USER, "Slot A2\n");
        break;
      default:
        fitPrint(USER, "unrecognized %d\n",mdp->location);
        break;
    }

    fitPrint(USER, "make: NEMA NTCIP Mfg ID %d",mdp->make);
    switch(mdp->make){
      case 1:
        fitPrint(USER, " (Peek)");
        break;
      case 2:
        fitPrint(USER, " (Naztec)");
        break;
      case 3:
        fitPrint(USER, " (Siemens[Eagle])");
        break;
      case 5:
        fitPrint(USER, " (Econolite)");
        break;
      case 17:
        fitPrint(USER, " (Siemens[Gardner])");
        break;
      case 21:
        fitPrint(USER, " (McCain)");
        break;
      case 36:
        fitPrint(USER, " (Intelite)");
        break;
      case 39:
        fitPrint(USER, " (4D)");
        break;
      case 43:
        fitPrint(USER, " (Trafficware)");
        break;
      default:
        fitPrint(USER, " (unknown)");
        break;
    }
    fitPrint(USER, "\n");

    fitPrint(USER, "model: %s\n",mdp->model);

    mvp = &mdp->version;
    fitPrint(USER, "version: %x, ",mvp->year_bcd);

    switch(mvp->mon_bcd){
      case 1:
        fitPrint(USER, "January ");
        break;
      case 2:
        fitPrint(USER, "February ");
        break;
      case 3:
        fitPrint(USER, "March ");
        break;
      case 4:
        fitPrint(USER, "April ");
        break;
      case 5:
        fitPrint(USER, "May ");
        break;
      case 6:
        fitPrint(USER, "June ");
        break;
      case 7:
        fitPrint(USER, "July ");
        break;
      case 8:
        fitPrint(USER, "August ");
        break;
      case 9:
        fitPrint(USER, "September ");
        break;
      case 0x10:
        fitPrint(USER, "October ");
        break;
      case 0x11:
        fitPrint(USER, "November ");
        break;
      case 0x12:
        fitPrint(USER, "December ");
        break;
      default:
        fitPrint(USER, "invalid(%x) ",mvp->mon_bcd);
        break;
    }

    switch(mvp->mon_bcd){
      case 2:
        if((mvp->day_bcd != 0x00u) && (mvp->day_bcd <= 0x29u)){ // for leap years
          fitPrint(USER, "%x\n",mvp->day_bcd);
        } else {
          fitPrint(USER, "invalid(%x)\n",mvp->day_bcd);
        }
        break;
      case 4:
      case 6:
      case 9:
      case 0x11:
        if((mvp->day_bcd != 0x00u) && (mvp->day_bcd <= 0x30u)){
          fitPrint(USER, "%x\n",mvp->day_bcd);
        } else {
          fitPrint(USER, "invalid(%x)\n",mvp->day_bcd);
        }
        break;
      case 1:
      case 3:
      case 5:
      case 7:
      case 8:
      case 0x10:
      case 0x12:
        if((mvp->day_bcd != 0x00u) && (mvp->day_bcd <= 0x31u)){
          fitPrint(USER, "%x\n",mvp->day_bcd);
        } else {
          fitPrint(USER, "invalid(%x)\n",mvp->day_bcd);
        }
        break;
      default:
          fitPrint(USER, "%x\n",mvp->day_bcd);
        break;
    }

    if((mdp->make == 3u) && (mdp->type == 2u)){
      // make==3(Siemens), type=2(Hardware)
      fitPrint(USER, "major_bcd: ");
      fitPrint(USER, "Hdwe Cfg Px%02x\n",mvp->major_bcd);
      fitPrint(USER, "minor_bcd: ");
      fitPrint(USER, "Hdwe Rev %x\n",mvp->minor_bcd);
      fitPrint(USER, "patch_bcd: ");
      fitPrint(USER, "Hdwe Spc %x ",mvp->patch_bcd);

      if (mdp->location == 2u)
      { //Host board
        switch (mvp->patch_bcd){
          case 0:
            fitPrint(USER, "(M60-ATC)");
            break;
          case 1:
            fitPrint(USER, "(ATCnx-2)");
            break;
          case 2:
            fitPrint(USER, "(2070-1C)");
            break;
          default:
            fitPrint(USER, "(unknown)");
            break;
        }
      }
      else if (mdp->location == 6u)
      { // Comm expansion in A2, etc.
        switch (mvp->patch_bcd){
          case 0:
            fitPrint(USER, "(4 Enet, 4 USB)");
            break;
          case 1:
            fitPrint(USER, "(2 Enet, 1 USB)");
            break;
          case 2:
            fitPrint(USER, "(3 Enet, 1 USB)");
            break;
          default:
            fitPrint(USER, "(unknown)");
            break;
        }
      }
      else
      { // Other type of Siemens hardware
        fitPrint(USER, "major_bcd: %x\n",mvp->major_bcd);
        fitPrint(USER, "minor_bcd: %x\n",mvp->minor_bcd);
        fitPrint(USER, "patch_bcd: %x\n",mvp->patch_bcd);
      }
    }
    else
    { // not Siemens Hardware
      fitPrint(USER, "major_bcd: %x\n",mvp->major_bcd);
      fitPrint(USER, "minor_bcd: %x\n",mvp->minor_bcd);
      fitPrint(USER, "patch_bcd: %x\n",mvp->patch_bcd);
    }

    fitPrint(USER, "\n");

    fitPrint(USER, "type: ");
    switch(mdp->type){
      case 1:
        fitPrint(USER, "%u - Other\n",mdp->type);
        break;
      case 2:
        fitPrint(USER, "%u - Hardware\n",mdp->type);
        break;
      case 3:
        fitPrint(USER, "%u - Soft/Firmware\n",mdp->type);
        break;
      default:
        fitPrint(USER, "%u - invalid\n",mdp->type);
        break;
    }
  }

  return(0);
}

static void setModule     (const char *cp,eeprom_v1 *eehp)
{
  char        lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;
  u_int32     idx;
  mod_d      *mdp;
  mod_v      *mvp;
  bool        loopOn = true;

  fitPrint(USER, "%s\n",cp);

  while(loopOn)
  {
    fitPrint(USER, "Enter module index for %s:\n\t0:\n\t1:\n\t2:\n\t3:\n" \
                   "\t*: any other number to quit\n-> ",cp);
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    idx = strtoul(keypress,NULL,10);
    if (idx >= 4u)
    {
      fitPrint(USER, "Invalid Module number: %lu\n",idx);
      loopOn = false;
      continue;
    }

    mdp = &eehp->module[idx];

    fitPrint(USER, "Enter location:\n\t1: Other\n\t2: Host\n\t3: Display\n" \
                   "\t4: Power Supply\n\t5: Slot A1\n\t6: Slot A2\n-> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    idx = strtoul(keypress,NULL,10);
    if (idx <= 6u)
    {
      mdp->location = (u_int8)idx;
    }
    else
    {
      fitPrint(USER, "unrecognized location %lu\n",idx);
      loopOn = false;
      continue;
    }

    fitPrint(USER, "Enter make:\n\t*: NEMA NTCIP Mfg ID\n\t3: ETCS\n-> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    mdp->make = (u_int8)strtoul(keypress,NULL,10);

    fitPrint(USER, "Enter model:\n-> ");
    memset(lbuf,0x20,sizeof(lbuf));// set to spaces
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    strncpy(mdp->model,keypress,7);

    mvp = &mdp->version;

    fitPrint(USER, "Enter version year:\n" \
                   "\t1: 2017: 2070-1C host AAD15753-002\n" \
                   "\t2: 2014: ATC host, M52-USB FIO\n" \
                   "\t3: 2009: all TEES 2009 (-1E, -2E, etc)\n" \
                   "\t4: 2007: 2070-1C host AAD15753P001\n" \
                   "\t5: 2004: all TEES 2004 (-2B, -2N, etc)\n" \
                   "\tor 4-digit year -> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    idx = strtoul(keypress,NULL,10);

    switch(idx){
      case 1:
        mvp->year_bcd = 0x2017;
        break;
      case 2:
        mvp->year_bcd = 0x2014;
        break;
      case 3:
        mvp->year_bcd = 0x2009;
        break;
      case 4:
        mvp->year_bcd = 0x2007;
        break;
      case 5:
        mvp->year_bcd = 0x2004;
        break;
      default:
        mvp->year_bcd = to4digitBCD(idx);
        break;
    }
    fitPrint(USER, ", ");

    fitPrint(USER, "Enter version month:\n\t 1: January\n\t 2: February\n\t 3: March \
                                \n\t 4: April\n\t 5: May\n\t 6: June\n\t 7: July \
                                \n\t 8: August\n\t 9: September\n\t16: October \
                                \n\t17: November\n\t18: December\n-> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    idx = strtoul(keypress,NULL,10);

    switch(idx){
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
      case 0x10:
      case 0x11:
      case 0x12:
        mvp->mon_bcd = to2digitBCD(idx);
      break;
      default:
        fitPrint(USER, "invalid month %lx",idx);
      break;
    }

    fitPrint(USER, "Enter version day:\n\t 1:  1\n\t 2:  2\n\t 3:  3\n\t 4:  4\n\t 5:  5\n\t 6:  6 \
                              \n\t 7:  7\n\t 8:  8\n\t 9:  9\n\t16: 10\n\t17: 11\n\t18: 12 \
                              \n\t19: 13\n\t20: 14\n\t21: 15\n\t22: 16\n\t23: 17\n\t24: 18 \
                              \n\t25: 19\n\t32: 20\n\t33: 21\n\t34: 22\n\t35: 23\n\t36: 24 \
                              \n\t37: 25\n\t38: 26\n\t39: 27\n\t40: 28\n\t41: 29\n\t48: 30\n\t49: 31\n-> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    idx = strtoul(keypress,NULL,10);

    switch(mvp->mon_bcd){
      case 0x02: //February
        if((idx>0u) && (idx <= 0x29u)){ // for leap years
          mvp->day_bcd = to2digitBCD(idx);
        } else {
          fitPrint(USER, "invalid day %lx\n",idx);
        }
        break;
      case 0x04: //April
      case 0x06: //June
      case 0x09: //September
      case 0x11: //November
        if((idx>0u) && (idx <= 0x30u)){
          mvp->day_bcd = to2digitBCD(idx);
        } else {
          fitPrint(USER, "invalid day %lx\n",idx);
        }
        break;
      case 0x01: //January
      case 0x03: //March
      case 0x05: //May
      case 0x07: //July
      case 0x08: //August
      case 0x10: //October
      case 0x12: //December
        if((idx>0u) && (idx <= 0x31u)){
          mvp->day_bcd = to2digitBCD(idx);
        } else {
          fitPrint(USER, "invalid day %lx\n",idx);
        }
        break;
      default:
        fitPrint(USER, "\n");
        break;
    }

    fitPrint(USER, "Enter two digit major version number \n-> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    idx = strtoul(keypress,NULL,10);
    mvp->major_bcd = to2digitBCD(idx);

    fitPrint(USER, "Enter two digit minor version number\n-> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    idx = strtoul(keypress,NULL,10);
    mvp->minor_bcd = to2digitBCD(idx);

    fitPrint(USER, "Enter two digit patch version number\n-> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    idx = strtoul(keypress,NULL,10);
    mvp->patch_bcd = to2digitBCD(idx);


    fitPrint(USER, "Enter type:\n\t1: Other\n\t2: Hardware\n\t3: Soft/Firmware\n-> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    idx = strtoul(keypress,NULL,10);
    if (idx < 4u)
    {
      mdp->type = to2digitBCD(idx);
    }
    else
    {
      fitPrint(USER, "unrecognized type %lu\n",idx);
    }
  }
}

static int32  getT_lines    (const char *cp,const eeprom_v1 *eehp)
{
  fitPrint(USER, "Display is configured for %d lines (text mode)\n",eehp->t_lines);
  return (int32)eehp->t_lines;
  //lint --e{715}  pointer cp must be passed in but is not referenced.
}

static void setT_lines    (const char *cp,eeprom_v1 *eehp)
{
  char         lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;

  fitPrint(USER, "Enter value for %s:\n-> ",cp);

  keypress = get_line(lbuf,sizeof(lbuf),stdin);

  eehp->t_lines = (u_int8)strtoul(keypress,NULL,10);
}

static int32  getT_cols     (const char *cp,const eeprom_v1 *eehp)
{
  fitPrint(USER, "Display is configured for %d columns (text mode)\n",eehp->t_cols);
  return (int32)eehp->t_cols;
  //lint --e{715}  pointer cp must be passed in but is not referenced.
}

static void setT_cols     (const char *cp,eeprom_v1 *eehp)
{
  char        lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;

  fitPrint(USER, "Enter value for %s:\n-> ",cp);

  keypress = get_line(lbuf,sizeof(lbuf),stdin);

  eehp->t_cols = (u_int8)strtoul(keypress,NULL,10);
}

static int32  getG_rows0    (const char *cp,const eeprom_v1 *eehp)
{
  fitPrint(USER, "Display is configured for %d rows (graphics mode)\n",eehp->g_rows0);
  return((int32)eehp->g_rows0);
  //lint --e{715}  pointer cp must be passed in but is not referenced.
}

static void setG_rows0    (const char *cp,eeprom_v1 *eehp)
{
  char        lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;

  fitPrint(USER, "Enter value for %s:\n-> ",cp);

  keypress = get_line(lbuf,sizeof(lbuf),stdin);

  eehp->g_rows0 = (u_int8)strtoul(keypress,NULL,10);
}

static int32  getG_cols0    (const char *cp,const eeprom_v1 *eehp)
{
  fitPrint(USER, "Display is configured for %d columns (graphics mode)\n",eehp->g_cols0);
  return((int32)eehp->g_cols0);
  //lint --e{715}  pointer cp must be passed in but is not referenced.
}

static void setG_cols0    (const char *cp,eeprom_v1 *eehp)
{
  char        lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;

  fitPrint(USER, "Enter value for %s:\n-> ",cp);

  keypress = get_line(lbuf,sizeof(lbuf),stdin);

  eehp->g_cols0 = (u_int8)strtoul(keypress,NULL,10);
}

static int32  getEthdevs    (const char *cp,const eeprom_v1 *eehp)
{
  fitPrint(USER, "There are %u Ethernet\n", eehp->ethdevs);
  fitPrint(USER, "Note: Number of Ethernet devices is fixed at 2\n");
  return((int32)eehp->ethdevs);
  //lint --e{715}  pointer cp must be passed in but is not referenced.
}

static void setEthdevs    (const char *cp,eeprom_v1 *eehp)
{
  char        lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;

  fitPrint(USER, "Enter number of %s (must be 2):\n-> ",cp);

  keypress = get_line(lbuf,sizeof(lbuf),stdin);
  eehp->ethdevs = (u_int8)strtoul(keypress,NULL,10);

  if (eehp->ethdevs != 2u)
  {
    fitPrint(USER, "Warning: Ignored input value of %u\n", eehp->ethdevs);
    eehp->ethdevs = 2;
  }
}

static int32  getEthdev     (const char *cp,const eeprom_v1 *eehp)
{
  char         lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char  *keypress;
  const eth_d *edp;
  u_int32      idx;
  bool         loopOn = true;

  fitPrint(USER, "%s\n",cp);

  while(loopOn)
  {
    fitPrint(USER, "Enter index for interface to view:\n\t0:\n\t1:\n" \
                   "\t*: any other number to quit\n-> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    idx = strtoul(keypress,NULL,10);

    if (idx >= 2u)
    {
      loopOn = false;
      continue;
    }

    edp = &eehp->ethdev[idx];
    fitPrint(USER, "Ethernet %lu type: ", idx);

    switch(edp->type){
      case 1:
        fitPrint(USER, "Other\n");
        break;
      case 2:
        fitPrint(USER, "Hub/Phy/Other Direct Port\n");
        break;
      case 3:
        fitPrint(USER, "Unmanaged Switch\n");
        break;
      case 4:
        fitPrint(USER, "Managed Switch\n");
        break;
      case 5:
        fitPrint(USER, "Router\n");
        break;
      default:
        fitPrint(USER, "unrecognized %d\n",edp->type);
        break;
    }

    fitPrint(USER, "ip_addr: %lu.%lu.%lu.%lu",
                    edp->ip_addr >> 24u,
                   (edp->ip_addr >> 16u) & 0xffu,
                   (edp->ip_addr >>  8u) & 0xffu,
                    edp->ip_addr         & 0xffu);
    fitPrint(USER, "mac_addr: %2.2x.%2.2x.%2.2x.%2.2x.%2.2x.%2.2x",
                    edp->mac_addr[5],edp->mac_addr[4], edp->mac_addr[3],
                    edp->mac_addr[2],edp->mac_addr[1], edp->mac_addr[0]);
    fitPrint(USER, "ip_mask: %lu.%lu.%lu.%lu",
                    edp->ip_mask >> 24u,
                   (edp->ip_mask >> 16u) & 0xffu,
                   (edp->ip_mask >>  8u) & 0xffu,
                    edp->ip_mask         & 0xffu);
    fitPrint(USER, "ip_gate: %lu.%lu.%lu.%lu",
                    edp->ip_gate >> 24u,
                   (edp->ip_gate >> 16u) & 0xffu,
                   (edp->ip_gate >>  8u) & 0xffu,
                    edp->ip_gate         & 0xffu);

    fitPrint(USER, "interface: ");
    switch(edp->interface){
      case 1:
        fitPrint(USER, "Other\n");
        break;
      case 2:
        fitPrint(USER, "Phy\n");
        break;
      case 3:
        fitPrint(USER, "RMII\n");
        break;
      case 4:
        fitPrint(USER, "MII\n");
        break;
      default:
        fitPrint(USER, "unrecognized %d\n",edp->interface);
        break;
    }
  }

  return 0;
}

static void setEthdev     (const char *cp,eeprom_v1 *eehp)
{
  char        lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;
  eth_d      *edp;
  u_int8      ucval;
  u_int32     ulval;
  u_int32     idx;
  bool        loopOn = true;

  fitPrint(USER, "%s: ",cp); // copy member name

  while(loopOn)
  {
    fitPrint(USER, "Enter index for interface to set:\n\t0:\n\t1:\n" \
             "\t*: any other number to quit\n-> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    idx = strtoul(keypress,NULL,10);

    if (idx >= 2u)
    {
      loopOn = false;
      continue;
    }

    edp = &eehp->ethdev[idx];

    fitPrint(USER, "Enter type:\n\t1: Other\n\t2: Hub/Phy/Other Direct Port\n" \
             "\t3: Unmanaged Switch\n\t4: Managed Switch\n\t5: Router\n-> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    ucval = (u_int8)strtoul(keypress,NULL,10);

    if (edp->type <= 5u)
    {
      edp->type = ucval;
    }
    else
    {
      fitPrint(USER, "unrecognized type %u\n",ucval);
    }

    edp->ip_addr = 0;

    for(idx=0;idx<4u;idx++){
      fitPrint(USER, "Enter IP address octet %lu:\n-> ",idx);
      keypress = get_line(lbuf,sizeof(lbuf),stdin);
      ulval = strtoul(keypress,NULL,10);
      switch(idx){
        case 0:
          edp->ip_addr = ulval << 24u;
          break;
        case 1:
          edp->ip_addr |= ulval << 16u;
          break;
        case 2:
          edp->ip_addr |= ulval << 8u;
          break;
        case 3:
          edp->ip_addr |= ulval;
          break;
        default:
          // impossible
          break;
      }
    }

    for(idx=0;idx<6u;idx++){
      fitPrint(USER, "Enter MAC octet %lu:\n-> ",idx);
      keypress = get_line(lbuf,sizeof(lbuf),stdin);
      ucval = (u_int8)strtoul(keypress,NULL,10);
      edp->mac_addr[5u-idx] = ucval;
    }

    edp->ip_mask = 0;

    for(idx=0;idx<4u;idx++){
      fitPrint(USER, "Enter IP netmask octet %lu:\n-> ",idx);
      keypress = get_line(lbuf,sizeof(lbuf),stdin);
      ulval = strtoul(keypress,NULL,10);

      switch(idx){
        case 0:
          edp->ip_mask = ulval << 24u;
          break;
        case 1:
          edp->ip_mask |= ulval << 16u;
          break;
        case 2:
          edp->ip_mask |= ulval << 8u;
          break;
        case 3:
          edp->ip_mask |= ulval;
          break;
        default:
          // impossible
          break;
      }
    }

    edp->ip_gate = 0;
    for(idx=0;idx<4u;idx++){
      fitPrint(USER, "Enter IP gateway octet %lu:\n-> ",idx);
      keypress = get_line(lbuf,sizeof(lbuf),stdin);
      ulval = strtoul(keypress,NULL,10);

      switch(idx){
        case 0:
          edp->ip_gate = ulval << 24u;
          break;
        case 1:
          edp->ip_gate |= ulval << 16u;
          break;
        case 2:
          edp->ip_gate |= ulval << 8u;
          break;
        case 3:
          edp->ip_gate |= ulval;
          break;
        default:
          // impossible
          break;
      }
    }

    fitPrint(USER, "Enter interface:\n\t1: Other\n\t2: Phy\n\t3: RMII\n\t4: MII\n-> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    ucval = (u_int8)strtoul(keypress,NULL,10);

    if (ucval <= 4u)
    {
      edp->interface = ucval;
    }
    else
    {
      fitPrint(USER, "unrecognized interface %d\n",ucval);
    }
  }
}

static int32  getSpi3use    (const char *cp,const eeprom_v1 *eehp)
{
  char lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};

  sprintf(lbuf,"%s: ",cp); // copy member name

  switch(eehp->spi3use){
    case 1:
      strcat(lbuf,"eth0 managed switch (Micrel KSZ8995MA)");
      break;
    case 2:
      strcat(lbuf,"eth1 managed switch (Micrel KSZ8995MA)");
      break;
    default:
      strcat(lbuf,"manufacturer specific code");
      break;
  }

  fitPrint(USER, "%s 0x%x\n",lbuf,eehp->spi3use);

  return((int32)eehp->spi3use);
}

static void setSpi3use    (const char *cp, eeprom_v1 *eehp)
{
  char        lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;

  fitPrint(USER, "Select code for %s:\n-> ",cp);

  keypress = get_line(lbuf,sizeof(lbuf),stdin);

  eehp->spi3use = (u_int8)strtoul(keypress,NULL,10);
}

static int32  getSpi4use    (const char *cp,const eeprom_v1 *eehp)
{
  char lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};

  sprintf(lbuf,"%s: ",cp); // copy member name

  switch(eehp->spi3use){
    case 1:
      strcat(lbuf,"eth0 managed switch (Micrel KSZ8995MA)");
      break;
    case 2:
      strcat(lbuf,"eth1 managed switch (Micrel KSZ8995MA)");
      break;
    default:
      strcat(lbuf,"manufacturer specific code");
      break;
  }

  fitPrint(USER, "%s 0x%x\n",lbuf,eehp->spi4use);

  return((int32)eehp->spi4use);
}

static void setSpi4use    (const char *cp,eeprom_v1 *eehp)
{
  char        lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;

  fitPrint(USER, "Select code for %s:\n-> ",cp);

  keypress = get_line(lbuf,sizeof(lbuf),stdin);

  eehp->spi4use = (u_int8)strtoul(keypress,NULL,10);
}

static u_int16 getPortmap (const char *cp,const portmap *pmp)
{
  u_int16 ports;
  u_int32 idx;

  fitPrint(USER, "%s: 0x%4.4x\n  ( ",cp,pmp->ports);
  idx = 0;

  for (ports = pmp->ports; ports > 0u; ports >>= 1u)
  {
    if ((ports & 0x0001u) == 1u)
    {
      switch (idx)
      {
        case 0:
          fitPrint(USER, "sp1 ");
          break;
        case 1:
          fitPrint(USER, "sp1s ");
          break;
        case 2:
          fitPrint(USER, "sp2 ");
          break;
        case 3:
          fitPrint(USER, "sp2s ");
          break;
        case 4:
          fitPrint(USER, "sp3 ");
          break;
        case 5:
          fitPrint(USER, "sp3s ");
          break;
        case 6:
          fitPrint(USER, "sp4 ");
          break;
        case 7:
          fitPrint(USER, "sp5s ");
          break;
        case 8:
          fitPrint(USER, "sp6 ");
          break;
        case 9:
          fitPrint(USER, "sp8 ");
          break;
        case 10:
          fitPrint(USER, "sp8s ");
          break;
        default: // 11 - 15
          fitPrint(USER, "unknown %lu ", idx);
          break;
      } // switch()
    }// if ()

    idx++;
  } // for()

  fitPrint(USER, ")\n");

  return(pmp->ports);
}

static void setPortmap (const char *cp,portmap *pmp)
{
  char        lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;
  u_int32     idx;
  bool        loopOn = true;

  while (loopOn)
  {
    fitPrint(USER, "%s: 0x%4.4x\n",cp,pmp->ports);
    fitPrint(USER, "Select serial ports to enable\n\t 1: sp1\n\t 2: sp1s\n" \
             "\t 3: sp2\n\t 4: sp2s\n\t 5: sp3\n\t 6: sp3s\n\t 7: sp4\n" \
             "\t 8: sp5s\n\t 9: sp6\n\t10: sp8\n\t11: sp8s\n" \
             "\t *: any other number to quit\n-> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    idx = strtoul(keypress,NULL,10);
    switch(idx){
      case 1:
        pmp->port.sp1 = 1;
        break;
      case 2:
        pmp->port.sp1s = 1;
        break;
      case 3:
        pmp->port.sp2 = 1;
        break;
      case 4:
        pmp->port.sp2s = 1;
        break;
      case 5:
        pmp->port.sp3 = 1;
        break;
      case 6:
        pmp->port.sp3s = 1;
        break;
      case 7:
        pmp->port.sp4 = 1;
        break;
      case 8:
        pmp->port.sp5s = 1;
        break;
      case 9:
        pmp->port.sp6 = 1;
        break;
      case 10:
        pmp->port.sp8 = 1;
        break;
      case 11:
        pmp->port.sp8s = 1;
        break;
      default:
        loopOn = false;
        break;
    } // switch()
  } // while()
}

static int32 getPorts_used (const char *cp,const eeprom_v1 *eehp)
{
  return((int32)getPortmap(cp,&eehp->ports_used));
}

static void setPorts_used (const char *cp,eeprom_v1 *eehp)
{
  setPortmap(cp,&eehp->ports_used);
}

static int32 getPorts    (const char *cp,const eeprom_v1 *eehp)
{
  fitPrint(USER, "Number of port descriptions is fixed at %d\n",eehp->ports);
  return((int32)eehp->ports);
  //lint --e{715}  pointer cp must be passed in but is not referenced.
}

static void setPorts    (const char *cp,eeprom_v1 *eehp)
{
  fitPrint(USER, "Number of port descriptions is fixed at 2\n");
  eehp->ports = 2u;
  //lint --e{715}  pointer cp must be passed in but is not referenced.
}

static int32 getPort       (const char *cp,const eeprom_v1 *eehp)
{
  char          lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char   *keypress;
  const port_d *pdp;
  u_int32       idx;
  bool          loopOn = true;

  fitPrint(USER, "%s: \n",cp); // copy member name

  while(loopOn)
  {
    fitPrint(USER, "Enter port index for %s:\n\t 0:\n\t 1:\n\t 2:\n\t 3:\n" \
             "\t 4:\n\t 5:\n\t 6:\n\t 7:\n\t 8:\n\t 9:\n\t10:\n" \
             "\t*: any other number to quit\n-> ",cp);
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    idx = strtoul(keypress,NULL,10);

    if (idx >= 11u)
    {
      loopOn = false;
      continue;
    }

    pdp = &eehp->port[idx];

    fitPrint(USER, "id: ");
    switch(pdp->id){
      case 0:
        fitPrint(USER, "none\n");
        break;
      case 1:
        fitPrint(USER, "sp1\n");
        break;
      case 2:
        fitPrint(USER, "sp2\n");
        break;
      case 3:
        fitPrint(USER, "sp3\n");
        break;
      case 4:
        fitPrint(USER, "sp4\n");
        break;
      case 5:
        fitPrint(USER, "sp5\n");
        break;
      case 6:
        fitPrint(USER, "sp6\n");
        break;
      case 7:
        fitPrint(USER, "resvd\n");
        break;
      case 8:
        fitPrint(USER, "sp8\n");
        break;
      case 9:
        fitPrint(USER, "eth0\n");
        break;
      case 10:
        fitPrint(USER, "eth1\n");
        break;
      case 11:
        fitPrint(USER, "spi3\n");
        break;
      case 12:
        fitPrint(USER, "spi4\n");
        break;
      case 13:
        fitPrint(USER, "usb\n");
        break;
      default:
        fitPrint(USER, "unrecognized id %d\n",pdp->id);
        break;
    }

    fitPrint(USER, "mode: ");
    switch(pdp->mode){
      case 0:
        fitPrint(USER, "other\n");
        break;
      case 1:
        fitPrint(USER, "async\n");
        break;
      case 2:
        fitPrint(USER, "sync\n");
        break;
      case 3:
        fitPrint(USER, "SDLC\n");
        break;
      case 4:
        fitPrint(USER, "HDLC\n");
        break;
      default:
        fitPrint(USER, "unrecognized mode %d\n",pdp->mode);
        break;
    }

    fitPrint(USER, "speed: %lu\n",pdp->speed);
  }

  return 0;
}

static void setPort       (const char *cp,eeprom_v1 *eehp)
{
  char        lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;
  port_d     *pdp;
  u_int8      ucval;
  u_int32     idx;
  bool        loopOn = true;

  fitPrint(USER, "%s: \n",cp); // copy member name

  while(loopOn)
  {
    fitPrint(USER, "Enter port index for %s:\n\t0-10:\n" \
             "\t*: any other number to quit\n-> ",cp);
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    idx = strtoul(keypress,NULL,10);

    if (idx >= 11u)
    {
      loopOn = false;
      continue;
    }

    pdp = &eehp->port[idx];

    fitPrint(USER, "Enter id:\n\t 0: none\n\t 1: sp1\n\t 2: sp2\n\t 3: sp3\n" \
             "\t 4: sp4\n\t 5: sp5\n\t 6: sp6\n\t 7: rsvd\n\t 8:sp8\n" \
             "\t 9: eth0\n\t10: eth1\n\t11: spi3\n\t12: spi4\n\t13: usb\n-> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    ucval = (u_int8)strtoul(keypress,NULL,10);

    if (ucval <= 13u)
    {
      pdp->id = ucval;
    }
    else
    {
      fitPrint(USER, "unrecognized id %u\n",ucval);
    }

    fitPrint(USER, "Enter mode:\n\t0: other\n\t1: async\n\t2: sync\n\t3: SDLC\n\t4: HDLC\n-> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    ucval = (u_int8)strtoul(keypress,NULL,10);

    if (ucval <= 4u)
    {
      pdp->mode = ucval;
    }
    else
    {
      fitPrint(USER, "unrecognized mode %d\n",ucval);
    }

    fitPrint(USER, "Enter speed:\n-> ");
    keypress = get_line(lbuf,sizeof(lbuf),stdin);
    idx = strtoul(keypress,NULL,10);

    switch (idx)
    {
      case 0: // Enet, USB, SPI
        fitPrint(USER, "Ethernet, USB or SPI speed: %lu\n",idx);
        pdp->speed = idx;
        break;
      case 1200:
      case 2400:
      case 4800:
      case 9600:
      case 19200:
      case 38400:
      case 57600:
      case 76800:
      case 115200:
        fitPrint(USER, "ATC standard async speed: %lu\n",idx);
        pdp->speed = idx;
        break;
      case 153600: // sync TS 2
      case 614400: // sync FIO, ITS
        fitPrint(USER, "ATC standard SDLC speed: %lu\n",idx);
        pdp->speed = idx;
        break;
      case 1312: // Boston UTCS
        fitPrint(USER, "Non-standard speed: %lu\n",idx);
        pdp->speed = idx;
        break;
      default:
        fitPrint(USER, "Invalid speed not set: %lu\n",idx);
        break;
    } // switch()
  } // while()
}

static int32  getPorts_pres (const char *cp,const eeprom_v1 *eehp)
{
  return((int32)getPortmap(cp,&eehp->ports_pres));
}

static void setPorts_pres (const char *cp, eeprom_v1 *eehp)
{
  setPortmap(cp,&eehp->ports_pres);
}

static int32  getSb1      (const char *cp,const eeprom_v1 *eehp)
{
  return((int32)getPortmap(cp,&eehp->sb1));
}

static void setSb1        (const char *cp,eeprom_v1 *eehp)
{
  setPortmap(cp,&eehp->sb1);
}

static int32  getSb2        (const char *cp,const eeprom_v1 *eehp)
{
  return((int32)getPortmap(cp,&eehp->sb2));
}

static void setSb2        (const char *cp,eeprom_v1 *eehp)
{
  setPortmap(cp,&eehp->sb2);
}

static int32  getTs2        (const char *cp,const eeprom_v1 *eehp)
{
  return((int32)getPortmap(cp,&eehp->ts2));
}

static void setTs2        (const char *cp,eeprom_v1 *eehp)
{
  setPortmap(cp,&eehp->ts2);
}

static int32  getExpbus     (const char *cp,const eeprom_v1 *eehp)
{
  char lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};

  sprintf(lbuf,"%s: ",cp); // copy member name

  switch(eehp->expbus){
    case 1:
      strcat(lbuf,"none");
      break;
    case 2:
      strcat(lbuf,"other");
      break;
    case 3:
      strcat(lbuf,"VME");
      break;
    default:
      strcat(lbuf,"unexpected");
      break;
  }

  strcat(lbuf," expansion bus");
  fitPrint(USER, "%s 0x%x\n",lbuf,eehp->expbus);

  return((int32)eehp->expbus);
}

static void setExpbus     (const char *cp, eeprom_v1 *eehp)
{
  char lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  char *keypress;

  fitPrint(USER, "Select number for %s:\n",cp);
  fitPrint(USER, "\t1: no expansion bus\n\t2: some other expansion bus\n" \
           "\t3: VME expansion bus\n-> ");
  keypress = get_line(lbuf,sizeof(lbuf),stdin);

  switch(*keypress){
    case '1':
    case '2':
    case '3':
      break;
    default:
      fitPrint(USER, "unexpected %s\n",keypress);
      break;
  }

  eehp->expbus = (u_int8)strtoul(keypress,NULL,10);
}

static int32  getCrc1       (const char *cp,const eeprom_v1 *eehp)
{
  fitPrint(USER, "%s: CRC1 is 0x%4.4x\n",cp,eehp->crc1);
  return((int32)eehp->crc1);
}

static void setCrc1       (const char *cp,eeprom_v1 *eehp)
{
  u_int16 fcs;

  fcs = genCrc(eehp,&eehp->crc1);
  fitPrint(USER, "%s: CRC1 set to 0x%x\n",cp,fcs);
  eehp->crc1 = fcs;
}

static int32  getLatitude   (const char *cp,const eeprom_v1 *eehp)
{
  fitPrint(USER, "%s: Latitude is %f\n",cp,eehp->latitude);
  return((int32)eehp->latitude);
}

static void setLatitude   (const char *cp,eeprom_v1 *eehp)
{
  char         lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;

  fitPrint(USER, "%s: Enter latitude:\n-> ",cp);
  keypress = get_line(lbuf,sizeof(lbuf),stdin);
  sscanf(keypress,"%f",&eehp->latitude);
}

static int32  getLongitude  (const char *cp,const eeprom_v1 *eehp)
{
  fitPrint(USER, "%s: Longitude is %f\n",cp, eehp->longitude);
  return((int32)eehp->longitude);
}

static void setLongitude  (const char *cp,eeprom_v1 *eehp)
{
  char         lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;

  fitPrint(USER, "%s: Enter latitude:\n-> ",cp);
  keypress = get_line(lbuf,sizeof(lbuf),stdin);
  sscanf(keypress,"%f",&eehp->longitude);
}

static int32  getContr_id    (const char *cp,const eeprom_v1 *eehp)
{
  fitPrint(USER, "%s: Controller ID is %d\n", cp, eehp->contr_id);
  return((int32)eehp->contr_id);
}

static void setContr_id    (const char *cp,eeprom_v1 *eehp)
{
  char        lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;

  fitPrint(USER, "Enter controller ID value for %s:\n-> ",cp);
  keypress = get_line(lbuf,sizeof(lbuf),stdin);
  eehp->contr_id = (u_int16)strtoul(keypress,NULL,10);
}

static int32  getCom_drop    (const char *cp,const eeprom_v1 *eehp)
{
  fitPrint(USER, "%s: Communication drop %d\n",cp,eehp->contr_id);
  return((int32)eehp->contr_id);
}

static void setCom_drop    (const char *cp,eeprom_v1 *eehp)
{
  char lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;

  fitPrint(USER, "Enter value for %s:\n-> ",cp);
  keypress = get_line(lbuf,sizeof(lbuf),stdin);
  eehp->contr_id = (u_int16)strtoul(keypress,NULL,10);
}

static int32  getRes_agency (const char *cp,const eeprom_v1 *eehp)
{
  fitPrint(USER, " %s: Agency Info is \'%s\'\n-> ",cp,eehp->res_agency);
  mdmp(eehp->res_agency,35,USER);
  return (-1);
}

static void setRes_agency (const char *cp,eeprom_v1 *eehp)
{
  char        lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;

  fitPrint(USER, " %s: Enter Agency Info (35 bytes max)\n-> ",cp);
  keypress = get_line(lbuf,sizeof(lbuf),stdin);
  memcpy(eehp->res_agency,keypress,sizeof(eehp->res_agency));
}

static int32 getCrc2 (const char *cp,const eeprom_v1 *eehp)
{
  fitPrint(USER, "%s: CRC2 is 0x%4.4x\n",cp,eehp->crc2);
  return((int32)eehp->crc2);
}

static void setCrc2  (const char *cp,eeprom_v1 *eehp)
{
  u_int16 fcs;

  fcs = genCrc(&eehp->latitude,&eehp->crc2);
  fitPrint(USER, "%s: CRC2 set to 0x%4.4x\n",cp,fcs);
  eehp->crc2 = fcs;
}

static int32  getUser_data  (const char *cp,const eeprom_v1 *eehp)
{
  size_t  ud_size;

  ud_size = eeSize - (u_int32)offsetof(eeprom_v1,user_data);
  fitPrint(USER, " %s: User Data is %u bytes\n-> ",cp,ud_size);
  mdmp(eehp->user_data,ud_size,USER);
  return (-1);
}

static void setUser_data  (const char *cp,eeprom_v1 *eehp)
{
  char        lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;
  u_int16     size;

  size = eehp->size;
  size = (size == 0u) ? 4u : (size-252u);
  size = (size >= MUST_BE_BIG_ENOUGH) ? (size - MUST_BE_BIG_ENOUGH) : size;

  fitPrint(USER, " %s: Enter user data (%u bytes max)\n-> ",cp,size);
  keypress = get_line(lbuf,sizeof(lbuf),stdin);
  memcpy(eehp->user_data,keypress,size); //lint !e669  user_data is just the
                                         //    first 4 bytes of the user data
}

static void eeEditHdr(eeprom_v1 *hp)
{
  char        lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *keypress;

  typedef struct _eeHdr {
    const char *eeField;
    int32 (*eeGfp)(const char *eeGname, const eeprom_v1 *eeprom); // get function
    void  (*eeSfp)(const char *eeSname, eeprom_v1 *eeprom);       // set function
  } eeHdr;

  const eeHdr *eep;
  eeHdr  eeEdit[] = { // tightly coupled to eeprom_v1...loosely
    {"version",getVersion,setVersion},
    {"size",getSize,setSize},
    {"modules",getModules,setModules},
    {"module[4]",getModule,setModule},
    {"t_lines",getT_lines,setT_lines},
    {"t_cols",getT_cols,setT_cols},
    {"g_rows0",getG_rows0,setG_rows0},
    {"g_cols0",getG_cols0,setG_cols0},
    {"ethdevs",getEthdevs,setEthdevs},
    {"ethdev[2]",getEthdev,setEthdev},
    {"spi3use",getSpi3use,setSpi3use},
    {"spi4use",getSpi4use,setSpi4use},
    {"ports_used",getPorts_used,setPorts_used},
    {"ports",getPorts,setPorts},
    {"port[11]",getPort,setPort},
    {"ports_pres",getPorts_pres,setPorts_pres},
    {"sb1",getSb1,setSb1},
    {"sb2",getSb2,setSb2},
    {"ts2",getTs2,setTs2},
    {"expbus",getExpbus,setExpbus},
    {"crc1",getCrc1,setCrc1},
    {"latitude",getLatitude,setLatitude},
    {"longitude",getLongitude,setLongitude},
    {"contr_id",getContr_id,setContr_id},
    {"com_drop",getCom_drop,setCom_drop},
    {"res_agency[35]",getRes_agency,setRes_agency},
    {"crc2",getCrc2,setCrc2},
    {"user_data[4]",getUser_data,setUser_data},
    {NULL,NULL,NULL}
  };

  fitPrint(USER, "\nWelcome to MFIT ATC EEPROM header editor!\n\n" \
                   "Use [y/n] to skip to the next member or expand member\n" \
                   "Press [q] to exit\n\n");

  for(eep = &eeEdit[0];eep->eeField != NULL;eep++){
    (void)eep->eeGfp(eep->eeField,hp); // get current field programming
    fitPrint(USER, "%s: [y/n/q]: ",eep->eeField);

    keypress = get_line(lbuf,sizeof(lbuf),stdin);

    if (*keypress == 'q')
    {
      break;
    }
    else if (*keypress == 'n')
    {
      continue;
    }
    else if (*keypress == 'y')
    {
      eep->eeSfp(eep->eeField,hp); // set field programming
    }
    else
    {
      fitPrint(USER, "eeEditHdr: unexpected input 0x%x\n",*keypress);
    }
  }

  mdmp(hp,256, USER);
}

static void eePrintHdr(eeprom_v1 *hp)
{
  mod_d        *mdp;
  const eth_d  *etp;
  const port_d *ptp;
  int32         i;

  fitPrint(USER, "ATC 5.2b Board Inventory\n");
  fitPrint(USER, "version: %u\n",hp->version);
  fitPrint(USER, "size: %u\n",hp->size);
  fitPrint(USER, "modules: %u\n",hp->modules);

  for(i=0;i<4;i++){
    mdp = &hp->module[i];
    fitPrint(USER, "\nmodule %ld location: %u\n",i,mdp->location);
    fitPrint(USER, "module %ld make: %u\n",i,mdp->make);
    fitPrint(USER, "module %ld model: %s\n",i,mdp->model);
    fitPrint(USER, "module %ld version:\n",i);
    mdmp(&mdp->version,7, USER);
    fitPrint(USER, "module %ld make: %d\n\n",i,mdp->type);
  }

  fitPrint(USER, "t_lines: %u\n",hp->t_lines);
  fitPrint(USER, "t_cols: %u\n",hp->t_cols);
  fitPrint(USER, "g_rows0: %u\n",hp->g_rows0);
  fitPrint(USER, "g_cols0: %u\n",hp->g_cols0);
  fitPrint(USER, "ethdevs: %u\n",hp->ethdevs);

  for(i=0;i<2;i++){
    etp = &hp->ethdev[i];
    fitPrint(USER, "\nethdev %ld type: %u\n",i,etp->type);
    fitPrint(USER, "ethdev %ld ip_addr:\n",i);
    mdmp(&etp->ip_addr,4, USER);
    fitPrint(USER, "ethdev %ld mac_addr:\n",i);
    mdmp(etp->mac_addr,6, USER);
    fitPrint(USER, "ethdev %ld ip_mask:\n",i);
    mdmp(&etp->ip_mask,4, USER);
    fitPrint(USER, "ethdev %ld ip_gate:\n",i);
    mdmp(&etp->ip_gate,4, USER);
    fitPrint(USER, "ethdev %ld interface: %d\n\n",i,etp->interface);
  }

  fitPrint(USER, "spi3use: %u\n",hp->spi3use);
  fitPrint(USER, "spi4use: %u\n",hp->spi4use);

  fitPrint(USER, "ports_used: 0x%x\n",hp->ports_used.ports);

  fitPrint(USER, "ports: %u\n",hp->ports);

  for(i=0;i<11;i++){
    ptp = &hp->port[i];
    fitPrint(USER, "\nport %ld id: %u\n",i,ptp->id);
    fitPrint(USER, "port %ld mode: %u\n",i,ptp->mode);
    fitPrint(USER, "port %ld speed: %lu\n\n",i,ptp->speed);
  }

  fitPrint(USER, "ports_pres: 0x%x\n",hp->ports_pres.ports);
  fitPrint(USER, "sb1: 0x%x\n",hp->sb1.ports);
  fitPrint(USER, "sb2: 0x%x\n",hp->sb2.ports);
  fitPrint(USER, "ts2: 0x%x\n",hp->ts2.ports);

  fitPrint(USER, "expbus: %d\n",hp->expbus);
  fitPrint(USER, "crc1: 0x%hx\n",hp->crc1);
  fitPrint(USER, "latitude: %f\n",hp->latitude);
  fitPrint(USER, "longitude: %f\n",hp->longitude);
  fitPrint(USER, "contr_id: %u\n",hp->contr_id);
  fitPrint(USER, "com_drop: %u\n",hp->com_drop);
  fitPrint(USER, "res_agency:\n");
  mdmp(hp->res_agency,35, USER);
  fitPrint(USER, "crc2: 0x%hx\n",hp->crc2);
  fitPrint(USER, "user_data:\n");
  mdmp(hp->user_data,4, USER);
  fitPrint(USER, "\n");
}

static int32 eeVerifyHdr(const eeprom_v1 *hp)
{
  u_int16 fcs;

  fcs = genCrc(hp,&hp->crc1);

  if(fcs != hp->crc1)
  {
    fitPrint(ERROR, "eeVerifyHdr: fails crc1, expected 0x%4.4x, actual 0x%4.4x\n",
             fcs,hp->crc1);
    return(-1);
  }

  fcs = genCrc(&hp->latitude,&hp->crc2);

  if(fcs != hp->crc2)
  {
    fitPrint(ERROR, "eeVerifyHdr: fails crc2, expected 0x%4.4x, actual 0x%4.4x\n",
             fcs,hp->crc2);
    return(-2);
  }

  fitPrint(USER, "eeVerifyHdr: CRCs are good\n");
  return(0);
}

static void eeLoadHdr(eeprom_v1 *hp,size_t sz)
{
  char    lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  char   *keypress;
  int32   fd;
  ssize_t bCnt;
  off_t   offset;

  fitPrint(USER, "Enter filename to load:\n-> ");
  keypress = get_line(lbuf,sizeof(lbuf),stdin);

  fd = open(keypress,O_RDWR);

  if(fd == -1){
    fitPrint(ERROR, "eeLoadHdr: cannot open %s, err %d: %s\n",
             keypress,errno,strerror(errno));
    return;
  }

  offset = lseek(fd,0,SEEK_SET);

  if (offset==-1)
  {
    fitPrint(ERROR, "eeLoadHdr: cannot seek within %s, err %d: %s\n",
             keypress,errno,strerror(errno));
    close(fd);
    return;
  }

  bCnt = read(fd,hp,sz);

  if((size_t)bCnt != sz){
    if(bCnt == -1){
      fitPrint(ERROR, "eeLoadHdr: cannot read from %s, err %d: %s\n",
               keypress,errno,strerror(errno));
    } else {
      fitPrint(ERROR, "eeLoadHdr: failed reading from %s, expected %u actual %d\n",
               keypress,sz,bCnt);
    }
    close(fd);
    return;
  }

  if(eeVerifyHdr(hp) < 0){
    /*
    * The file image was read into the 'saved' eeprom memory
    * and fails to verify.
    *
    * Abort badly here to keep from corrupting the eeprom image.
    */
    close(fd);
    exit(-1);
  }

  close(fd);
}

static void eeSaveHdr(const eeprom_v1 *hp,size_t sz)
{
  char    lbuf[MUST_BE_BIG_ENOUGH] = {'\0'};
  char   *keypress;
  int32   fd;
  ssize_t bCnt;
  off_t   offset;

  if(eeVerifyHdr(hp) < 0){
    /*
    * The eeprom memory image fails to verify,
    * abort the save operation.
    */
    fitPrint(ERROR, "eeSaveHdr: Cannot save invalid eeprom data\n");
    return;
  }

  fitPrint(USER, "Enter filename to save:\n-> ");
  keypress = get_line(lbuf,sizeof(lbuf),stdin);

  fd = open(keypress,O_CREAT|O_WRONLY);

  if(fd == -1){
    fitPrint(ERROR, "eeSaveHdr: cannot open %s, err %d: %s\n",
             keypress,errno,strerror(errno));
    return;
  }

  offset = lseek(fd,0,SEEK_SET);

  if (offset == -1)
  {
    fitPrint(ERROR, "eeSaveHdr: cannot seek within %s, err %d: %s\n",
             keypress,errno,strerror(errno));
    close(fd);
    return;
  }

  bCnt = write(fd,hp,sz);

  if((size_t)bCnt != sz){
    if(bCnt == -1){
      fitPrint(ERROR, "eeSaveHdr: cannot write from %s, err %d: %s\n",
               keypress,errno,strerror(errno));
    } else {
      fitPrint(ERROR, "eeSaveHdr: failed writing from %s, expected %u actual %d\n",
               keypress,sz,bCnt);
    }
    close(fd);
    return;
  }

  close(fd);
}

static void eeHdrOps(u_int16 hdrOps,void *mp,size_t sz)
{
  eeprom_v1  *hp;

  // check that the eeprom is larger than the header size
  if (sz >= sizeof(eeprom_v1))
  {
    hp = (void*)mp;

    if((hdrOps & EE_INIT) != 0u){
      memset(mp,0xff,sz);
    }
    if((hdrOps & EE_DEFAULT_HDR) != 0u){
      eeDefaultHdr(hp);
    }
    if((hdrOps & EE_LOAD_HDR) != 0u){
      eeLoadHdr(hp,sz);
    }
    if((hdrOps & EE_EDIT_HDR) != 0u){
      eeEditHdr(hp);
    }
    if((hdrOps & EE_SAVE_HDR) != 0u){
      eeSaveHdr(hp,sz);
    }
    if((hdrOps & EE_VERIFY_HDR) != 0u){
      (void)eeVerifyHdr(hp);
    }
    if((hdrOps & EE_PRINT_HDR) != 0u){
      eePrintHdr(hp);
    }
  }
}

/*
*
* EEPROM Usage
*
*/

static void eepromUsage(char * cp)
{
  const char *us =
"\
This provides a high level, non-destructive functional test of the ATC serial\n\
Host EEPROM.  The test is standalone and is not configurable. It saves the\n\
contents to RAM, runs a cycle of write/read/verify tests, then restors the\n\
original contents.\n\
\n\
This also provides an interactive way to access and modify the device memory.\n\
The storage format is compliant with ATC EEPROM version 1 (ATC v5.2b) format.\n\
This is done via the calling arguments listed below.\n\
below. As such, no options should be selected for automatic test.\n\
\n\
The arguments and functionality are defined as:\n\
\t-d generate a default dataset\n\
\t-e edit the dataset\n\
\t-i initialize the device to all ones\n\
\t-l load dataset from a file\n\
\t-s save dataset to a file\n\
\t-p print the header\n\
\t-v verify the dataset\n\
\t-h this help\n\
";
  fitPrint(USER, "Usage: %s [-deilpsvh]\n%s\n",cp,us);
  fitLicense();
}

/*
*
* EEPROM test entry
*
*/

ftRet_t eepromFit(plint argc, char * const argv[])
{
  int32          eeFd, bCnt, c, idx;
  bool           getOut=false;
  ftRet_t        retval=ftRxError;
  u_int32        i;
  FILE          *fp;
  u_int16        hdrOps = 0;
  off_t          eeOffset;
  const u_int8  *cp;
  void          *mp, *mp_orig;
  const char    *sd = "/dev/eeprom";
  const char    *optFlags = "-deilpsvhf:";

  /*
  *
  * Parse command line arguments
  *
  */

  opterr = 0;

  c = getopt(argc,argv,optFlags);

  while((c != -1)  && !getOut){
    switch((char)c){
      case 'd': // default ATC5.2b header
        hdrOps |= EE_DEFAULT_HDR;
        break;
      case 'e': // edit ATC5.2b header
        hdrOps |= EE_EDIT_HDR;
        break;
      case 'i': // init eeprom device
        hdrOps |= EE_INIT;
        break;
      case 'l': // load ATC5.2b header
        hdrOps |= EE_LOAD_HDR;
        break;
      case 'p': // print ATC5.2b header
        hdrOps |= EE_PRINT_HDR;
        break;
      case 's': // save ATC5.2b header
        hdrOps |= EE_SAVE_HDR;
        break;
      case 'v': // verify ATC5.2b header
        hdrOps |= EE_VERIFY_HDR;
        break;
      case 'f': // configuration file name
        fp = fopen(optarg,"r");

        if(fp == NULL)
        {
          fitPrint(ERROR, "cannot open %s: error %d, %s.\n",
                   optarg,errno,strerror(errno));
          retval = ftUpdateTestStatus(ftrp,ftComplete,NULL);
          getOut = true;
        }
        else
        {
          fclose(fp);
        }
        break;
      case 'h':
        eepromUsage(argv[0]);
        retval = ftUpdateTestStatus(ftrp,ftComplete,NULL);
        getOut = true;
        break;
      default:
        if(isprint(optopt)){
          fitPrint(ERROR, "unknown option `-%c`.\n",optopt);
        } else {
          fitPrint(ERROR, "unknown option character `\\X%X`.\n",optopt);
        }
        eepromUsage(argv[0]);
        retval = ftUpdateTestStatus(ftrp,ftComplete,NULL);
        getOut = true;
        break;
    }

    c = getopt(argc,argv,optFlags);
  }

  for(idx = optind; idx < argc; idx++){
    fitPrint(ERROR, "Non-option argument %s\n",argv[idx]);
    eepromUsage(argv[0]);
    retval = ftUpdateTestStatus(ftrp,ftComplete,NULL);
    getOut = true;
    break;
  }

  if (getOut)
  {
    return retval;
  }


  /*
  *
  * Open EEPROM
  *
  */

  eeFd = open(sd,O_RDWR);

  if(eeFd == -1){
    fitPrint(ERROR, "%s test cannot open %s, err %d: %s\n",
             argv[0],sd,errno,strerror(errno));
    ftUpdateTestStatus(ftrp,ftError,NULL);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  eeOffset = lseek(eeFd,0,SEEK_END);// get the size of device

  if(eeOffset <= 0){
    fitPrint(ERROR, "%s test cannot get size of %s, err %d: %s\n",
             argv[0],sd,errno,strerror(errno));
    ftUpdateTestStatus(ftrp,ftError,NULL);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  eeSize = (size_t)eeOffset;

  /*
  *
  * Malloc receive buffers
  *
  */

  /* buffer used to store the unmodified eeprom data contents */
  mp_orig = malloc(eeSize);

  if(mp_orig == NULL){
    fitPrint(ERROR, "%s test cannot malloc %u bytes, err %d: %s\n",
             argv[0],eeSize,errno,strerror(errno));
    close(eeFd);
    ftUpdateTestStatus(ftrp,ftError,NULL);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  memset(mp_orig,0,eeSize);// clear out memory

  /* buffer used for destructive testing of eeprom data */
  mp = malloc(eeSize);

  if(mp == NULL){
    fitPrint(ERROR, "%s test cannot malloc %u bytes, err %d: %s\n",
             argv[0],eeSize,errno,strerror(errno));
    free(mp_orig);
    close(eeFd);
    ftUpdateTestStatus(ftrp,ftError,NULL);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  memset(mp,0,eeSize);// clear out memory

  /*
  *
  * Save the device content
  *
  */

  fitPrint(VERBOSE, "%s saving %s, size %u\n",argv[0],sd,eeSize);

  eeOffset = lseek(eeFd,0,SEEK_SET);

  if (eeOffset == -1)
  {
    fitPrint(ERROR, "%s cannot seek within %s, err %d: %s\n",
             argv[0],sd,errno,strerror(errno));
    ftUpdateTestStatus(ftrp,ftError,NULL);
    free(mp_orig);
    free(mp);
    close(eeFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }
  // It is presumed that lseek() to the beginning will always work now.

  bCnt = read(eeFd,mp_orig,eeSize);

  if((size_t)bCnt != eeSize){
    if(bCnt == -1){
      fitPrint(ERROR, "%s cannot read from %s, err %d: %s\n",
               argv[0],sd,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      fitPrint(ERROR, "%s failed reading from %s, expected %u actual %ld\n",
               argv[0],sd,eeSize,bCnt);
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    free(mp_orig);
    free(mp);
    close(eeFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  fitPrint(VERBOSE, "%s saving %s, size %u complete\n",argv[0],sd,eeSize);

  mdmp(mp_orig,eeSize, VERBOSE);

  (void)lseek(eeFd,0,SEEK_SET);

  /*
  *
  * ATC 5.2b header operations
  *
  */
  eeHdrOps(hdrOps,mp_orig,eeSize);

  /*
  *
  * Write the eeprom device
  *
  */

  fitPrint(VERBOSE, "%s writing %s, size %u\n",argv[0],sd,eeSize);

  (void)lseek(eeFd,0,SEEK_SET);
  bCnt = write(eeFd,mp,eeSize);

  if((size_t)bCnt != eeSize){
    if(bCnt == -1){
      fitPrint(ERROR, "%s cannot write to %s, err %d: %s\n",
               argv[0],sd,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      ftUpdateTestStatus(ftrp,ftFail,NULL);
      fitPrint(ERROR, "%s error writing to %s, expected %u actual %ld\n",
               argv[0],sd,eeSize,bCnt);
    }
    free(mp_orig);
    free(mp);
    close(eeFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  fitPrint(VERBOSE, "%s writing %s, size %u complete\n",argv[0],sd,eeSize);

  /*
  *
  * Read the eeprom device
  *
  */

  fitPrint(VERBOSE, "%s reading %s, size %u\n",argv[0],sd,eeSize);

  (void)lseek(eeFd,0,SEEK_SET);
  bCnt = read(eeFd,mp,eeSize);

  if((size_t)bCnt != eeSize){
    if(bCnt == -1){
      fitPrint(ERROR, "%s cannot read from %s, err %d: %s\n",
               argv[0],sd,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      fitPrint(ERROR, "%s failed reading from %s, expected %u actual %ld\n",
               argv[0],sd,eeSize,bCnt);
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    free(mp_orig);
    free(mp);
    close(eeFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  fitPrint(VERBOSE, "%s reading %s, size %u complete\n",argv[0],sd,eeSize);

  fitPrint(VERBOSE, "%s verifying device is cleared\n",argv[0]);

  mdmp(mp,eeSize, VERBOSE);

  cp=(u_int8*)mp;

  for(i=0;i<eeSize;i++)
  {
    if (*cp != 0u)
    {
      break;
    }
    cp++;
  }

  if(i!=eeSize){
    fitPrint(ERROR, "%s device is not cleared, addr %lu, val 0x%2.2x\n",argv[0],i,*cp);
    ftUpdateTestStatus(ftrp,ftFail,NULL);
  } else {
    ftUpdateTestStatus(ftrp,ftPass,NULL);
  }

  fitPrint(VERBOSE, "%s verifying device is cleared complete\n",argv[0]);

  /*
  *
  * Restore the eeprom device
  *
  */

  fitPrint(VERBOSE, "%s restoring %s, size %u\n",argv[0],sd,eeSize);

  (void)lseek(eeFd,0,SEEK_SET);
  bCnt = write(eeFd,mp_orig,eeSize);

  if((size_t)bCnt != eeSize){
    if(bCnt == -1){
      fitPrint(ERROR, "%s cannot write to %s, err %d: %s\n",
               argv[0],sd,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      fitPrint(ERROR, "%s failed writing to %s, expected %u actual %ld\n",
               argv[0],sd,eeSize,bCnt);
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    free(mp_orig);
    free(mp);
    close(eeFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  fitPrint(VERBOSE, "%s restoring %s, size %u complete\n",argv[0],sd,eeSize);

  /*
  *
  * Read the eeprom device
  *
  */

  fitPrint(VERBOSE, "%s reading %s, size %u\n",argv[0],sd,eeSize);

  (void)lseek(eeFd,0,SEEK_SET);
  bCnt = read(eeFd,mp,eeSize);

  if((size_t)bCnt != eeSize){
    if(bCnt == -1){
      fitPrint(ERROR, "%s cannot read from %s, err %d: %s\n",
               argv[0],sd,errno,strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    } else {
      fitPrint(ERROR, "%s failed reading from %s, expected %u actual %ld\n",
               argv[0],sd,eeSize,bCnt);
      ftUpdateTestStatus(ftrp,ftFail,NULL);
    }
    free(mp_orig);
    free(mp);
    close(eeFd);
    return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  fitPrint(VERBOSE, "%s reading %s, size %u complete\n",argv[0],sd,eeSize);

  /*
  *
  * Verify that eeprom device has successfully restored
  *
  */

  fitPrint(VERBOSE, "%s verifying %s, size %u\n",argv[0],sd,eeSize);

  mdmp(mp,eeSize, VERBOSE);

  c =  memcmp(mp,mp_orig,(size_t)eeSize);

  if(c != 0){
    ftUpdateTestStatus(ftrp,ftFail,NULL);
  } else {
    ftUpdateTestStatus(ftrp,ftPass,NULL);
  }

  fitPrint(VERBOSE, "%s verifying %s, size %u complete\n",argv[0],sd,eeSize);

  free(mp_orig);
  free(mp);
  close(eeFd);
  return(ftUpdateTestStatus(ftrp,ftComplete,NULL));
 }
