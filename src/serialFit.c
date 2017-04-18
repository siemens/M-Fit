/******************************************************************************
                                  serialFit.c

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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h> /* POSIX Threads */
#include <semaphore.h>
#include <string.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "fit.h"
#include "serialFit.h"

//#define DUMP_TERMIOS

// somehow the compiler can't fine sem_timedwait() when <semaphore.h> is included
int32 sem_timedwait(sem_t *semstruct,const struct timespec *timeout);
// but the linker finds it just fine


static ftResults_t ftResults = {0};
static ftResults_t *ftrp = &ftResults;

static void printSerialStats(void);


// ====================== Start of Inactive Code =======================
#if 0  //master disable
/*
* Data to track GPIO programming
*/
volatile u_int32 prev_pdata = 0;
volatile u_int32 prev_pdira = 0;
volatile u_int32 prev_pdirb = 0;
volatile u_int32 prev_pdirc = 0;
volatile u_int32 prev_pdird = 0;
volatile u_int32 prev_ppara = 0;
volatile u_int32 prev_pparb = 0;
volatile u_int32 prev_pparc = 0;
volatile u_int32 prev_ppard = 0;
volatile u_int32 prev_psora = 0;
volatile u_int32 prev_psorb = 0;
volatile u_int32 prev_psorc = 0;
volatile u_int32 prev_psord = 0;

typedef volatile u_int32 vu32;
typedef u_int32 u_int32;

struct cpm2_ioports {
  u_int32 dir, par, sor, odr, dat;
  u_int32 res[3];
};

#define P00 (1 << (31- 0))
#define P01 (1 << (31- 1))
#define P02 (1 << (31- 2))
#define P03 (1 << (31- 3))
#define P04 (1 << (31- 4))
#define P05 (1 << (31- 5))
#define P06 (1 << (31- 6))
#define P07 (1 << (31- 7))
#define P08 (1 << (31- 8))
#define P09 (1 << (31- 9))
#define P10 (1 << (31-10))
#define P11 (1 << (31-11))
#define P12 (1 << (31-12))
#define P13 (1 << (31-13))
#define P14 (1 << (31-14))
#define P15 (1 << (31-15))
#define P16 (1 << (31-16))
#define P17 (1 << (31-17))
#define P18 (1 << (31-18))
#define P19 (1 << (31-19))
#define P20 (1 << (31-20))
#define P21 (1 << (31-21))
#define P22 (1 << (31-22))
#define P23 (1 << (31-23))
#define P24 (1 << (31-24))
#define P25 (1 << (31-25))
#define P26 (1 << (31-26))
#define P27 (1 << (31-27))
#define P28 (1 << (31-28))
#define P29 (1 << (31-29))
#define P30 (1 << (31-30))
#define P31 (1 << (31-31))

const u_int32 pin_val[] = {
  P00, P01, P02, P03, P04, P05, P06, P07, P08,
  P09, P10, P11, P12, P13, P14, P15, P16, P17,
  P18, P19, P20, P21, P22, P23, P24, P25, P26,
  P27, P28, P29, P30, P31
};

const char *pin_str[] = {
  "P00", "P01", "P02", "P03", "P04", "P05", "P06", "P07", "P08",
  "P09", "P10", "P11", "P12", "P13", "P14", "P15", "P16", "P17",
  "P18", "P19", "P20", "P21", "P22", "P23", "P24", "P25", "P26",
  "P27", "P28", "P29", "P30", "P31"
};

#define PORTA   0
#define PORTB   1
#define PORTC   2
#define PORTD   3
#define MUXSCR  4
#define MUXFCR  5

const char *port_str[] = {
  "PORTA ", "PORTB ", "PORTC ", "PORTD ", "MUXSCR", "MUXFCR", "INVALID"
};

#define PDIR 0
#define PPAR 1
#define PSOR 2
#define PODR 3
#define CMXS 4
#define CMXF 5

const char *reg_str[] = {
  //"PDIR", "PPAR", "PSOR", "PODR","CMXS", "CMXF"
  "Port Direction Register     ", "Port Pin Assignment Register",
  "Port Special Option Register", "Port Open Drain Register","CMXS", "CMXF"
};

typedef struct {
  u_int32 gr1   :1; // grant support of SCC1
  u_int32 sc1   :1; // SCC1 connection
  u_int32 rs1cs :3; // RX SCC1 clock source in NMSI mode, ignored if sc1 connected to TSA
  u_int32 ts1cs :3; // TX SCC1 clock source in NMSI mode, ignored if sc1 connected to TSA
  u_int32 gr2   :1; // grant support of SCC2
  u_int32 sc2   :1; // SCC2 connection
  u_int32 rs2cs :3; // RX SCC2 clock source in NMSI mode, ignored if sc2 connected to TSA
  u_int32 ts2cs :3; // TX SCC2 clock source in NMSI mode, ignored if sc2 connected to TSA
  u_int32 gr3   :1; // grant support of SCC3
  u_int32 sc3   :1; // SCC3 connection
  u_int32 rs3cs :3; // RX SCC3 clock source in NMSI mode, ignored if sc3 connected to TSA
  u_int32 ts3cs :3; // TX SCC3 clock source in NMSI mode, ignored if sc3 connected to TSA
  u_int32 gr4   :1; // grant support of SCC4
  u_int32 sc4   :1; // SCC4 connection
  u_int32 rs4cs :3; // RX SCC4 clock source in NMSI mode, ignored if sc4 connected to TSA
  u_int32 ts4cs :3; // TX SCC4 clock source in NMSI mode, ignored if sc4 connected to TSA
} CMXSCR_t;

typedef union {
  CMXSCR_t bitField;
  volatile u_int32 regValue;
} CMXSCR_u;

typedef struct {
  u_int32 rsvd1   :1;
  u_int32 fc1   :1; // FCC1 connection
  u_int32 rf1cs :3; // RX FCC1 clock source in NMSI mode, ignored if sc1 connected to TSA
  u_int32 tf1cs :3; // TX FCC1 clock source in NMSI mode, ignored if sc1 connected to TSA
  u_int32 fc2   :2; // FCC2 connection
  u_int32 rf2cs :3; // RX FCC2 clock source in NMSI mode, ignored if sc2 connected to TSA
  u_int32 tf2cs :3; // TX FCC2 clock source in NMSI mode, ignored if sc2 connected to TSA
  u_int32 rsvd2   :1;
  u_int32 fc3   :1; // FCC3 connection
  u_int32 rf3cs :3; // RX FCC3 clock source in NMSI mode, ignored if sc3 connected to TSA
  u_int32 tf3cs :3; // TX FCC3 clock source in NMSI mode, ignored if sc3 connected to TSA
  u_int32 rsvd3   :8;
} CMXFCR_t;

typedef union {
  CMXFCR_t bitField;
  volatile u_int32 regValue;
} CMXFCR_u;



void pscan(int32 port, int32 regn, u_int32 val)
{
  int32 i;
  u_int32 pv;

  // currently not looking for zeros
  if(!val){
    return;
  }

  switch(port){
  case PORTA:
  case PORTC:
    i = 0;
    break;
  case PORTB:
  case PORTD:
    i = 4;
    break;
  default:
    fitPrint(VERBOSE, "port %d is invalid\n",port);
    return;
  }

  for(pv = pin_val[i]; i < 32;pv = pin_val[++i]){
    //fitPrint(VERBOSE, "pscan: %s i = %d, pv = %8.8x, val = %8.8x, and = %8.8x\n",port_str[port],i,pv,val,pv&val);//HACK
    //continue;//HACK

    if(pv & val){
      switch(regn){
      case 0:
        fitPrint(VERBOSE, "%s %s %8.8x %s is an output\n",port_str[port],reg_str[regn],pv,pin_str[i]);
        break;
      case 1:
        fitPrint(VERBOSE, "%s %s %8.8x %s dedicated peripheral pin\n",port_str[port],reg_str[regn],pv,pin_str[i]);
        break;
      case 2:
        fitPrint(VERBOSE, "%s %s %8.8x %s dedicated peripheral option 2\n",port_str[port],reg_str[regn],pv,pin_str[i]);
        break;
      case 3:
      case 4:
        fitPrint(VERBOSE, "%s %s %8.8x\n",port_str[port],reg_str[regn],pv);
        break;
      default:
        fitPrint(VERBOSE, "register number %d is invalid for %s\n",regn,port_str[port]);
        return;
      }
    }
  }
}

//
// scan clock route registers
//
void clk_route_reg(int32 idx,u_int32 val)
{
  CMXSCR_u cmxscr;
  CMXFCR_u cmxfcr;

  cmxscr.regValue = val;
  cmxfcr.regValue = val;

  switch(idx){
  case 0:
    //
    // SCC1
    //
    if(cmxscr.bitField.gr1 & 1){
      fitPrint(VERBOSE, "SCC1 transmitter supports the grant mechanism as determined by the GMx bit\nof a serial device channel\n");
    } else {
      fitPrint(VERBOSE, "SCC1 transmitter does not support the grant mechanism.\nThe grant is always asserted internally.\n");
    }

    if(cmxscr.bitField.sc1 & 1){
      fitPrint(VERBOSE, "SCC1 is connected to the Time Slot Assigner.\nThe Non Multiplexed Serial Interface pins are available.\n\n");
    } else {
      fitPrint(VERBOSE, "SCC1 is not connected to the Time Slot Assigner and is either connected\ndirectly to the Non Multiplexed Serial Interface pins or not used.\n\n");

      switch(cmxscr.bitField.rs1cs){
      case 0:
        fitPrint(VERBOSE, "\tSCC1 RX clock is BRG01\n");
        break;
      case 1:
        fitPrint(VERBOSE, "\tSCC1 RX clock is BRG02\n");
        break;
      case 2:
        fitPrint(VERBOSE, "\tSCC1 RX clock is BRG03\n");
        break;
      case 3:
        fitPrint(VERBOSE, "\tSCC1 RX clock is BRG04\n");
        break;
      case 4:
        fitPrint(VERBOSE, "\tSCC1 RX clock is CLK11\n");
        break;
      case 5:
        fitPrint(VERBOSE, "\tSCC1 RX clock is CLK12\n");
        break;
      case 6:
        fitPrint(VERBOSE, "\tSCC1 RX clock is CLK3\n");
        break;
      case 7:
        fitPrint(VERBOSE, "\tSCC1 RX clock is CLK4\n");
        break;
      }

      switch(cmxscr.bitField.ts1cs){
      case 0:
        fitPrint(VERBOSE, "\tSCC1 TX clock is BRG01\n");
        break;
      case 1:
        fitPrint(VERBOSE, "\tSCC1 TX clock is BRG02\n");
        break;
      case 2:
        fitPrint(VERBOSE, "\tSCC1 TX clock is BRG03\n");
        break;
      case 3:
        fitPrint(VERBOSE, "\tSCC1 TX clock is BRG04\n");
        break;
      case 4:
        fitPrint(VERBOSE, "\tSCC1 TX clock is CLK11\n");
        break;
      case 5:
        fitPrint(VERBOSE, "\tSCC1 TX clock is CLK12\n");
        break;
      case 6:
        fitPrint(VERBOSE, "\tSCC1 TX clock is CLK3\n");
        break;
      case 7:
        fitPrint(VERBOSE, "\tSCC1 TX clock is CLK4\n");
        break;
      }
    }
    break;

  case 1:
    //
    // SCC2
    //
    if(cmxscr.bitField.gr2 & 1){
      fitPrint(VERBOSE, "SCC2 transmitter supports the grant mechanism as determined by the GMx bit\nof a serial device channel\n");
    } else {
      fitPrint(VERBOSE, "SCC2 transmitter does not support the grant mechanism.\nThe grant is always asserted internally.\n");
    }

    if(cmxscr.bitField.sc2 & 1){
      fitPrint(VERBOSE, "SCC2 is connected to the Time Slot Assigner.\nThe Non Multiplexed Serial Interface pins are available.\n\n");
    } else {
      fitPrint(VERBOSE, "SCC2 is not connected to the Time Slot Assigner and is either connected\ndirectly to the Non Multiplexed Serial Interface pins or not used.\n\n");

      switch(cmxscr.bitField.rs2cs){
      case 0:
        fitPrint(VERBOSE, "\tSCC2 RX clock is BRG01\n");
        break;
      case 1:
        fitPrint(VERBOSE, "\tSCC2 RX clock is BRG02\n");
        break;
      case 2:
        fitPrint(VERBOSE, "\tSCC2 RX clock is BRG03\n");
        break;
      case 3:
        fitPrint(VERBOSE, "\tSCC2 RX clock is BRG04\n");
        break;
      case 4:
        fitPrint(VERBOSE, "\tSCC2 RX clock is CLK11\n");
        break;
      case 5:
        fitPrint(VERBOSE, "\tSCC2 RX clock is CLK12\n");
        break;
      case 6:
        fitPrint(VERBOSE, "\tSCC2 RX clock is CLK3\n");
        break;
      case 7:
        fitPrint(VERBOSE, "\tSCC2 RX clock is CLK4\n");
        break;
      }

      switch(cmxscr.bitField.ts2cs){
      case 0:
        fitPrint(VERBOSE, "\tSCC2 TX clock is BRG01\n");
        break;
      case 1:
        fitPrint(VERBOSE, "\tSCC2 TX clock is BRG02\n");
        break;
      case 2:
        fitPrint(VERBOSE, "\tSCC2 TX clock is BRG03\n");
        break;
      case 3:
        fitPrint(VERBOSE, "\tSCC2 TX clock is BRG04\n");
        break;
      case 4:
        fitPrint(VERBOSE, "\tSCC2 TX clock is CLK11\n");
        break;
      case 5:
        fitPrint(VERBOSE, "\tSCC2 TX clock is CLK12\n");
        break;
      case 6:
        fitPrint(VERBOSE, "\tSCC2 TX clock is CLK3\n");
        break;
      case 7:
        fitPrint(VERBOSE, "\tSCC2 TX clock is CLK4\n");
        break;
      }
    }
    break;

  case 2:
    //
    // SCC3
    //
    if(cmxscr.bitField.gr3 & 1){
      fitPrint(VERBOSE, "SCC3 transmitter supports the grant mechanism as determined by the GMx bit\nof a serial device channel\n");
    } else {
      fitPrint(VERBOSE, "SCC3 transmitter does not support the grant mechanism.\nThe grant is always asserted internally.\n");
    }

    if(cmxscr.bitField.sc3 & 1){
      fitPrint(VERBOSE, "SCC3 is connected to the Time Slot Assigner.\nThe Non Multiplexed Serial Interface pins are available.\n\n");
    } else {
      fitPrint(VERBOSE, "SCC3 is not connected to the Time Slot Assigner and is either connected\ndirectly to the Non Multiplexed Serial Interface pins or not used.\n\n");

      switch(cmxscr.bitField.rs3cs){
      case 0:
        fitPrint(VERBOSE, "\tSCC3 RX clock is BRG01\n");
        break;
      case 1:
        fitPrint(VERBOSE, "\tSCC3 RX clock is BRG02\n");
        break;
      case 2:
        fitPrint(VERBOSE, "\tSCC3 RX clock is BRG03\n");
        break;
      case 3:
        fitPrint(VERBOSE, "\tSCC3 RX clock is BRG04\n");
        break;
      case 4:
        fitPrint(VERBOSE, "\tSCC3 RX clock is CLK05\n");
        break;
      case 5:
        fitPrint(VERBOSE, "\tSCC3 RX clock is CLK06\n");
        break;
      case 6:
        fitPrint(VERBOSE, "\tSCC3 RX clock is CLK07\n");
        break;
      case 7:
        fitPrint(VERBOSE, "\tSCC3 RX clock is CLK08\n");
        break;
      }

      switch(cmxscr.bitField.ts3cs){
      case 0:
        fitPrint(VERBOSE, "\tSCC3 TX clock is BRG01\n");
        break;
      case 1:
        fitPrint(VERBOSE, "\tSCC3 TX clock is BRG02\n");
        break;
      case 2:
        fitPrint(VERBOSE, "\tSCC3 TX clock is BRG03\n");
        break;
      case 3:
        fitPrint(VERBOSE, "\tSCC3 TX clock is BRG04\n");
        break;
      case 4:
        fitPrint(VERBOSE, "\tSCC3 TX clock is CLK05\n");
        break;
      case 5:
        fitPrint(VERBOSE, "\tSCC3 TX clock is CLK06\n");
        break;
      case 6:
        fitPrint(VERBOSE, "\tSCC3 TX clock is CLK07\n");
        break;
      case 7:
        fitPrint(VERBOSE, "\tSCC3 TX clock is CLK08\n");
        break;
      }
    }
    break;

  case 3:
    //
    // SCC4
    //
    if(cmxscr.bitField.gr4 & 1){
      fitPrint(VERBOSE, "SCC4 transmitter supports the grant mechanism as determined by the GMx bit\nof a serial device channel\n");
    } else {
      fitPrint(VERBOSE, "SCC4 transmitter does not support the grant mechanism.\nThe grant is always asserted internally.\n");
    }

    if(cmxscr.bitField.sc4 & 1){
      fitPrint(VERBOSE, "SCC4 is connected to the Time Slot Assigner.\nThe Non Multiplexed Serial Interface pins are available.\n\n");
    } else {
      fitPrint(VERBOSE, "SCC4 is not connected to the Time Slot Assigner and is either connected\ndirectly to the Non Multiplexed Serial Interface pins or not used.\n\n");

      switch(cmxscr.bitField.rs4cs){
      case 0:
        fitPrint(VERBOSE, "\tSCC4 RX clock is BRG01\n");
        break;
      case 1:
        fitPrint(VERBOSE, "\tSCC4 RX clock is BRG02\n");
        break;
      case 2:
        fitPrint(VERBOSE, "\tSCC4 RX clock is BRG03\n");
        break;
      case 3:
        fitPrint(VERBOSE, "\tSCC4 RX clock is BRG04\n");
        break;
      case 4:
        fitPrint(VERBOSE, "\tSCC4 RX clock is CLK05\n");
        break;
      case 5:
        fitPrint(VERBOSE, "\tSCC4 RX clock is CLK06\n");
        break;
      case 6:
        fitPrint(VERBOSE, "\tSCC4 RX clock is CLK07\n");
        break;
      case 7:
        fitPrint(VERBOSE, "\tSCC4 RX clock is CLK08\n");
        break;
      }

      switch(cmxscr.bitField.ts4cs){
      case 0:
        fitPrint(VERBOSE, "\tSCC4 TX clock is BRG01\n");
        break;
      case 1:
        fitPrint(VERBOSE, "\tSCC4 TX clock is BRG02\n");
        break;
      case 2:
        fitPrint(VERBOSE, "\tSCC4 TX clock is BRG03\n");
        break;
      case 3:
        fitPrint(VERBOSE, "\tSCC4 TX clock is BRG04\n");
        break;
      case 4:
        fitPrint(VERBOSE, "\tSCC4 TX clock is CLK05\n");
        break;
      case 5:
        fitPrint(VERBOSE, "\tSCC4 TX clock is CLK06\n");
        break;
      case 6:
        fitPrint(VERBOSE, "\tSCC4 TX clock is CLK07\n");
        break;
      case 7:
        fitPrint(VERBOSE, "\tSCC4 TX clock is CLK08\n");
        break;
      }
    }
    break;
  case 4:
    //
    // FCC3
    //
#   if 0
    if(cmxfcr.bitField.fc1 & 1){
      fitPrint(VERBOSE, "FCC1 is connected to the TSA\n");
    } else {
      fitPrint(VERBOSE, "FCC1 is connected to the NMSI\n");
      switch(cmxfcr.bitField.rf1cs){
      case 0:
        fitPrint(VERBOSE, "\tFCC1 RX clock is BRG05\n");
        break;
      case 1:
        fitPrint(VERBOSE, "\tFCC1 RX clock is BRG06\n");
        break;
      case 2:
        fitPrint(VERBOSE, "\tFCC1 RX clock is BRG07\n");
        break;
      case 3:
        fitPrint(VERBOSE, "\tFCC1 RX clock is BRG08\n");
        break;
      case 4:
        fitPrint(VERBOSE, "\tFCC1 RX clock is CLK09\n");
        break;
      case 5:
        fitPrint(VERBOSE, "\tFCC1 RX clock is CLK10\n");
        break;
      case 6:
        fitPrint(VERBOSE, "\tFCC1 RX clock is CLK11\n");
        break;
      case 7:
        fitPrint(VERBOSE, "\tFCC1 RX clock is CLK12\n");
        break;
      }
      switch(cmxfcr.bitField.tf1cs){
      case 0:
        fitPrint(VERBOSE, "\tFCC1 TX clock is BRG05\n");
        break;
      case 1:
        fitPrint(VERBOSE, "\tFCC1 TX clock is BRG06\n");
        break;
      case 2:
        fitPrint(VERBOSE, "\tFCC1 TX clock is BRG07\n");
        break;
      case 3:
        fitPrint(VERBOSE, "\tFCC1 TX clock is BRG08\n");
        break;
      case 4:
        fitPrint(VERBOSE, "\tFCC1 TX clock is CLK09\n");
        break;
      case 5:
        fitPrint(VERBOSE, "\tFCC1 TX clock is CLK10\n");
        break;
      case 6:
        fitPrint(VERBOSE, "\tFCC1 TX clock is CLK11\n");
        break;
      case 7:
        fitPrint(VERBOSE, "\tFCC1 TX clock is CLK12\n");
        break;
      }
    }

    if(cmxfcr.bitField.fc2 & 1){ //HACK incomplete check!
      fitPrint(VERBOSE, "FCC2 is connected to the TSA\n");
    } else {
      fitPrint(VERBOSE, "FCC2 is connected to the NMSI\n");
      switch(cmxfcr.bitField.rf2cs){
      case 0:
        fitPrint(VERBOSE, "\tFCC2 RX clock is BRG05\n");
        break;
      case 1:
        fitPrint(VERBOSE, "\tFCC2 RX clock is BRG06\n");
        break;
      case 2:
        fitPrint(VERBOSE, "\tFCC2 RX clock is BRG07\n");
        break;
      case 3:
        fitPrint(VERBOSE, "\tFCC2 RX clock is BRG08\n");
        break;
      case 4:
        fitPrint(VERBOSE, "\tFCC2 RX clock is CLK13\n");
        break;
      case 5:
        fitPrint(VERBOSE, "\tFCC2 RX clock is CLK14\n");
        break;
      case 6:
        fitPrint(VERBOSE, "\tFCC2 RX clock is CLK15\n");
        break;
      case 7:
        fitPrint(VERBOSE, "\tFCC2 RX clock is CLK16\n");
        break;
      }
      switch(cmxfcr.bitField.tf2cs){
      case 0:
        fitPrint(VERBOSE, "\tFCC2 TX clock is BRG05\n");
        break;
      case 1:
        fitPrint(VERBOSE, "\tFCC2 TX clock is BRG06\n");
        break;
      case 2:
        fitPrint(VERBOSE, "\tFCC2 TX clock is BRG07\n");
        break;
      case 3:
        fitPrint(VERBOSE, "\tFCC2 TX clock is BRG08\n");
        break;
      case 4:
        fitPrint(VERBOSE, "\tFCC2 TX clock is CLK13\n");
        break;
      case 5:
        fitPrint(VERBOSE, "\tFCC2 TX clock is CLK14\n");
        break;
      case 6:
        fitPrint(VERBOSE, "\tFCC2 TX clock is CLK15\n");
        break;
      case 7:
        fitPrint(VERBOSE, "\tFCC2 TX clock is CLK16\n");
        break;
      }
    }
#   endif // FCC3

    if(cmxfcr.bitField.fc3 & 1){
      fitPrint(VERBOSE, "FCC3 is connected to the TSA\n");
    } else {
      fitPrint(VERBOSE, "FCC3 is connected to the NMSI\n");
      switch(cmxfcr.bitField.rf3cs){
      case 0:
        fitPrint(VERBOSE, "\tFCC3 RX clock is BRG05\n");
        break;
      case 1:
        fitPrint(VERBOSE, "\tFCC3 RX clock is BRG06\n");
        break;
      case 2:
        fitPrint(VERBOSE, "\tFCC3 RX clock is BRG07\n");
        break;
      case 3:
        fitPrint(VERBOSE, "\tFCC3 RX clock is BRG08\n");
        break;
      case 4:
        fitPrint(VERBOSE, "\tFCC3 RX clock is CLK13\n");
        break;
      case 5:
        fitPrint(VERBOSE, "\tFCC3 RX clock is CLK14\n");
        break;
      case 6:
        fitPrint(VERBOSE, "\tFCC3 RX clock is CLK15\n");
        break;
      case 7:
        fitPrint(VERBOSE, "\tFCC3 RX clock is CLK16\n");
        break;
      }
      switch(cmxfcr.bitField.tf3cs){
      case 0:
        fitPrint(VERBOSE, "\tFCC3 TX clock is BRG05\n");
        break;
      case 1:
        fitPrint(VERBOSE, "\tFCC3 TX clock is BRG06\n");
        break;
      case 2:
        fitPrint(VERBOSE, "\tFCC3 TX clock is BRG07\n");
        break;
      case 3:
        fitPrint(VERBOSE, "\tFCC3 TX clock is BRG08\n");
        break;
      case 4:
        fitPrint(VERBOSE, "\tFCC3 TX clock is CLK13\n");
        break;
      case 5:
        fitPrint(VERBOSE, "\tFCC3 TX clock is CLK14\n");
        break;
      case 6:
        fitPrint(VERBOSE, "\tFCC3 TX clock is CLK15\n");
        break;
      case 7:
        fitPrint(VERBOSE, "\tFCC3 TX clock is CLK16\n");
        break;
      }
    }
    break;
  default:
    fitPrint(VERBOSE, "port %d is invalid\n",idx);
    break;
  }
}

// bits P0,P1,P2,P3 are only valid for PORTA and PORTC
int32 bchk(int32 port, u_int32 val)
{
  u_int32 lbits = 0;

  switch(port){
  case PORTA:
  case PORTC:
    break;
  case PORTB:
  case PORTD:
    lbits = ((P00|P01|P02|P03) & val);
    if(lbits){
      fitPrint(VERBOSE, "low order bits %8.8x are invalid for %s\n",lbits,port_str[port]);
      return(-1);
    }
    break;
  default:
    fitPrint(VERBOSE, "port %d is invalid\n",port);
    return(-2);
  }
  return(0);
}

//
// generated SYNC port config
//
void pcfg(int32 idx,struct cpm2_ioports *iop,u_int32 *cmxscc,u_int32 *cmxfcc)
{
  u_int32 iop_pdata = 0;
  u_int32 iop_pdira = 0;
  u_int32 iop_pdirb = 0;
  u_int32 iop_pdirc = 0;
  u_int32 iop_pdird = 0;
  u_int32 iop_ppara = 0;
  u_int32 iop_pparb = 0;
  u_int32 iop_pparc = 0;
  u_int32 iop_ppard = 0;
  u_int32 iop_psora = 0;
  u_int32 iop_psorb = 0;
  u_int32 iop_psorc = 0;
  u_int32 iop_psord = 0;

  u_int32 cmx_scr = 0;
  u_int32 cmx_fcr = 0;

  switch(idx){
  case 0: // SCC1_DEV_INDEX:  // sp1s
    // SCC1: TxD, RxD, RTS, CTS, CD
    //       PD30 PD31 PD29 PC15 PC14
    iop_ppard |=  (P31|P30|P29);
    iop_pdird |=  (P29|P30);
    iop_pdird &= ~(P31);
    iop_psord |=  (P30);
    iop_psord &= ~(P31|P29);

    // CD & CTS not under SCC
    iop_pparc &=  ~(P15|P14);

    // PC21=CLK11; PC31=BRG01
    iop_pparc |= (P21|P31);
    iop_pdirc &= ~P21;
    iop_pdirc |=  P31;
    iop_psorc &= ~(P21|P31);

    // Clear SCC1
    // Connect SCC1 RXC-EXT to CLK11 SCC1
    // Connect SCC1 TXC-INT to BRG01 SCC1
    cmx_scr &= ~(P00|P01|P02|P03|P04|P05|P06|P07);
    cmx_scr |=  (              P02               );

    // We set PA24 to an output here
    iop_ppara |=  P24;
    iop_pdira |=  P24;
    iop_psora &= ~P24;
    break;

  case 1: // SCC2_DEV_INDEX: // sp2s
    // SCC2: TxD, RxD, RTS, CTS, CD
    //       PB12 PB15 PD26 PC13 PC12

    iop_pparb |=  (P15|P12);
    iop_pdirb |=  (P12);
    iop_pdirb &= ~(P15);
    iop_psorb |=  (P12);
    iop_psorb &= ~(P15);

    iop_ppard |=  P26;
    iop_psord &= ~P26;
    iop_pdird |=  P26;

    iop_pparc &=  ~(P12|P13);

    // PC28=CLK4; PC29=BRG02
    iop_pparc |=  (P28|P29);
    iop_pdirc &=  ~P28;
    iop_pdirc |=   P29;
    iop_psorc &=  ~(P28|P29);

    // Connect SCC2 RXC-EXT to CLK4  SCC2
    // Connect SCC2 TXC-INT to BRG02 SCC2
    cmx_scr &= ~(P08|P09|P10|P11|P12|P13|P14|P15);
    cmx_scr |=  (              P10|P11|P12|        P15);

    // Special for ATC8270
    // PA23 switches SP1_TXC_EXT either to PC29 (i) or PC28 (o)
    // We set PA23 to a output here
    iop_ppara |=  P23;
    iop_pdira |=  P23;
    iop_psora &= ~P23;
    break;

  case 2: // SCC3_DEV_INDEX: // sp8s
    // SCC3: TxD, RxD, RTS, CTS, CD
    //       PD24 PB14 PD23 PC11 PC10
    iop_ppard |=  (P24|P23);
    iop_psord &= ~(P24|P23);
    iop_pdird |=  (P24|P23);

    iop_pparb |=  P14;
    iop_psorb &= ~P14;
    iop_pdirb &= ~P14;

    iop_pparc &=  ~(P10|P11);

    // PC25=CLK7; PC27=BRG03
    iop_pparc |=  (P25|P27);
    iop_pdirc &= ~P25;
    iop_pdirc |=  P27;
    iop_psorc &= ~(P25);
    iop_psorc |= (P27);

    // Connect SCC3 RXC-EXT to CLK7 SCC3
    // Connect SCC3 TXC-INT to BRG03 SCC3
    cmx_scr &= ~(P16|P17|P18|P19|P20|P21|P22|P23);
    cmx_scr |=  (              P18|P19|        P22    );
    break;

  case 3: // SCC4_DEV_INDEX:  // SDLC port (sp3s)
    // SCC4: TxD, RxD, RTS, CTS, CD
    //       PD21 PD22 PD20 PC9 PC8
    iop_ppard |=  (P20|P21|P22);
    iop_psord &= ~(P20|P21|P22);
    iop_pdird |=  (P20|P21);
    iop_pdird &= ~(P22);

    // Assert ~CTS & ~CD permanently into SCC4
    iop_pparc &= ~(P09|P08); // GPIO pins

    // PC26=CLK6; PD10=BRG04
    iop_pparc |=  P26;
    iop_pdirc &= ~P26;
    iop_psorc &= ~P26;
    iop_ppard |=  P10;
    iop_pdird |=  P10;
    iop_psord |=  P10;

    // Connect SCC4 RXC-EXT to CLK6  SCC4
    // Connect SCC4 TXC-INT to BRG04 SCC4
    cmx_scr &= ~(P24|P25|P26|P27|P28|P29|P30|P31);
    cmx_scr |=    (             P26      |P28      |P30|P31);

    // Special for ATC8270
    // PA22 switches  SP1_TXC_EXT to PC29 (i) or PC28 (o)
    // We set PA22 to an output here
    iop_ppara &= ~P22;
    iop_pdira |=  P22;
    iop_pdata |=  P22;
    break;

  case 4: // FCC3_DEV_INDEX: // sp5s
    // FCC3: TxD, RxD, RTS, CTS, CD
    //       PB07 PB08    -    -    -
    iop_pparb |=  (P08|P07);
    iop_psorb &= ~(P08|P07);
    iop_pdirb |=  (P07);    // output
    iop_pdirb &= ~(P08);    // input

    // assert ~CTS & ~CD into FCC3
    iop_pparc &= ~(P02|P03); // GPIO

    // PC19=CLK13; PC1=BRG06
    iop_pparc |= (P19|P01);
    iop_psorc &= ~(P19|P01);
    iop_pdirc &= ~P19; // input
    iop_pdirc |=  P01; // output

    // Connect FCC3 RXC-EXT to CLK13
    // Connect FCC3 TXC-INT to BRG06
    cmx_fcr &= ~(P17|P18|P19|P20|P21|P22|P23);
    cmx_fcr |=  (    P18|                P23);
    break;

  default:
    fitPrint(USER, "illegal port %d\n",idx);
    return;
  }

  switch(idx){
  case 0:
    fitPrint(USER, "\nSCC1 Parallel Port Register Programming\n\n");
    break;
  case 1:
    fitPrint(USER, "\nSCC2 Parallel Port Register Programming\n\n");
    if(bchk(idx,iop_pdirb)){ // valid bit consistancy check
      return;
    }
    if(bchk(idx,iop_pparb)){ // valid bit consistancy check
      return;
    }
    if(bchk(idx,iop_psorb)){ // valid bit consistancy check
      return;
    }
    break;
  case 2:
    fitPrint(USER, "\nSCC3 Parallel Port Register Programming\n\n");
    break;
  case 3:
    fitPrint(USER, "\nSCC4 Parallel Port Register Programming\n\n");
    if(bchk(idx,iop_pdird)){ // valid bit consistancy check
      return;
    }
    if(bchk(idx,iop_ppard)){ // valid bit consistancy check
      return;
    }
    if(bchk(idx,iop_psord)){ // valid bit consistancy check
      return;
    }
    break;
  case 4:
    fitPrint(USER, "\nFCC3 Parallel Port Register Programming\n\n");
    break;
  }

  fitPrint(USER, "\tiop_pdira\tiop_pdirb\tiop_pdirc\tiop_pdird\n");
  fitPrint(USER, " SYNC\t%8.8x\t%8.8x\t%8.8x\t%8.8x\n",iop_pdira,iop_pdirb,iop_pdirc,iop_pdird);
  fitPrint(USER, "ASYNC\t%8.8x\t%8.8x\t%8.8x\t%8.8x\n",iop[PORTA].dir,iop[PORTB].dir,iop[PORTC].dir,iop[PORTD].dir);
  //fitPrint(USER, " EXOR\t%8.8x\t%8.8x\t%8.8x\t%8.8x\n",iop_pdira^iop[PORTA].dir,iop_pdirb^iop[PORTB].dir,iop_pdirc^iop[PORTC].dir,iop_pdird^iop[PORTD].dir);
  fitPrint(USER, "  AND\t%8.8x\t%8.8x\t%8.8x\t%8.8x\n\n",iop_pdira&iop[PORTA].dir,iop_pdirb&iop[PORTB].dir,iop_pdirc&iop[PORTC].dir,iop_pdird&iop[PORTD].dir);

  fitPrint(USER, "\tiop_ppara\tiop_pparb\tiop_pparc\tiop_ppard\n");
  fitPrint(USER, " SYNC\t%8.8x\t%8.8x\t%8.8x\t%8.8x\n",iop_ppara,iop_pparb,iop_pparc,iop_ppard);
  fitPrint(USER, "ASYNC\t%8.8x\t%8.8x\t%8.8x\t%8.8x\n",iop[PORTA].par,iop[PORTB].par,iop[PORTC].par,iop[PORTD].par);
  //fitPrint(USER, " EXOR\t%8.8x\t%8.8x\t%8.8x\t%8.8x\n",iop_ppara^iop[PORTA].par,iop_pparb^iop[PORTB].par,iop_pparc^iop[PORTC].par,iop_ppard^iop[PORTD].par);
  fitPrint(USER, "  AND\t%8.8x\t%8.8x\t%8.8x\t%8.8x\n\n",iop_ppara&iop[PORTA].par,iop_pparb&iop[PORTB].par,iop_pparc&iop[PORTC].par,iop_ppard&iop[PORTD].par);

  fitPrint(USER, "\tiop_psora\tiop_psorb\tiop_psorc\tiop_psord\n");
  fitPrint(USER, " SYNC\t%8.8x\t%8.8x\t%8.8x\t%8.8x\n",iop_psora,iop_psorb,iop_psorc,iop_psord);
  fitPrint(USER, "ASYNC\t%8.8x\t%8.8x\t%8.8x\t%8.8x\n",iop[PORTA].sor,iop[PORTB].sor,iop[PORTC].sor,iop[PORTD].sor);
  //fitPrint(USER, " EXOR\t%8.8x\t%8.8x\t%8.8x\t%8.8x\n",iop_psora^iop[PORTA].sor,iop_psorb^iop[PORTB].sor,iop_psorc^iop[PORTC].sor,iop_psord^iop[PORTD].sor);
  fitPrint(USER, "  AND\t%8.8x\t%8.8x\t%8.8x\t%8.8x\n\n",iop_psora&iop[PORTA].sor,iop_psorb&iop[PORTB].sor,iop_psorc&iop[PORTC].sor,iop_psord&iop[PORTD].sor);

  fitPrint(USER, "\tcmx_scr\t\tcmx_fcr\n");
  fitPrint(USER, " SYNC\t%8.8x\t%8.8x\n",cmx_scr,cmx_fcr);
  fitPrint(USER, "ASYNC\t%8.8x\t%8.8x\n",*cmxscc,*cmxfcc);
  //fitPrint(USER, " EXOR\t%8.8x\n",cmx_scr^*cmxscc);
  fitPrint(USER, "  AND\t%8.8x\t%8.8x\n\n",cmx_scr&*cmxscc,cmx_fcr&*cmxfcc);

  //
  // do this smarter
  //
# if 1
  // PDIR

  fitPrint(USER, "\n SYNC Registers\n");
  pscan(PORTA,PDIR,iop_pdira);

  fitPrint(USER, "\nASYNC Registers\n");
  pscan(PORTA,PDIR,iop[PORTA].dir);

  fitPrint(USER, "\n SYNC Registers\n");
  pscan(PORTB,PDIR,iop_pdirb);

  fitPrint(USER, "\nASYNC Registers\n");
  pscan(PORTB,PDIR,iop[PORTB].dir);

  fitPrint(USER, "\n SYNC Registers\n");
  pscan(PORTC,PDIR,iop_pdirc);

  fitPrint(USER, "\nASYNC Registers\n");
  pscan(PORTC,PDIR,iop[PORTC].dir);

  fitPrint(USER, "\n SYNC Registers\n");
  pscan(PORTD,PDIR,iop_pdird);

  fitPrint(USER, "\nASYNC Registers\n");
  pscan(PORTD,PDIR,iop[PORTD].dir);

  // PPAR

  fitPrint(USER, "\n SYNC Registers\n");
  pscan(PORTA,PPAR,iop_ppara);

  fitPrint(USER, "\nASYNC Registers\n");
  pscan(PORTA,PPAR,iop[PORTA].par);

  fitPrint(USER, "\n SYNC Registers\n");
  pscan(PORTB,PPAR,iop_pparb);
  pscan(PORTB,PPAR,iop[PORTB].par);

  fitPrint(USER, "\n SYNC Registers\n");
  pscan(PORTC,PPAR,iop_pparc);

  fitPrint(USER, "\nASYNC Registers\n");
  pscan(PORTC,PPAR,iop[PORTC].par);

  fitPrint(USER, "\n SYNC Registers\n");
  pscan(PORTD,PPAR,iop_ppard);

  fitPrint(USER, "\nASYNC Registers\n");
  pscan(PORTD,PPAR,iop[PORTD].par);

  // PSOR

  fitPrint(USER, "\n SYNC Registers\n");
  pscan(PORTA,PSOR,iop_psora);

  fitPrint(USER, "\nASYNC Registers\n");
  pscan(PORTA,PSOR,iop[PORTA].sor);

  fitPrint(USER, "\n SYNC Registers\n");
  pscan(PORTB,PSOR,iop_psorb);
  pscan(PORTB,PSOR,iop[PORTB].sor);

  fitPrint(USER, "\n SYNC Registers\n");
  pscan(PORTC,PSOR,iop_psorc);

  fitPrint(USER, "\nASYNC Registers\n");
  pscan(PORTC,PSOR,iop[PORTC].sor);

  fitPrint(USER, "\n SYNC Registers\n");
  pscan(PORTD,PSOR,iop_psord);

  fitPrint(USER, "\nASYNC Registers\n");
  pscan(PORTD,PSOR,iop[PORTD].sor);

  pscan(MUXSCR,CMXS,cmx_scr);
  pscan(MUXSCR,CMXS,*cmxscc);
# endif // pdir

  if(idx < 4) {
    clk_route_reg(idx,cmx_scr);
    clk_route_reg(idx,*cmxscc);
  } else {
    clk_route_reg(idx,cmx_fcr);
    clk_route_reg(idx,*cmxfcc);
  }
}

//
// get the current port programming
//
void get_pports(void)
{
  int32 fd;
  char *ptr;
  u_int32 adrs = 0xf0010000u;
  u_int32 *cmxscc;
  u_int32 *cmxfcc;
  struct cpm2_ioports  *iop;
  int32 i;
  //char dd;

  fd = open ("/dev/mem", 2);

  if (fd == -1)
  {
    fitPrint(ERROR, "\nUnable to open /dev/mem\n");
    return;
  }

  ptr = mmap (0, 0x2000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, adrs);
  iop = (struct cpm2_ioports *) (ptr + 0xd00);
  cmxfcc = (u_int32 *) (ptr + 0x1b04);
  cmxscc = (u_int32 *) (ptr + 0x1b08);

  for(i=PORTA; i <= 4; i++)
  {
    pcfg(i,iop,cmxscc,cmxfcc);
  }

# if 0
  fitPrint(USER, "\n\n CMX SCC = 0x%08x\n", *cmxscc);

  for(i=0; i<4; i++) {
    dd = *cmxscc >> (i *8);
    fitPrint(USER, "SCC%d TX clock = ",  4 - i);
    if(dd & 4)
      fitPrint(USER, "CLK%d  ", 5 + (dd & 0x3) );
    else
      fitPrint(USER, "BRG%d  ", 1 + (dd & 0x3) );
    dd = dd >> 3;
    fitPrint(USER, "RX clock = ");
    if(dd & 4)
      fitPrint(USER, "CLK%d\n", 5 + (dd & 0x3) );
    else
      fitPrint(USER, "BRG%d\n", 1 + ( dd & 0x3) );
  }
# endif // pports

  close (fd);

  return;
}

# ifdef INTERNAL_LOOPBACK
//
// set both internal loopback and echo on
// for FCC3,SCC1,SCC2,SCC3 and SCC4.
//
// TODO: need to swap clocks for synchronous
//           and flow control signals for modems
//
// FCC1 11300
// FCC2 11320
// FCC3 11340
// SCC1 11a00
// SCC2 11a20
// SCC3 11a40
// SCC4 11a60
//

static void set_loopback_echo(void)
{
  int32 fd;
  char *ptr;
  u_int32 adrs = 0xF0010000u;
  u_int32 *gmrp; // general mode register

  fd = open ("/dev/mem", 2);

  if (fd == -1)
  {
    fitPrint(ERROR, "\nUnable to open /dev/mem\n");
    return;
  }

  ptr = mmap (NULL, 0x2000u, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (int32)adrs);

  //FCC3
  gmrp = (u_int32 *) (&ptr[0x1340]);
  *gmrp |= P00|P01;
  //SCC1
  gmrp = (u_int32 *) (&ptr[0x1a00]);
  *gmrp |= P24|P25;
  //SCC2
  gmrp = (u_int32 *) (&ptr[0x1a20]);
  *gmrp |= P24|P25;
  //SCC3
  gmrp = (u_int32 *) (&ptr[0x1a40]);
  *gmrp |= P24|P25;
  //SCC4
  gmrp = (u_int32 *) (&ptr[0x1a60]);
  *gmrp |= P24|P25;

  close (fd);

  return;
}
# endif //internal loopback

#endif //master disable
// ====================== End of Inactive Code =======================

// ====================== Start of Active Code =======================
/*
* SP1 ASYNC Baud Rate Data Initialization
*/
static bRate_t sp1BR[] = { // asynchronous baud rates for sp1
  {   B1200,0,0,0,0,NULL },
  {   B2400,0,0,0,0,NULL },
  {   B4800,0,0,0,0,NULL },
  {   B9600,0,0,0,0,NULL },
  {  B19200,0,0,0,0,NULL },
  {  B38400,0,0,0,0,NULL },
  {  B57600,0,0,0,0,NULL },
  { B115200,0,0,0,0,NULL },
  {       0,0,0,0,0,NULL },
};

static sem_t     mon1Mutex;              // test executor mutex
static sem_t     sp1Mutex_r, sp1Mutex_w; // test thread mutexs
static sem_t     sp1Lock;                // port locking mutex
static pthread_t sp1Tid_r, sp1Tid_w;     // test thread IDs
static char      sp1Buf_r[MAX_TRANSFER], sp1Buf_w[MAX_TRANSFER]; // COM buffers

/*
* SP1s SYNC Baud Rate Data Initialization
*/
static bRate_t sp1sBR[] = { // synchronous baud rates for sp1s
  {  19200,0,0,0,0,NULL },
  {  38400,0,0,0,0,NULL },
  {  57600,0,0,0,0,NULL },
  {  76800,0,0,0,0,NULL },
  { 153600,0,0,0,0,NULL },
  {      0,0,0,0,0,NULL },
};

static sem_t     mon1sMutex;               // test executor mutex
static sem_t     sp1sMutex_r, sp1sMutex_w; // test thread mutexs
static sem_t     sp1sLock;                 // port locking mutex
static pthread_t sp1sTid_r, sp1sTid_w;     // test thread IDs
static char      sp1sBuf_r[MAX_TRANSFER], sp1sBuf_w[MAX_TRANSFER]; // COM buffers

/*
* SP2 ASYNC Baud Rate Data Initialization
*/
static bRate_t sp2BR[] = { // asynchronous baud rates for sp2
  {   B1200,0,0,0,0,NULL },
  {   B2400,0,0,0,0,NULL },
  {   B4800,0,0,0,0,NULL },
  {   B9600,0,0,0,0,NULL },
  {  B19200,0,0,0,0,NULL },
  {  B38400,0,0,0,0,NULL },
  {  B57600,0,0,0,0,NULL },
  { B115200,0,0,0,0,NULL },
  {       0,0,0,0,0,NULL },
};

static sem_t     mon2Mutex;              // test executor mutex
static sem_t     sp2Mutex_r, sp2Mutex_w; // test thread mutexs
static sem_t     sp2Lock;                // port locking mutex
static pthread_t sp2Tid_r, sp2Tid_w;     // test thread IDs
static char      sp2Buf_r[MAX_TRANSFER], sp2Buf_w[MAX_TRANSFER]; // COM buffers

/*
* SP2s SYNC Baud Rate Data Initialization
*/
static bRate_t sp2sBR[] = { // synchronous baud rates for sp2s
  {  19200,0,0,0,0,NULL },
  {  38400,0,0,0,0,NULL },
  {  57600,0,0,0,0,NULL },
  {  76800,0,0,0,0,NULL },
  { 153600,0,0,0,0,NULL },
  {      0,0,0,0,0,NULL },
};

static sem_t     mon2sMutex;               // test executor mutex
static sem_t     sp2sMutex_r, sp2sMutex_w; // test thread mutexs
static sem_t     sp2sLock;                 // port locking mutex
static pthread_t sp2sTid_r, sp2sTid_w;     // test thread IDs
static char      sp2sBuf_r[MAX_TRANSFER], sp2sBuf_w[MAX_TRANSFER]; // COM buffers

/*
* SP3 ASYNC Baud Rate Data Initialization
*/
static bRate_t sp3BR[] = { // asynchronous baud rates for sp3
  {   B1200,0,0,0,0,NULL },
  {   B2400,0,0,0,0,NULL },
  {   B4800,0,0,0,0,NULL },
  {   B9600,0,0,0,0,NULL },
  {  B19200,0,0,0,0,NULL },
  {  B38400,0,0,0,0,NULL },
  {  B57600,0,0,0,0,NULL },
  { B115200,0,0,0,0,NULL },
  {       0,0,0,0,0,NULL },
};

static sem_t     mon3Mutex;              // test executor mutex
static sem_t     sp3Mutex_r, sp3Mutex_w; // test thread mutexs
static sem_t     sp3Lock;                // port locking mutex
static pthread_t sp3Tid_r, sp3Tid_w;     // test thread IDs
static char      sp3Buf_r[MAX_TRANSFER], sp3Buf_w[MAX_TRANSFER]; // COM buffers

/*
* SP3s SYNC Baud Rate Data Initialization
*/
static bRate_t sp3sBR[] = { // synchronous baud rates for sp3s
  { 153600,0,0,0,0,NULL },
  { 614400,0,0,0,0,NULL },
  {      0,0,0,0,0,NULL },
};

static sem_t     mon3sMutex;               // test executor mutex
static sem_t     sp3sMutex_r, sp3sMutex_w; // test thread mutexs
static sem_t     sp3sLock;                 // port locking mutex
static pthread_t sp3sTid_r, sp3sTid_w;     // test thread IDs
static char      sp3sBuf_r[MAX_TRANSFER], sp3sBuf_w[MAX_TRANSFER]; // COM buffers

/*
* SP4 ASYNC Baud Rate Data Initialization
*/
static bRate_t sp4BR[] = { // asynchronous baud rates for sp4
  {   B1200,0,0,0,0,NULL },
  {   B2400,0,0,0,0,NULL },
  {   B4800,0,0,0,0,NULL },
  {   B9600,0,0,0,0,NULL },
  {  B19200,0,0,0,0,NULL },
  {  B38400,0,0,0,0,NULL },
  {  B57600,0,0,0,0,NULL },
  { B115200,0,0,0,0,NULL },
  {       0,0,0,0,0,NULL },
};

static sem_t     mon4Mutex;              // test executor mutex
static sem_t     sp4Mutex_r, sp4Mutex_w; // test thread mutexs
static sem_t     sp4Lock;                // port locking mutex
static pthread_t sp4Tid_r, sp4Tid_w;     // test thread IDs
static char      sp4Buf_r[MAX_TRANSFER], sp4Buf_w[MAX_TRANSFER]; // COM buffers

/*
* SP5s SYNC Baud Rate Data Initialization
*/
static bRate_t sp5sBR[] = { // synchronous baud rates for sp5s
  { 153600,0,0,0,0,NULL },
  { 614400,0,0,0,0,NULL },
  {      0,0,0,0,0,NULL },
};

static sem_t     mon5sMutex;               // test executor mutex
static sem_t     sp5sMutex_r, sp5sMutex_w; // test thread mutexs
static sem_t     sp5sLock;                 // port locking mutex
static pthread_t sp5sTid_r, sp5sTid_w;     // test thread IDs
static char      sp5sBuf_r[MAX_TRANSFER], sp5sBuf_w[MAX_TRANSFER]; // COM buffers

/*
* SP6 ASYNC Baud Rate Data Initialization
*/
static bRate_t sp6BR[] = { // asynchronous baud rates for sp6
  {   B1200,0,0,0,0,NULL },
  {   B2400,0,0,0,0,NULL },
  {   B4800,0,0,0,0,NULL },
  {   B9600,0,0,0,0,NULL },
  {  B19200,0,0,0,0,NULL },
  {  B38400,0,0,0,0,NULL },
  {  B57600,0,0,0,0,NULL },
  { B115200,0,0,0,0,NULL },
  {       0,0,0,0,0,NULL },
};

static sem_t     mon6Mutex;              // test executor mutex
static sem_t     sp6Mutex_r, sp6Mutex_w; // test thread mutexs
static sem_t     sp6Lock;                // port locking mutex
static pthread_t sp6Tid_r, sp6Tid_w;     // test thread IDs
static char      sp6Buf_r[MAX_TRANSFER], sp6Buf_w[MAX_TRANSFER]; // COM buffers

/*
* SP8 ASYNC Baud Rate Data Initialization
*/
static bRate_t sp8BR[] = { // asynchronous baud rates for sp8
  {   B1200,0,0,0,0,NULL },
  {   B2400,0,0,0,0,NULL },
  {   B4800,0,0,0,0,NULL },
  {   B9600,0,0,0,0,NULL },
  {  B19200,0,0,0,0,NULL },
  {  B38400,0,0,0,0,NULL },
  {  B57600,0,0,0,0,NULL },
  { B115200,0,0,0,0,NULL },
  {       0,0,0,0,0,NULL },
};

static sem_t     mon8Mutex;              // test executor mutex
static sem_t     sp8Mutex_r, sp8Mutex_w; // test thread mutexs
static sem_t     sp8Lock;                // port locking mutex
static pthread_t sp8Tid_r, sp8Tid_w;     // test thread IDs
static char      sp8Buf_r[MAX_TRANSFER], sp8Buf_w[MAX_TRANSFER]; // COM buffers

/*
* SP8s SYNC Baud Rate Data Initialization
*/
static bRate_t sp8sBR[] = { // synchronous baud rates for sp8s
  {  19200,0,0,0,0,NULL },
  {  38400,0,0,0,0,NULL },
  {  57600,0,0,0,0,NULL },
  {  76800,0,0,0,0,NULL },
  { 153600,0,0,0,0,NULL },
//{ 614400,0,0,0,0,NULL },
  {      0,0,0,0,0,NULL },
};

static sem_t     mon8sMutex;               // test executor mutex
static sem_t     sp8sMutex_r, sp8sMutex_w; // test thread mutexs
static sem_t     sp8sLock;                 // port locking mutex
static pthread_t sp8sTid_r, sp8sTid_w;     // test thread IDs
static char      sp8sBuf_r[MAX_TRANSFER], sp8sBuf_w[MAX_TRANSFER]; // COM buffers

/*
* Initialized DAT Data
*/
static spDatDat_t spDatDat[] = {
  /*
  * Single asynchronous ports that are physically looped back to themselves.
  */
  { "AsyncExtLoopback","sp1","sp1",&mon1Mutex,&sp1Mutex_r,&sp1Mutex_w,&sp1Lock,
    &sp1Tid_r,&sp1Tid_w,sp1Buf_r,sp1Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp1BR,NULL},

  { "AsyncExtLoopback","sp2","sp2",&mon2Mutex,&sp2Mutex_r,&sp2Mutex_w,&sp2Lock,
    &sp2Tid_r,&sp2Tid_w,sp2Buf_r,sp2Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp2BR,NULL},

  { "AsyncExtLoopback","sp3","sp3",&mon3Mutex,&sp3Mutex_r,&sp3Mutex_w,&sp3Lock,
    &sp3Tid_r,&sp3Tid_w,sp3Buf_r,sp3Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp3BR,NULL},

  { "AsyncExtLoopback","sp4","sp4",&mon4Mutex,&sp4Mutex_r,&sp4Mutex_w,&sp4Lock,
    &sp4Tid_r,&sp4Tid_w,sp4Buf_r,sp4Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp4BR,NULL},

  { "AsyncExtLoopback","sp6","sp6",&mon6Mutex,&sp6Mutex_r,&sp6Mutex_w,&sp6Lock,
    &sp6Tid_r,&sp6Tid_w,sp6Buf_r,sp6Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp6BR,NULL},

  { "AsyncExtLoopback","sp8","sp8",&mon8Mutex,&sp8Mutex_r,&sp8Mutex_w,&sp8Lock,
    &sp8Tid_r,&sp8Tid_w,sp8Buf_r,sp8Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp8BR,NULL},

  /*
  * Single synchronous ports that are physically looped back to themselves.
  */
  { "SyncExtLoopback","sp1s","sp1s",&mon1sMutex,&sp1sMutex_r,&sp1sMutex_w,
    &sp1sLock,&sp1sTid_r,&sp1sTid_w, sp1sBuf_r,sp1sBuf_w,0,32,32,1,protSync,
    -1,-1,unscheduled,unscheduled,sp1sBR,NULL},

  { "SyncExtLoopback","sp2s","sp2s",&mon2sMutex,&sp2sMutex_r,&sp2sMutex_w,
    &sp2sLock,&sp2sTid_r,&sp2sTid_w, sp2sBuf_r,sp2sBuf_w,0,32,32,1,protSync,
    -1,-1,unscheduled,unscheduled,sp2sBR,NULL},

  { "SyncExtLoopback","sp3s","sp3s",&mon3sMutex,&sp3sMutex_r,&sp3sMutex_w,
    &sp3sLock,&sp3sTid_r,&sp3sTid_w,sp3sBuf_r,sp3sBuf_w,0,32,32,1,protSync,
    -1,-1,unscheduled,unscheduled,sp3sBR,NULL},

  { "SyncExtLoopback","sp5s","sp5s",&mon5sMutex,&sp5sMutex_r,&sp5sMutex_w,
    &sp5sLock,&sp5sTid_r,&sp5sTid_w,sp5sBuf_r,sp5sBuf_w,0,32,32,1,protSync,
    -1,-1,unscheduled,unscheduled,sp5sBR,NULL},

  { "SyncExtLoopback","sp8s","sp8s",&mon8sMutex,&sp8sMutex_r,&sp8sMutex_w,
    &sp8sLock,&sp8sTid_r,&sp8sTid_w,sp8sBuf_r,sp8sBuf_w,0,32,32,1,protSync,
    -1,-1,unscheduled,unscheduled,sp8sBR,NULL},

  /*
  * This data defines sp1 cross-connected to sp2.
  */
  { "AsyncExtLoopback","sp1","sp2",&mon1Mutex,&sp1Mutex_r,&sp2Mutex_w,&sp1Lock,
    &sp1Tid_r,&sp2Tid_w,sp1Buf_r,sp2Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp1BR,NULL},
  { "AsyncExtLoopback","sp2","sp1",&mon2Mutex,&sp2Mutex_r,&sp1Mutex_w,&sp1Lock,
    &sp2Tid_r,&sp1Tid_w,sp2Buf_r,sp1Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp2BR,NULL},

  /*
  * This data defines sp1 cross-connected to sp3.
  */
  { "AsyncExtLoopback","sp1","sp3",&mon1Mutex,&sp1Mutex_r,&sp3Mutex_w,&sp1Lock,
    &sp1Tid_r,&sp3Tid_w,sp1Buf_r,sp3Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp1BR,NULL},
  { "AsyncExtLoopback","sp3","sp1",&mon3Mutex,&sp3Mutex_r,&sp1Mutex_w,&sp1Lock,
    &sp3Tid_r,&sp1Tid_w,sp3Buf_r,sp1Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp3BR,NULL},

  /*
  * This data defines sp1 cross-connected to sp8.
  */
  { "AsyncExtLoopback","sp1","sp8",&mon1Mutex,&sp1Mutex_r,&sp8Mutex_w,&sp1Lock,
    &sp1Tid_r,&sp8Tid_w,sp1Buf_r,sp8Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp1BR,NULL},
  { "AsyncExtLoopback","sp8","sp1",&mon8Mutex,&sp8Mutex_r,&sp1Mutex_w,&sp1Lock,
    &sp8Tid_r,&sp1Tid_w,sp8Buf_r,sp1Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp8BR,NULL},

  /*
  * This data defines sp2 cross-connected to sp3.
  */
  { "AsyncExtLoopback","sp2","sp3",&mon2Mutex,&sp2Mutex_r,&sp3Mutex_w,&sp2Lock,
    &sp2Tid_r,&sp3Tid_w,sp2Buf_r,sp3Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp2BR,NULL},
  { "AsyncExtLoopback","sp3","sp2",&mon3Mutex,&sp3Mutex_r,&sp2Mutex_w,&sp2Lock,
    &sp3Tid_r,&sp2Tid_w,sp3Buf_r,sp2Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp3BR,NULL},

  /*
  * This data defines sp2 cross-connected to sp8.
  */
  { "AsyncExtLoopback","sp2","sp8",&mon2Mutex,&sp2Mutex_r,&sp8Mutex_w,&sp2Lock,
    &sp2Tid_r,&sp8Tid_w,sp2Buf_r,sp8Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp2BR,NULL},
  { "AsyncExtLoopback","sp8","sp2",&mon8Mutex,&sp8Mutex_r,&sp2Mutex_w,&sp2Lock,
    &sp8Tid_r,&sp2Tid_w,sp8Buf_r,sp2Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp8BR,NULL},

  /*
  * This data defines sp3 cross-connected to sp8.
  */
  { "AsyncExtLoopback","sp3","sp8",&mon3Mutex,&sp3Mutex_r,&sp8Mutex_w,&sp3Lock,
    &sp3Tid_r,&sp8Tid_w,sp3Buf_r,sp8Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp3BR,NULL},
  { "AsyncExtLoopback","sp8","sp3",&mon8Mutex,&sp8Mutex_r,&sp3Mutex_w,&sp3Lock,
    &sp8Tid_r,&sp3Tid_w,sp8Buf_r,sp3Buf_w,0,32,32,1,protAsync,-1,-1,unscheduled,
    unscheduled,sp8BR,NULL},


  /*
  * This data defines sp1s cross-connected to sp2s.
  */
  { "SyncExtLoopback","sp1s","sp2s",&mon1sMutex,&sp1sMutex_r,&sp2sMutex_w,&sp1sLock,&sp1sTid_r,&sp2sTid_w,
  sp1sBuf_r,sp2sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,unscheduled,sp1sBR,NULL},
  { "SyncExtLoopback","sp2s","sp1s",&mon2sMutex,&sp2sMutex_r,&sp1sMutex_w,&sp1sLock,&sp2sTid_r,&sp1sTid_w,
  sp2sBuf_r,sp1sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,unscheduled,sp2sBR,NULL},

  /*
  * This data defines sp1s cross-connected to sp3s.
  */
  { "SyncExtLoopback","sp1s","sp3s",&mon1sMutex,&sp1sMutex_r,&sp3sMutex_w,&sp1sLock,
    &sp1sTid_r,&sp3sTid_w,  sp1sBuf_r,sp3sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp1sBR,NULL},
  { "SyncExtLoopback","sp3s","sp1s",&mon3sMutex,&sp3sMutex_r,&sp1sMutex_w,&sp1sLock,
    &sp3sTid_r,&sp1sTid_w,sp3sBuf_r,sp1sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp3sBR,NULL},

  /*
  * This data defines sp1s cross-connected to sp5s.
  */
  { "SyncExtLoopback","sp1s","sp5s",&mon1sMutex,&sp1sMutex_r,&sp5sMutex_w,&sp1sLock,
    &sp1sTid_r,&sp5sTid_w,sp1sBuf_r,sp5sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp1sBR,NULL},
  { "SyncExtLoopback","sp5s","sp1s",&mon5sMutex,&sp5sMutex_r,&sp1sMutex_w,&sp1sLock,
    &sp5sTid_r,&sp1sTid_w,sp5sBuf_r,sp1sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp5sBR,NULL},

  /*
  * This data defines sp1s cross-connected to sp8s.
  */
  { "SyncExtLoopback","sp1s","sp8s",&mon1sMutex,&sp1sMutex_r,&sp8sMutex_w,&sp1sLock,
    &sp1sTid_r,&sp8sTid_w,sp1sBuf_r,sp8sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp1sBR,NULL},
  { "SyncExtLoopback","sp8s","sp1s",&mon8sMutex,&sp8sMutex_r,&sp1sMutex_w,&sp1sLock,
    &sp8sTid_r,&sp1sTid_w,sp8sBuf_r,sp1sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp8sBR,NULL},

  /*
  * This data defines sp2s cross-connected to sp3s.
  */
  { "SyncExtLoopback","sp2s","sp3s",&mon2sMutex,&sp2sMutex_r,&sp3sMutex_w,&sp2sLock,
    &sp2sTid_r,&sp3sTid_w,sp2sBuf_r,sp3sBuf_w,0,0,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp2sBR,NULL},
  { "SyncExtLoopback","sp3s","sp2s",&mon3sMutex,&sp3sMutex_r,&sp2sMutex_w,&sp2sLock,
    &sp3sTid_r,&sp2sTid_w,sp3sBuf_r,sp2sBuf_w,0,0,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp3sBR,NULL},

  /*
  * This data defines sp2s cross-connected to sp5s.
  */
  { "SyncExtLoopback","sp2s","sp5s",&mon2sMutex,&sp2sMutex_r,&sp5sMutex_w,&sp2sLock,
    &sp2sTid_r,&sp5sTid_w,sp2sBuf_r,sp5sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp2sBR,NULL},
  { "SyncExtLoopback","sp5s","sp2s",&mon5sMutex,&sp5sMutex_r,&sp2sMutex_w,&sp2sLock,
    &sp5sTid_r,&sp2sTid_w,sp5sBuf_r,sp2sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp5sBR,NULL},

  /*
  * This data defines sp2s cross-connected to sp8s.
  */
  { "SyncExtLoopback","sp2s","sp8s",&mon2sMutex,&sp2sMutex_r,&sp8sMutex_w,&sp2sLock,
    &sp2sTid_r,&sp8sTid_w,sp2sBuf_r,sp8sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp2sBR,NULL},
  { "SyncExtLoopback","sp8s","sp2s",&mon8sMutex,&sp8sMutex_r,&sp2sMutex_w,&sp2sLock,
    &sp8sTid_r,&sp2sTid_w,sp8sBuf_r,sp2sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp8sBR,NULL},

  /*
  * This data defines sp3s cross-connected to sp5s.
  */
  { "SyncExtLoopback","sp3s","sp5s",&mon3sMutex,&sp3sMutex_r,&sp5sMutex_w,&sp3sLock,
    &sp3sTid_r,&sp5sTid_w,sp3sBuf_r,sp5sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp3sBR,NULL},
  { "SyncExtLoopback","sp5s","sp3s",&mon5sMutex,&sp5sMutex_r,&sp3sMutex_w,&sp3sLock,
    &sp5sTid_r,&sp3sTid_w,sp5sBuf_r,sp3sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp5sBR,NULL},

  /*
  * This data defines sp3s cross-connected to sp8s.
  */
  { "SyncExtLoopback","sp3s","sp8s",&mon3sMutex,&sp3sMutex_r,&sp8sMutex_w,&sp3sLock,
    &sp3sTid_r,&sp8sTid_w,sp3sBuf_r,sp8sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp3sBR,NULL},
  { "SyncExtLoopback","sp8s","sp3s",&mon8sMutex,&sp8sMutex_r,&sp3sMutex_w,&sp3sLock,
    &sp8sTid_r,&sp3sTid_w,sp8sBuf_r,sp3sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp8sBR,NULL},

  /*
  * This data defines sp5s cross-connected to sp8s.
  */
  { "SyncExtLoopback","sp5s","sp8s",&mon5sMutex,&sp5sMutex_r,&sp8sMutex_w,&sp5sLock,
    &sp5sTid_r,&sp8sTid_w,sp5sBuf_r,sp8sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp5sBR,NULL},
  { "SyncExtLoopback","sp8s","sp5s",&mon8sMutex,&sp8sMutex_r,&sp5sMutex_w,&sp5sLock,
    &sp8sTid_r,&sp5sTid_w,sp8sBuf_r,sp5sBuf_w,0,32,32,1,protSync,-1,-1,unscheduled,
    unscheduled,sp8sBR,NULL},

  /*
  * This ends the list.
  */
  { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    0,0,0,0,protNone,-1,-1,unscheduled,unscheduled,NULL,NULL}
};

typedef enum {
  waitAndScroll,
  waitAndUnblock,
  waitAndBlock
} monitorLoop_t;

static monitorLoop_t monitorLoop = waitAndBlock;

static int32   rxFlags = O_RDWR  /*|O_NONBLOCK|O_ASYNC|O_NOCTTY|O_NDELAY*/;
static int32   txFlags = O_RDWR  /*|O_NONBLOCK|O_ASYNC|O_NOCTTY|O_NDELAY*/;
static u_int32 baudRateOveride = 0;   // don't overide
static u_int8  rxType = 2;            // choose receiver software design

static int32   tmoMultiplier = 1000;  // timeout multiplier
#ifdef PARALLEL_PORTS
static bool    showParallelPorts = false; // don't show
#endif
static bool    quickFail = false;         // run test to completion
static bool    enableFlowControl = false; // default flow control to disabled
static bool    useAtcTestString = false;  // use ATC 6.24 test string for data
#ifdef REMOTE_CONTROLLER
static bool    remoteController = false;  // not a remote controller
#endif

// Test string from ATC 6.24 §8.1.1 as follows:
// ">The quick[0xF7][0xFE]brow[0xBE]n_fox jumps }over the~|azy dog?[0xFE]"
static const char atcTestString[] =
          ">The quick\xF7\xFE" "brow\xBE" "n_fox jumps }over the~|azy dog?\xFE";

#define MICROSECONDS_IN_SEC 1000000
#define ASYNC_BITS_PER_BYTE 10 // Use this for both sync/async since it is the greater


// ==================== End of Active Declarations ======================

/*
* Map the low density asynchronous speed_t rates to baud rate for printing.
*/
static u_int32 mapBaudRate (speed_t br)
{
  u_int32 ret;

  switch (br) {
    case   B1200:
      ret = 1200;
      break;
    case   B2400:
      ret = 2400;
      break;
    case   B4800:
      ret = 4800;
      break;
    case   B9600:
      ret = 9600;
      break;
    case  B19200:
      ret = 19200;
      break;
    case  B38400:
      ret = 38400;
      break;
    case  B57600:
      ret = 57600;
      break;
    case B115200:
      ret = 115200;
      break;
    default: // these are the synchronous rates
      ret =  br;
      break;
  }

  return ret;
}

static int32 mapsdlcbaud(u_int32 rate)
{
    int32 cnt;

    for(cnt = 0; cnt < 11; cnt++)
    {
      if (rate == ATC_B[cnt])
      {
        break;
      }
    }

    if (cnt == 11)
    {
      fitPrint(ERROR, "%s Baudrate not supported", __func__);
      cnt = -1;
    }

    return cnt;
}

/*
* Set the new baud rate*
* NOTE: if retVal is 0 or positive, it represents the old
* rate. Otherwise, it is an error code.
*/
static int32 set_spx_bit_rate (int32 fd, u_int32 rate)
{
  int32    retVal = 0;
  int32    tmprate;
  user_spx_ioctl_t user_spx_ioctl_data;
  atc_spxs_config_t spxs;

  switch(targetARCH) {
    case ARCH_82XX:
      memset(&user_spx_ioctl_data,0,sizeof(user_spx_ioctl_t));

      retVal = ioctl (fd, ATC_SPXS_READ_CONFIG, &user_spx_ioctl_data);
      if (retVal >= 0)
      {
        user_spx_ioctl_data.x[1] = rate; // new baud rate
        retVal = ioctl (fd, ATC_SPXS_WRITE_CONFIG, &user_spx_ioctl_data);
      }
      break;
    case ARCH_UNKNOWN:  // ATC 6
    case ARCH_83XX:
      spxs.protocol = 0;
      tmprate = mapsdlcbaud(rate);
      if (tmprate == -1)
      {
        retVal = -1;
      }
      else
      {
        spxs.baud = (u_int8)tmprate;
        spxs.transmit_clock_source = 0;
        spxs.transmit_clock_mode = 1;
        retVal = ioctl (fd, ATC_SPXS_WRITE_CONFIG, &spxs);
      }
      break;
    default:
      retVal = -1;
      break;
  }

  if (retVal < 0) {
    fitPrint(ERROR, "set_spx_bit_rate: ioctl command %d failed, ret %ld, err %d, %s\n",
             ATC_SPXS_WRITE_CONFIG,retVal,errno,strerror(errno));
    return (retVal);
  }
  return (retVal);
}

#ifdef DUMP_TERMIOS
static void dumpTermios(const struct termios *tp)
{
  int32 i;
  fprintf(stderr,"Size of flags is %u\n", sizeof(tp->c_cflag));
  fprintf(stderr,"%s:\tiflags:%o oflags:%o cflags:%o \n",
          __func__,tp->c_iflag,tp->c_oflag,tp->c_cflag);
  fprintf(stderr,"\t\tlflags:0x%x line-disc:%u ispeed:%u ospeed:%u\n",
         tp->c_lflag,tp->c_line,tp->c_ispeed,tp->c_ospeed);
  fprintf(stderr,"%s: control chars:",__func__);
  for(i=0;i<NCCS;i++)
    fprintf(stderr," %02x",tp->c_cc[i]);
  fprintf(stderr,"\n");
}
#endif

void initSerialPort (int32 fd,speed_t baudRate,spProt_t protocol,bool flowCntrl)
{
  struct termios t;

  tcgetattr (fd,&t);

  cfmakeraw (&t);

  t.c_cc[VMIN]  = 1;
  t.c_cc[VTIME] = 2;

  if (flowCntrl) {
     t.c_cflag |= CRTSCTS;
  }

  switch (protocol) {
    case protAsync:
      /*
      * TODO: change interface to verify independent input/output rates.
      */
      (void) cfsetispeed (&t,baudRate);
      (void) cfsetospeed (&t,baudRate);
      break;
    case protSync:
      (void) set_spx_bit_rate (fd,baudRate);
      break;
    case protNone:
    default:
      // nothing to do
      break;
  }

  (void) tcflush (fd,TCIOFLUSH); // Flush the input and output

  #ifdef DUMP_TERMIOS
  dumpTermios(&t);
  #endif

  tcsetattr (fd,TCSANOW,&t);
}

static void calculateTimeout (const spDatDat_t *spdp, struct timeval *tv)
{
  int32 numBytes, bitsPerSec;
  int32 timeoutUsecs;

  bitsPerSec = (int32)mapBaudRate(spdp->lbrp->baudRate);
  numBytes = (int32)spdp->comSz;

  timeoutUsecs = ((numBytes * ASYNC_BITS_PER_BYTE * MICROSECONDS_IN_SEC) / bitsPerSec) ;
  /* Arbitrary x times longer than it should take
   * 5 mostly worked but we received some erroneous timeouts with certain packet sizes
   * 50 times longer should be more than enough
   */
  timeoutUsecs *= tmoMultiplier;

  //fitPrint(VERBOSE, "timeout for BAUD %d, numBytes %d waits %lu microseconds\n", bitsPerSec, numBytes, timeoutUsecs);

  /* Wait up to one second. */
  tv->tv_sec = (int32)timeoutUsecs / MICROSECONDS_IN_SEC;
  tv->tv_usec = timeoutUsecs % MICROSECONDS_IN_SEC;
}

/*
*
* Receiver Type 0
*
* expect to receive the requested size
*
*/
static ftRet_t rxTypeZero (const spDatDat_t *spdp,bRateMsmnt_t *lbrmp)
{
  int32     reTry=0;
  u_int32   i;
  ssize_t   bCnt;
  const char   *cp;
  ftRet_t ret = ftFail;
  fd_set  rfds;
  int32     retval;
  struct timeval tv;

  FD_ZERO(&rfds);
  FD_SET(spdp->fd_r, &rfds);

eagain0:
  bCnt = -1;
  calculateTimeout (spdp, &tv);
  retval = select (spdp->fd_r + 1, &rfds,NULL, NULL, &tv);

  if (retval == 0)
  {
    // Select timed out
    fitPrint(VERBOSE, "%s timed out reading from %s, fd %ld, sz %4.4u, baud %lu\n",
             spdp->testName,spdp->devName_r,spdp->fd_r,spdp->comSz,
             mapBaudRate(spdp->lbrp->baudRate));
    spdp->lbrp->rxNG++;
    ret = ftUpdateTestStatus(ftrp,ftRxTimeout,NULL);
  }
  else if ((retval == -1) || ((bCnt = read (spdp->fd_r,spdp->buf_r,spdp->comSz)) == -1)) {
    if (EAGAIN == errno) {
      if (++reTry > RETRY_CNT) {
        fitPrint(VERBOSE, "%s nothing to read, err %d\n",spdp->testName,errno);
      } else {
        goto eagain0;
      }
    }
    fitPrint(VERBOSE, "%s cannot read from %s, err %d, %s, fd %ld, sz %4.4u, baud %lu\n",
             spdp->testName,spdp->devName_r,errno,strerror(errno),spdp->fd_r,spdp->comSz,
             mapBaudRate(spdp->lbrp->baudRate));
    spdp->lbrp->rxNG++;
    ret = ftUpdateTestStatus(ftrp,ftRxError,NULL);
  } else {
    if ((size_t)bCnt != spdp->comSz) {
      fitPrint(VERBOSE, "%s %s returned read byte count %d, expected %u, baud %lu\n",
               spdp->testName,spdp->devName_r,bCnt,spdp->comSz,
               mapBaudRate(spdp->lbrp->baudRate));
      spdp->lbrp->rxNG++;
      ret = ftUpdateTestStatus(ftrp,ftRxFail,NULL);
    } else {
      /*
      * Received a frame, check payload
      */

      clock_gettime(CLOCK_MONOTONIC ,&lbrmp->end); // end timing this frame

      if (memcmp (spdp->buf_r,spdp->buf_w,spdp->comSz) == 0) {
        /*
        * Frame has been successfully transmitted and received
        */
        spdp->lbrp->rxOK++;
        ret = ftUpdateTestStatus(ftrp,ftPass,NULL);
      } else {
        spdp->lbrp->rxNG++;
        ret = ftUpdateTestStatus(ftrp,ftRxFail,NULL);
        fitPrint(VERBOSE, "\n%s test failed for %s rx, %s tx size %u, baud %lu\n",
                 spdp->testName,spdp->devName_r,spdp->devName_w,spdp->comSz,
                 mapBaudRate(spdp->lbrp->baudRate));
        fitPrint(VERBOSE, "expected buffer (%p):",spdp->buf_w);
        cp = spdp->buf_w;
        for (i=0;i < spdp->comSz;i++) {
          if ((i%32u) == 0u) { // format output
            fitPrint(VERBOSE, "\n");
          }
          fitPrint(VERBOSE, "%2.2x",*cp++);
        }
        fitPrint(VERBOSE, "\nactual buffer   (%p):",spdp->buf_r);
        cp = spdp->buf_r;
        for (i=0;i<spdp->comSz;i++) {
          if ((i%32u) == 0u) { // format output
            fitPrint(VERBOSE, "\n");
          }
          fitPrint(VERBOSE, "%2.2x",*cp++);
        }
        fitPrint(VERBOSE, "\n\n");
      }
    }
  }

  return (ret);
}

/*
*
* Receiver Type 1
*
* receive packet one byte at a time
*
*/

static ftRet_t rxTypeOne (const spDatDat_t *spdp,bRateMsmnt_t *lbrmp)
{
  int32 reTry=0, bCnt;
  u_int i,j;
  char *cp;
  ftRet_t ret = ftFail;
  fd_set rfds;
  struct timeval tv;
  int32 retval;

  FD_ZERO(&rfds);
  FD_SET(spdp->fd_r, &rfds);

  cp = spdp->buf_r;

  for (i = 0u;i < spdp->comSz;i++)
  {
eagain1:
    bCnt = -1;
    calculateTimeout (spdp, &tv);
    retval = select (spdp->fd_r + 1, &rfds,NULL, NULL, &tv);

    if (retval == 0)
    {
      // Select timed out
      fitPrint(VERBOSE, "%s timed out reading from %s, fd %ld, sz %4.4u, baud %lu\n",
               spdp->testName,spdp->devName_r,spdp->fd_r,spdp->comSz,
               mapBaudRate(spdp->lbrp->baudRate));
      spdp->lbrp->rxNG++;
      ret = ftUpdateTestStatus(ftrp,ftRxTimeout,NULL);
    }
    else if ((retval == -1) || ((bCnt = read (spdp->fd_r,cp,ONE)) == -1))
    {
      if (EAGAIN == errno)
      {
        if (++reTry > RETRY_CNT)
        {
          fitPrint(VERBOSE, "%s nothing to read, err %d\n",spdp->testName,errno);
          reTry = 0;
        }
        else
        {
          goto eagain1;
        }
      }
      fitPrint(VERBOSE, "%s cannot read from %s, err %d, %s, fd %ld, sz %4.4u, baud %lu\n",
               spdp->testName,spdp->devName_r,errno,strerror(errno),spdp->fd_r,spdp->comSz,
               mapBaudRate(spdp->lbrp->baudRate));
      spdp->lbrp->rxNG++;
      ret = ftUpdateTestStatus(ftrp,ftRxError,NULL);
    }
    else // retval > 0
    {
      if (bCnt != 1)
      {
        fitPrint(VERBOSE, "%s %s returned read byte count %ld, expected %u, baud %lu\n",
                 spdp->testName,spdp->devName_r,bCnt,spdp->comSz,
                 mapBaudRate(spdp->lbrp->baudRate));
        spdp->lbrp->rxNG++;
        ret = ftUpdateTestStatus(ftrp,ftRxFail,NULL);
      }
      else
      {
        /*
        * Received a frame, check payload
        */
        clock_gettime(CLOCK_MONOTONIC,&lbrmp->end); // end timing this frame
        if (memcmp (spdp->buf_r,spdp->buf_w,spdp->comSz) == 0)\
        {
          /*
          * Frame has been successfully transmitted and received
          */
          spdp->lbrp->rxOK++;
          ret = ftUpdateTestStatus(ftrp,ftPass,NULL);
        }
        else
        {
          spdp->lbrp->rxNG++;
          ret = ftUpdateTestStatus(ftrp,ftRxFail,NULL);
          fitPrint(VERBOSE, "\n%s test failed for %s rx, %s tx size %u, baud %lu\n",
                   spdp->testName,spdp->devName_r,spdp->devName_w,spdp->comSz,
                   mapBaudRate(spdp->lbrp->baudRate));
          fitPrint(VERBOSE, "expected buffer (%p):",spdp->buf_w);
          cp = spdp->buf_w;
          for (j=0u;j < spdp->comSz;j++)
          {
            if ((j%32u) == 0u)
            { // format output
              fitPrint(VERBOSE, "\n");
            }
            fitPrint(VERBOSE, "%2.2x",*cp++);
          }
          fitPrint(VERBOSE, "\nactual buffer   (%p):",spdp->buf_r);
          cp = spdp->buf_r;
          for (j=0;j<spdp->comSz;j++)
          {
            if ((j%32u) == 0u)
            { // format output
              fitPrint(VERBOSE, "\n");
            }
            fitPrint(VERBOSE, "%2.2x",*cp++);
          }
          fitPrint(VERBOSE, "\n\n");
        }
      }
    } // else retval != 0

    cp++;
  } // for(cp...)

  return (ret);
}

/*
*
* Receiver Type 2
*
* packet may be assembled from fragmented frames
*
*/
static ftRet_t rxTypeTwo (const spDatDat_t *spdp,bRateMsmnt_t *lbrmp)
{
  int32 reTry=0, bCnt, i;
  char *cp = spdp->buf_r;
  ftRet_t ret = ftFail;
  fd_set rfds;
  struct timeval tv;
  int32 retval;

  FD_ZERO(&rfds);
  FD_SET(spdp->fd_r, &rfds);
  i=(int32)spdp->comSz;

eagain2:
  bCnt = -1;
  calculateTimeout (spdp, &tv);
  retval = select (spdp->fd_r + 1, &rfds, NULL, NULL, &tv);

  if (retval == 0)
  {
    // Select timed out
    fitPrint(VERBOSE, "%s timed out reading from %s, fd %ld, sz %4.4u, baud %lu\n",
             spdp->testName,spdp->devName_r,spdp->fd_r,spdp->comSz,
             mapBaudRate(spdp->lbrp->baudRate));
    spdp->lbrp->rxNG++;
    ret = ftUpdateTestStatus(ftrp,ftRxTimeout,NULL);
  }
  else if ((retval == -1) || ((bCnt = read (spdp->fd_r,cp,(size_t)i)) == -1))
  {
    if (EAGAIN == errno) {
      if (++reTry > RETRY_CNT) {
        fitPrint(VERBOSE, "%s nothing to read, err %d\n",spdp->testName,errno);
      } else {
        goto eagain2;
      }
    }
    fitPrint(VERBOSE, "%s cannot read from %s, err %d, %s, fd %ld, sz %4.4u, baud %lu\n",
             spdp->testName,spdp->devName_r,errno,strerror(errno),spdp->fd_r,spdp->comSz,
             mapBaudRate(spdp->lbrp->baudRate));
    spdp->lbrp->rxNG++;
    ret = ftUpdateTestStatus(ftrp,ftRxError,NULL);
  }
  else
  {
    if (bCnt != i)
    {
      if (rxType != 2u)
      {
        fitPrint(VERBOSE, "%s %s returned read byte count %ld, expected %ld, baud %lu\n",
                 spdp->testName,spdp->devName_r,bCnt,i,mapBaudRate(spdp->lbrp->baudRate));
      }

      /*
      * Check for partial successful read. Update request and pointers.
      */
      if ((bCnt < i) && (bCnt != 0))
      {
        i -= bCnt;
        cp = &cp[bCnt];
        reTry = 0;
        goto eagain2; // try to read the rest of the buffer
      }
      else
      {
        spdp->lbrp->rxNG++;
        ret = ftUpdateTestStatus(ftrp,ftRxFail,NULL);
      }
    } //if (bCnt != i)
    else  //bCnt == i
    {
      /*
      * Received a frame, check payload
      */

      clock_gettime(CLOCK_MONOTONIC, &lbrmp->end);   // end timing this frame

      if (memcmp (spdp->buf_r,spdp->buf_w,spdp->comSz) == 0)
      {
        /*
        * Frame has been successfully transmitted and received
        */
        spdp->lbrp->rxOK++;
        ret = ftUpdateTestStatus(ftrp,ftPass,NULL);
      } else {
        spdp->lbrp->rxNG++;
        ret = ftUpdateTestStatus(ftrp,ftRxFail,NULL);
        fitPrint(VERBOSE, "\n%s test failed for %s rx, %s tx size %u, baud %lu\n",
                 spdp->testName,spdp->devName_r,spdp->devName_w,spdp->comSz,
                 mapBaudRate(spdp->lbrp->baudRate));

        fitPrint(VERBOSE, "expected buffer (%p):",spdp->buf_w);
#if 0
        cp = spdp->buf_w;
        for (i=0;i < spdp->comSz;i++) {
          if (i%32 == 0) { // format output
            fitPrint(VERBOSE, "\n");
          }
          fitPrint(VERBOSE, "%2.2x",*cp++);
        }
#else
        mdmp(spdp->buf_w,spdp->comSz,VERBOSE);
#endif
        fitPrint(VERBOSE, "\nactual buffer   (%p):",spdp->buf_r);
#if 0
        cp = spdp->buf_r;
        for (i=0;i<spdp->comSz;i++) {
          if (i%32 == 0) { // format output
            fitPrint(VERBOSE, "\n");
          }
          fitPrint(VERBOSE, "%2.2x",*cp++);
        }
#else
        mdmp(spdp->buf_r,spdp->comSz,VERBOSE);
#endif
        fitPrint(VERBOSE, "\n\n");
      }
    }  //bCnt == i
  }

  return (ret);
}

/*
*
* Get concurrent test
*
*/

static const spDatDat_t *getConcurrentTest(const spDatDat_t *spdp)
{
  const spDatDat_t *lspdp;

  for (lspdp = spDatDat; lspdp->testName != NULL; lspdp++) {
    if ((lspdp->spSt_r == unscheduled) || (lspdp->spSt_w == unscheduled)) {
      continue;
    }

    if (lspdp == spdp) {
      continue;
    }

    //lint -e{9007} strcmp has no side effects
    if ( (strcmp (lspdp->devName_r,spdp->devName_w) == 0) &&
         (strcmp (lspdp->devName_w,spdp->devName_r) == 0)    ) {
      /*
      * found a running concurrent test
      */
      return (lspdp);
    }
  }

  //return ((spDatDat_t *)0);
  return (NULL);
}

/*
*
* Serial Port External Loopback test
*
*/

static ftRet_t spExtLpback(spDatDat_t *spdp,xfer_t xf)
{
  int32   reTry=0;
  int32   result;
  ssize_t bCnt;
  size_t  comSize, iterCnt;
  char    specialDevice[20]; // must be large enough
  const spDatDat_t   *lspdp;       // local serial port data pointer
  const spDatDat_t   *spdp_c;
  bRateMsmnt_t *lbrmp;       // local baud rate measurements pointer
  ftRet_t ret = ftFail;
  spState_t spTmp;

  switch (xf) {
    case receiver:
      /*
      * The receiving case allocates resources, manages the
      * test and outputs results.
      */

      /*
      * Construct RX special device name
      */
      specialDevice[0] = '\0';
      strcat (specialDevice,"/dev/");
      strcat (specialDevice,spdp->devName_r);

      /*
      * Open RX special device
      */
      spdp->fd_r = open (specialDevice,rxFlags);

      if (spdp->fd_r == -1) {
        if (strcmp (spdp->devName_r,spdp->devName_w) == 0) {
          fitPrint(ERROR, "%s test cannot open %s for read, err %d, %s\n",
                   spdp->testName,specialDevice,errno,strerror(errno));
          ret = ftUpdateTestStatus(ftrp,ftRxError,NULL);
          return (ret);
        }

        /*
        * RX and TX ports are different and may have been opened previously for TX.
        */
        for (lspdp = spDatDat; lspdp->testName != NULL; lspdp++) {
          if ((lspdp->spSt_r == unscheduled) || (lspdp->spSt_w == unscheduled)) {
            continue;
          }

          if (lspdp == spdp) {
            continue;
          }

          if (strcmp (spdp->devName_r,lspdp->devName_w) == 0) {
            if (lspdp->fd_w > 0) {
              /*
              * Already opened for previous transmit open.
              */
              spdp->fd_r = lspdp->fd_w;
              break;
            }
          }
        }

        if (spdp->fd_r == -1) {
          fitPrint(ERROR, "%s test cannot find previous open %s for read, err %d, %s\n",
                   spdp->testName,specialDevice,errno,strerror(errno));
          return (ret);
        }
      }

      /*
      * Receiver is doing some work for the transmitter.
      * The test will support loopback to the same device
      * or between two devices.
      */
      if (strcmp (spdp->devName_r,spdp->devName_w) != 0) {
        /*
        * Construct TX special device name
        */
        specialDevice[0] = '\0';
        strcat (specialDevice,"/dev/");
        strcat (specialDevice,spdp->devName_w);

        /*
        * Open TX special device
        */
        spdp->fd_w = open (specialDevice,txFlags);

        if (spdp->fd_w == -1) {
          /*
          * RX and TX ports are different and may have been opened previously for RX.
          */
          for (lspdp = spDatDat; lspdp->testName != NULL; lspdp++) {
            if ((spdp->spSt_r == unscheduled) || (spdp->spSt_w == unscheduled)) {
              continue;
            }

            if (lspdp == spdp) {
              continue;
            }

            if (strcmp (spdp->devName_w,lspdp->devName_r) == 0) {
              if (lspdp->fd_r > 0) {
                /*
                * Already opened for previous receive open.
                */
                spdp->fd_w = lspdp->fd_r;
                break;
              }
            }
          }

          if (spdp->fd_w == -1) {
            fitPrint(ERROR, "%s test cannot find previous open %s for write, err %d, %s\n",
            spdp->testName,specialDevice,errno,strerror(errno));
            ret = ftUpdateTestStatus(ftrp,ftTxError,NULL);
            return (ret);
          }
        }
      } else {
        spdp->fd_w = spdp->fd_r; // single port loopback, share the device descriptor
      }

  #if 0
      if (showParallelPorts) {
        get_pports ();
      }
  #endif

      /*
      *
      * Loop through baud rates
      *
      */

      for (spdp->lbrp = spdp->brp;spdp->lbrp->baudRate != 0u;spdp->lbrp++) {
        // command line baud rate overide
        if (baudRateOveride != 0u) {
          if (baudRateOveride != mapBaudRate(spdp->lbrp->baudRate)) {
            continue;
          }
        }

        switch (spdp->protocol) {
          case protAsync:
            (void) fcntl (spdp->fd_r,F_SETFL,0/*FNDELAY*/); // useless?
            break;
          case protSync:
            /*
            * Enforce baud rate rules.
            */
            if (strcmp (spdp->devName_r,spdp->devName_w) != 0)
            {
              //lint -e{9007} strcmp has no side effects
              if ((strcmp (spdp->devName_r,"sp3s") == 0) ||
                  (strcmp (spdp->devName_r,"sp5s") == 0) ||
                  (strcmp (spdp->devName_w,"sp3s") == 0) ||
                  (strcmp (spdp->devName_w,"sp5s") == 0))
              {
                /*
                * SP3S and SP5S only support two high data rates
                * that are shared. The slowest can also be shared
                * with the fastest rate on the other synchronous ports.
                */
                //lint -e{9007} strcmp has no side effects
                if (!(((strcmp (spdp->devName_r,"sp3s") == 0) &&
                       (strcmp (spdp->devName_w,"sp5s") == 0))||
                      ((strcmp (spdp->devName_r,"sp5s") == 0) &&
                       (strcmp (spdp->devName_w,"sp3s") == 0))))
                {
                  /*
                  * The remaining serial ports share a single rate.
                  */
                  if (spdp->lbrp->baudRate != 153600u)
                  {
                    continue;
                  }
                }
              }
            }
            break;
          case protNone:
          default:
            // nothing to do
            break;
        }

        /*
        *
        * Initialize the ports under test
        *
        */

        if (spdp->fd_r != spdp->fd_w) {
          /*
          *
          * Reader and writer are different physical ports.
          *
          * Check for concurrent test scheduled for the same physical ports
          * in the opposite direction. If there are two tests running on the
          * same cable, check the lock count and wait if zero or post and go.
          *
          * This is intended to coordinate baud rate changes between two tests.
          *
          */

          spdp_c = getConcurrentTest (spdp);

          if (spdp_c != NULL) {
            /* there is a concurrent test scheduled */
            if (spdp_c->spSt_r == waitingForConcurrent) {
              /*
              * NOTE: If it is possible to run different protocol in each direction,
              *       this is specifically not supported here.
              */
              initSerialPort (spdp->fd_r, spdp->lbrp->baudRate, spdp->protocol, enableFlowControl);
              initSerialPort (spdp->fd_w, spdp->lbrp->baudRate, spdp->protocol, enableFlowControl);

              result = sem_post (spdp_c->spLock); // wakeup concurrent test
              if (result != 0) {
                return (ftUpdateTestStatus(ftrp,ftError,NULL));
              }
            } else {
              /* concurrent test is not waiting, block this thread */
              spTmp = spdp->spSt_r;
              spdp->spSt_r = waitingForConcurrent;
              sem_wait (spdp->spLock); // wait for concurrent test
              spdp->spSt_r = spTmp;
            }
          } else {
            /* there is no concurrent test scheduled */
            initSerialPort (spdp->fd_r, spdp->lbrp->baudRate, spdp->protocol, enableFlowControl);
            initSerialPort (spdp->fd_w, spdp->lbrp->baudRate, spdp->protocol, enableFlowControl);
          }
        } else {
          /*
          *
          * Reader and writer are the same physical port.
          *
          */
          initSerialPort (spdp->fd_r, spdp->lbrp->baudRate, spdp->protocol, enableFlowControl);
        }

        /*
        *
        * Loop through requested iterations
        *
        */
        for (iterCnt = 0;iterCnt < spdp->iter;iterCnt++)
        {
          /*
          *
          * Loop through increasing framesizes
          *
          */
          lbrmp = spdp->lbrp->brmp;

          for (comSize = spdp->minFrameSize;comSize < (spdp->maxFrameSize + 1u);
               comSize++)
          {
            spdp->comSz = comSize;

            memset (spdp->buf_r,0,spdp->comSz);           // initialize receive buffer to zeros
            memset (spdp->buf_w,(plint)spdp->comSz,spdp->comSz); // iniz xsmit buf to packet size

            //overlay ATC test string on first bytes of message
            if (useAtcTestString)
            {
              memcpy (spdp->buf_w,atcTestString,MIN(spdp->comSz,sizeof(atcTestString)-1u));
            }

            for (reTry = 0; (spdp->spSt_w != waitingForRx) && (reTry < MAX_WAIT_FOR_TX);
                 reTry++) // wait for transmitter ready state
            {
              usleep (10000); // Sleeps for a hundredth of a second
            }

            if (reTry == MAX_WAIT_FOR_TX) {
              fitPrint(ERROR, "receiver %s timeout transmitter %s, fd %ld, sz %4.4u, " \
                       "baud %lu\n",spdp->devName_r,spdp->devName_w,spdp->fd_w,spdp->comSz,
                       mapBaudRate(spdp->lbrp->baudRate));
              return (ftUpdateTestStatus(ftrp,ftTxTimeout,NULL));
            }

            spdp->spSt_r = receiving;
            clock_gettime (CLOCK_MONOTONIC ,&lbrmp->start); // start timing this frame
            sem_post (spdp->mp_w); // start transmitter UP semaphore

            /*
            *
            * Receive data
            *
            */
            switch (rxType) {
              case 0u:
                ret  = rxTypeZero (spdp,lbrmp);
                break;
              case 1u:
                ret  = rxTypeOne (spdp,lbrmp);
                break;
              case 2u:
                ret  = rxTypeTwo (spdp,lbrmp);
                break;
              case 3u:
                /*
                * Transmit only test
                */
                spdp->comSz = spdp->maxFrameSize;
                memset (spdp->buf_w,0x55,spdp->maxFrameSize); // initialize transmit buffer
                break;
              default:
                ret = ftError;
                break;
            } // switch(rxType)

            if ((ret != ftPass) && quickFail)
            {
              break;
            }

            lbrmp++;
          } // for (comSize...)

          if ((ret != ftPass) && quickFail)
          {
            break;
          }
        } // for (iterCnt...)

        if ((ret != ftPass) && quickFail)
        {
          break;
        }
      }

      spdp->spSt_r = receiveComplete;

  #if 0
      close (spdp->fd_r); // close the receiver port
      close (spdp->fd_w); // close the transmitter port
  #endif

      //fitPrint(VERBOSE, "%s test receive complete for %s\n",spdp->testName,spdp->devName_r);
      break;

    case transmitter:

      spdp->spSt_w = transmitting;

      /*
      * Transmit data
      */
      bCnt = write(spdp->fd_w,spdp->buf_w,spdp->comSz);

      if(bCnt == -1) {
        fitPrint(ERROR, "%s cannot write from %s, err %d, %s, fd %ld, sz %4.4u\n",
                 spdp->testName,spdp->devName_w,errno,strerror(errno),spdp->fd_w,spdp->comSz);
        spdp->lbrp->txNG++;
        ret = ftUpdateTestStatus(ftrp,ftTxError,NULL);
      } else {
        if ((size_t)bCnt != spdp->comSz) {
          fitPrint(ERROR, "%s info, %s returned write byte count %4.4d, expected %4.4u, " \
                   "baud %lu\n",spdp->testName,spdp->devName_w,bCnt,spdp->comSz,
                   mapBaudRate(spdp->lbrp->baudRate));
          if (rxType != 2u) {
            spdp->lbrp->txNG++; // Frame did not transmit properly.
            ret = ftUpdateTestStatus(ftrp,ftTxFail,NULL);
          } else {
            // Frame has been successfully transmitted.
            // A type two receiver can put the correct frame together.
            spdp->lbrp->txOK++;
            ret = ftUpdateTestStatus(ftrp,ftPass,NULL);
          }
        } else {
          // Frame has been successfully transmitted.
          spdp->lbrp->txOK++;
          ret = ftUpdateTestStatus(ftrp,ftPass,NULL);
        }
      }

      spdp->spSt_w = transmitComplete;
      break;

    default:
      fitPrint(ERROR, "%s error, unknown transfer mode %d\n",spdp->testName,xf);
      ret = ftUpdateTestStatus(ftrp,ftError,NULL);
      break;
  } // switch (xf)

  return (ret);
}

/*
* Generic handler for serial port read operations
*/
static void* serialPort_r (void *vp)
{
  ftRet_t     ret;
  spDatDat_t *spdp;

  if (vp != NULL)
  {
    spdp = vp;
  }
  else
  {
    return NULL;
  }

  while (true)
  {
    spdp->spSt_r = waitingForMon;
    sem_wait (spdp->mp_r);
    ret = spExtLpback (spdp,receiver); // run the test
    sem_post (spdp->mp); // wake up the test monitor
  }
  // never actually gets here
}

/*
* Generic handler for serial port write operations
*/
static void* serialPort_w (void *vp)
{
  spDatDat_t *spdp;
  ftRet_t     ret;

  if (vp != NULL)
  {
    spdp = vp;
  }
  else
  {
    return NULL;
  }

  while (true) // should ret be checked?
  {
    spdp->spSt_w = waitingForRx;
    sem_wait (spdp->mp_w);
    spdp->spSt_w = scheduled;
    ret = spExtLpback (spdp,transmitter);
  }
  // never actually gets here
}

/*
* Initialized serial port thread
*/
static int32 initSpDat(spDatDat_t *spdp)
{
  int32 retVal;

  for (spdp->lbrp = spdp->brp;spdp->lbrp->baudRate != 0u;spdp->lbrp++) {
    spdp->lbrp->brmp = malloc (spdp->maxFrameSize * sizeof(bRateMsmnt_t));
  }

  sem_init (spdp->mp,0,0);     // binary semaphore for this serial port monitor
  sem_init (spdp->mp_r,0,0);   // binary semaphore for this serial port read thread
  sem_init (spdp->mp_w,0,0);   // binary semaphore for this serial port write thread
  sem_init (spdp->spLock,0,0); // binary semaphore for this serial port concurrent tests

  retVal = pthread_create (spdp->ptp_r,NULL,serialPort_r,spdp);
  if (retVal != 0) {
    fitPrint(ERROR, "can't pthread_create thread_id_r for %s, errno 0x%lx\n",
             spdp->devName_r,retVal);
    return (-1);
  }

  retVal = pthread_create (spdp->ptp_w,NULL,serialPort_w,spdp);
  if (retVal != 0) {
    fitPrint(ERROR, "can't pthread_create thread_id_w for %s, errno 0x%lx\n",
             spdp->devName_w,retVal);
    return (-1);
  }

  return (0);
}

/*
* Cancel serial port read/write threads
*/
static int32 cancelSpDat(spDatDat_t *spdp)
{
  int32 retVal;
  void *res;

  for (spdp->lbrp = spdp->brp;spdp->lbrp->baudRate != 0u;spdp->lbrp++) {
    free (spdp->lbrp->brmp);
  }

  sem_destroy (spdp->spLock); // binary semaphore for this serial port concurrent tests
  sem_destroy (spdp->mp_w);   // binary semaphore for this serial port write thread
  sem_destroy (spdp->mp_r);   // binary semaphore for this serial port read thread
  sem_destroy (spdp->mp);     // binary semaphore for this serial port monitor

  retVal = pthread_cancel (*spdp->ptp_w);
  if (retVal != 0) {
    fitPrint (VERBOSE, "%s %s pthread_cancel write, ret %ld\n",
              spdp->testName,spdp->devName_w,retVal);
  }

  retVal = pthread_join (*spdp->ptp_w,&res);
  if (retVal != 0) {
    fitPrint (VERBOSE, "%s %s pthread_join write, ret %ld\n",
              spdp->testName,spdp->devName_w,retVal);
  }

  if (res != PTHREAD_CANCELED) {
    fitPrint(VERBOSE, "%s %s write thread was not cancelled\n",
             spdp->testName,spdp->devName_w);
  }

  // Cancelling read thread
  retVal = pthread_cancel (*spdp->ptp_r);
  if (retVal != 0) {
    fitPrint (VERBOSE, "%s %s pthread_cancel read, ret %ld\n",
              spdp->testName,spdp->devName_r,retVal);
  }

  retVal = pthread_join (*spdp->ptp_r,&res);
  if (retVal != 0) {
    fitPrint (VERBOSE, "%s %s pthread_join read, ret %ld\n",
              spdp->testName,spdp->devName_r,retVal);
  }

  if (res != PTHREAD_CANCELED) {
    fitPrint(VERBOSE, "%s %s read thread was not cancelled\n",
             spdp->testName,spdp->devName_r);
  }

  return (0);
}

static void usage(char * cp)
{
  const char *us =
"\
This software tests the MPC82xx and MPC8309 serial ports. Asynchronous and\n\
synchronous protocols are supported at standard baud rates.\n\
\n\
The test requires a point to point serial connection topology, a minimum and\n\
maximum payload size that defines the range of packet sizes being tested\n\
and the number of test iterations to be run. A configuration\n\
file is scanned to communicate the expected connections. This is an ascii\n\
file in the following format:\n\
\n\
recieve_port transmit_port min_payload_size max_payload_size test_iteration\n\
\n\
A typical file might look like:\n\
\n\
sp2 sp1 1 32 10\n\
sp1 sp2 128 256 4\n\
\n\
The number of lines is arbitrary and lines beginning with # are comments.\n\
Ports are linux special device basenames. The dev dirname is presumed.\n\
Two posix threads are created for each line, one for recieve and one for\n\
transmit. For each baud rate, packets are transmitted starting from a\n\
minimum payload size up to the specified maximum payload size. The payload\n\
data is initialized to the payload size which is checked by the reciever\n\
thread.\n\
\n\
The arguments and functionality are defined as:\n\
\t-f hardware signals for RTS/CTS/DCD are used to implement flow control.\n\
\t-b locks all testing to a single standard baud rate .e.g 38400.\n\
\t-i specifies the number of overall test iterations.\n\
\t-m monitor loop behavior where parameter 0 specifies scrolling status,\n\
\t   1 specifies wait and unblock and 2 specifies wait and block.\n\
\t-q quick fail mode.\n\
\t-c configuration file name\n\
\t-h this help\n\
";
  fitPrint(USER,  "usage: %s [lfb[baud rate]i[iteration]c[config file]h]\n%s\n",cp,us);
  fitLicense();
  exit(0);
}

/*
*
* Read configuration file and mark for scheduled to run
*
*/

static int32 spConfigDat (char *fn)
{
  spDatDat_t *spdp;
  FILE *fp;
  char spDev_r[20],spDev_w[20]; // must be big enough
  size_t minFrameSize,maxFrameSize,iterCnt;
  int32  isReadDevNameEq;
  int32  isWriteDevNameEq;


  fp = fopen (fn,"r");

  if (fp == NULL) {
    fitPrint(ERROR, "cannot open %s\n",fn);
    return (-1);
  }

  while (fscanf (fp,"%s %s %u %u %u",spDev_r,spDev_w,&minFrameSize,&maxFrameSize,&iterCnt) != EOF) {
    if (spDev_r[0] == '#') {
      continue; // this is a comment in the config file
    }

    /*
    * TODO: need configuration integrity and resource checks
    */
    for (spdp = spDatDat; spdp->testName != NULL; spdp++)
    {
      isReadDevNameEq  = strncmp (spdp->devName_r,spDev_r,sizeof(spDev_r));
      isWriteDevNameEq = strncmp (spdp->devName_w,spDev_w,sizeof(spDev_w));

      if ((isReadDevNameEq == 0) && (isWriteDevNameEq == 0))
      {
        spdp->spSt_r = scheduled;
        spdp->spSt_w = scheduled;

        if ((minFrameSize <= MAX_TRANSFER) && (minFrameSize > 0u)) {
          spdp->minFrameSize = minFrameSize;
        } else {
          spdp->minFrameSize = MIN(minFrameSize,MAX_TRANSFER);
          fitPrint(ERROR, "warning: request for minimum frame size %u out of range, " \
                   "using default %u bytes\n",minFrameSize,spdp->minFrameSize);
        }

        if ((maxFrameSize <= MAX_TRANSFER) && (maxFrameSize > 0u)) {
          spdp->maxFrameSize = maxFrameSize;
        } else {
          spdp->maxFrameSize = MIN(maxFrameSize,MAX_TRANSFER);
          fitPrint(ERROR, "warning: request for maximum frame size %u out of range, " \
                   "using default %u bytes\n",maxFrameSize,spdp->maxFrameSize);
        }

        spdp->iter = iterCnt;
      }
    }
  }

  fclose (fp);
  return (0);
}

static void printSerialStats (void)
{
  const spDatDat_t *spdp;
  const bRate_t   *brPtr;
  // Holds the number of lines that were written the last pass through this function

  #ifdef NUMLINES // Change to 1 to enable in-place scrolling, this will overwrite error messages that are written to the screen
  static int32 numLines = 0;
  if (numLines != 0) {
    fitPrint(VERBOSE, "\33[%dA", numLines); // Go up as many lines as you wrote last
  }
  #endif

  fitPrint(VERBOSE, "\n");
  #ifdef NUMLINES
  numLines = 1;
  #endif

  // Print out info for each test running
  for (spdp = spDatDat; spdp->testName != NULL; spdp++) {
    if ((spdp->spSt_r == unscheduled) || (spdp->spSt_w == unscheduled)) {
      continue;
    }

    fitPrint(VERBOSE, "\n%s->%s:\n", spdp->devName_w, spdp->devName_r);
    fitPrint(VERBOSE, "\t%lu iterations from %u to %u bytes\n",
             spdp->iter, spdp->minFrameSize, spdp->maxFrameSize);
    if (useAtcTestString)
    {
      fitPrint(VERBOSE, "\tATC test string used for first %u bytes\n",
               sizeof(atcTestString)-1u);
    }
    else
    {
      fitPrint(VERBOSE, "\tmessage size used for all data bytes\n");
    }

    #ifdef NUMLINES
    numLines++;
    #endif
    // Print out info for each baud rate
    for (brPtr = spdp->brp; brPtr->baudRate != 0u; brPtr++) {
      fitPrint(VERBOSE, "BAUD %6lu: success ratio: RX = %8.8lu/%8.8lu TX = %8.8lu/%8.8lu\n",
               mapBaudRate (brPtr->baudRate), brPtr->rxOK, brPtr->rxNG + brPtr->rxOK,
               brPtr->txOK, brPtr->txOK + brPtr->txNG);
      #ifdef NUMLINES
      numLines++;
      #endif
    }
  }

  fitPrint(VERBOSE, "\n");
  #ifdef NUMLINES
  numLines++;
  #endif
}

/*
*
* Entry function for Serial Port Tests
*
*/

ftRet_t serialFit(plint argc, char * const argv[])
{
  const bRate_t *brPtr;
  spDatDat_t *spdp;
  spDatDat_t spd = {NULL};
  int32 iter = 1; // number of test iterations
  bool  loopAbort = false;
  ftRet_t  ftRet = ftError;

  int32 c, idx, waits = 1;
  char errorStr[MUST_BE_BIG_ENOUGH] = {'\0'};
  char tempStr[MUST_BE_BIG_ENOUGH] = {'\0'};
  const char *flagOpts = "-afqzm:b:hc:i:x:r:t:y:";
  struct timespec ts;
  time_t start_time, end_time;

  //mtrace();

  /*
  *
  * Parse command line arguments
  *
  */
  opterr = 0;
  c = getopt (argc,argv,flagOpts);

  while ((c != -1) && !loopAbort) {
    switch ((char)c) {
#ifdef INTERNAL_LOOPBACK
      case 'l': // enable internal loopback and echo
        set_loopback_echo();
        break;
#endif
      case 'a': // use ATC test string
        useAtcTestString = true;
        break;
      case 'f': // enable flow control
        enableFlowControl = true;
        break;
      case 'q': // fail test quickly
        quickFail = true;
        break;
#ifdef PARALLEL_PORTS
      case 'p': // display parallel port programming
        showParallelPorts = true;
        break;
#endif
      case 'b': // run test at specified rate
        baudRateOveride = strtoul(optarg,NULL,10);
        break;
      case 'm': // specify monitor loop behavior
        idx = strtol(optarg,NULL,10);

        switch (idx)
        {
          case 0:
            monitorLoop = waitAndScroll;
            break;
          case 1:
            monitorLoop = waitAndUnblock;
            break;
          case 2:
          default:
            monitorLoop = waitAndBlock;
            break;
        }
        break;
      case 'c': // configuration file name
        if (spConfigDat (optarg) != 0) {
          fitPrint(ERROR, "%s error: cannot config serial port data %s\n",argv[0],optarg);
          ftRet = ftUpdateTestStatus(ftrp,ftError,NULL);
          loopAbort = true;
        }
        break;
      case 'i': // number of test iterations
        iter = strtol(optarg,NULL,10);
        break;
      case 'r': // receiver open flags
        rxFlags = strtol(optarg,NULL,10);
        break;
      case 't': // transmitter open flags
        txFlags = strtol(optarg,NULL,10);
        break;
      case 'x': // choose receiver software design
        rxType = (u_int8)strtoul(optarg,NULL,10);
        break;
      case 'y': // timeout multiplier
        tmoMultiplier = strtol(optarg,NULL,10);
        break;
        #ifdef REMOTE_CONTROLLER
      case 'z': // relay to interconnected controller
        remoteController = true;
        break;
        #endif
      case 'h':
        usage(argv[0]);
        ftUpdateTestStatus(ftrp,ftError,NULL);
        ftRet = ftUpdateTestStatus(ftrp,ftComplete,NULL);
        loopAbort = true;
        break;
      default:
        if (isprint (optopt)) {
          fitPrint(ERROR, "unknown option `-%c`.\n",optopt);
        } else {
          fitPrint(ERROR, "unknown option character `\\X%X`.\n",optopt);
        }
        usage(argv[0]);
        ftUpdateTestStatus(ftrp,ftError,NULL);
        ftRet = ftUpdateTestStatus(ftrp,ftComplete,NULL);
        loopAbort = true;
        break;
    }

    c = getopt (argc,argv,flagOpts);
  }

  if (loopAbort)
  {
    return ftRet;  // user help request or command line error
  }

  for (idx = optind; idx < argc; idx++) {
    fitPrint(ERROR, "Non-option argument %s\n",argv[idx]);
    usage(argv[0]);
  }

  /*
  *
  * Initialize and start serial port threads
  *
  */

  for (spdp = spDatDat; spdp->testName != NULL; spdp++) {
    if ((spdp->spSt_r == unscheduled) || (spdp->spSt_w == unscheduled)) {
      continue;
    }

    if (initSpDat (spdp) != 0) {
      fitPrint(VERBOSE, "%s not started, %s rx, %s tx\n",spdp->testName,spdp->devName_r,spdp->devName_w);
      return (ftUpdateTestStatus(ftrp,ftError,NULL));
    } else {
      //fitPrint(VERBOSE, "%s started, %s rx, %s tx\n",spdp->testName,spdp->devName_r,spdp->devName_w);
    }
  }

  /*
  *
  * Operational loop
  *
  */

  do {

    /*
    *
    * Start up the tests
    *
    */

    for (spdp = spDatDat; spdp->testName != NULL; spdp++) {
      if ((spdp->spSt_r == unscheduled) || (spdp->spSt_w == unscheduled)) {
        continue;
      }

      sem_post (spdp->mp_r); // enable the receiver thread
    }

    /*
    *
    * Wait for the tests to complete
    *
    */

    for (spdp = spDatDat; spdp->testName != NULL; spdp++) {
      if ((spdp->spSt_r == unscheduled) || (spdp->spSt_w == unscheduled)) {
        // do nothing
      }
      else
      {
        /*
        * select monitor loop behavior
        */
        switch (monitorLoop) {
          case waitAndScroll:
            while (sem_trywait (spdp->mp) != 0)
            {
              c = usleep (10000); // Sleeps for a hundredth of a second
              if ((waits % 1000) == 0) { // ten seconds
                printSerialStats (); // Print out stats
                waits = 1;
              }
              waits++;
            }
            break;
          case waitAndUnblock:
            /*
            *
            * This optional block allows the monitor to unblock while the tests are running.
            * For now, try to determine whether the test is making progress.
            * The 'stuck' test conditions are apparently all fixed, but this may prove useful
            * when bringing in new drivers.
            *
            */
            while (true) {
              clock_gettime(CLOCK_REALTIME,&ts);
              ts.tv_sec += 2;

              time(&start_time);
              c = sem_timedwait(spdp->mp,&ts); // monitor semaphore for this test
              time(&end_time);

              if (c == -1) {
                if (errno == ETIMEDOUT) {
                  //fitPrint(VERBOSE,"sem_timedwait timed out %s after %d seconds\n",spdp->devName_r,end_time - start_time);
                } else {
                   perror("sem_timedwait");
                }
              } else {
                //fitPrint(VERBOSE,"sem_timedwait succeeded for receiver %s\n",spdp->devName_r);
                break;
              }

              /*
              * check if receiver thread is running
              */
              if (spdp->spSt_r == waitingForMon) {
                //fitPrint(VERBOSE,"Monitor found receiver %s blocked\n",spdp->devName_r);
                break;
              } else {
                /* check that this thread is making progress */
                if (memcmp (spdp,&spd,sizeof(spDatDat_t)) == 0) {
                  //fitPrint(VERBOSE,"Monitor found receiver %s stuck\n",spdp->devName_r);
                  break; /* no change -> no progress */
                } else {
                  memcpy (&spd,spdp,sizeof(spDatDat_t));
                  //fitPrint(VERBOSE,"Monitor found receiver %s running\n",spdp->devName_r);
                  continue;
                }
              }
            }

            //fitPrint(VERBOSE,"Monitor leaving %s\n",spdp->devName_r);
            break;
          case waitAndBlock:
          /*lint -fallthrough */
          default:
            //fitPrint(VERBOSE, "Monitor waiting for post from %-4s, sval %d, twait %d\n",spdp->devName_r,sval,abs(c));
            sem_wait (spdp->mp);
            //fitPrint(VERBOSE, "Monitor received post from %-4s\n",spdp->devName_r);
            break;
        } // switch()
      } //*else
    } // for()

    --iter;
  } while (keepGoing && (iter > 0));

  printSerialStats ();

  /*
  * Cancel serial port read/write threads
  */
  for (spdp = spDatDat; spdp->testName != NULL; spdp++) {
    if ((spdp->spSt_r == unscheduled) || (spdp->spSt_w == unscheduled)) {
      continue;
    }

#if 0 // Don't want this print out for final product
    {
      double t_ns = 0;
      bRateMsmnt_t *lbrmp; // local baud rate measurements pointer
      /*
      * Process frame timing data
      */
      //fitPrint(VERBOSE, "processing frame timing data\n");
      for (spdp->lbrp = spdp->brp;spdp->lbrp->baudRate != 0;spdp->lbrp++) {
        for (lbrmp = spdp->lbrp->brmp,iter = 0;iter < (spdp->maxFrameSize - spdp->minFrameSize);iter++,lbrmp++) {
          lbrmp->end.tv_sec -= lbrmp->start.tv_sec;
          lbrmp->end.tv_nsec -= lbrmp->start.tv_nsec;

          if (lbrmp->end.tv_nsec < 0) {
            lbrmp->end.tv_sec--;
            lbrmp->end.tv_nsec += 1000000000;
          }

          t_ns = (double)(lbrmp->end.tv_sec *1.0e9 + lbrmp->end.tv_nsec);
        }
        fitPrint(VERBOSE, "BAUD %6.6d %-4s FRAME SIZE %4.4d NS %14.2f\n",mapBaudRate(spdp->lbrp->baudRate),spdp->devName_r,spdp->maxFrameSize,t_ns);
      }
    }
#endif

    //fitPrint(VERBOSE, "monitor is cancelling threads...\n");
    if (cancelSpDat (spdp) != 0) {
      fitPrint(VERBOSE, "%s not canceled\n",spdp->testName);
    } else {
      //fitPrint(VERBOSE, "%s cancelled for %s rx, %s tx\n",spdp->testName,spdp->devName_r,spdp->devName_w);
    }

#if 1
    // Check for failures and add them to errorStr
    for (brPtr = &spdp->brp[0]; brPtr->baudRate != 0u; brPtr++) {
      if ((brPtr->rxNG != 0u) || (brPtr->txNG != 0u)) {
        sprintf (tempStr, "%s->%s@%lu, ", spdp->devName_w, spdp->devName_r, mapBaudRate (brPtr->baudRate));

        // Check and make sure there is room in the array. This test has a user-specified number of configurations, so it could fill up our buffer.
        // In which case, just stop adding them to this list.
        //lint -e{9007,931} strlen has no side effects
        if ((strlen(errorStr) + strlen(tempStr)) < sizeof (errorStr)) {
          strcat (errorStr, tempStr);
        } else {
          fitPrint(VERBOSE, "%s: error string is full\n",__func__);
          break;
        }
      }
    }
  }
#else
        sprintf (tempStr, "%s->%s@%d, ", spdp->devName_w, spdp->devName_r, mapBaudRate (brPtr->baudRate));
#endif

  return(ftUpdateTestStatus(ftrp,ftComplete, errorStr));
}
