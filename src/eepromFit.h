/******************************************************************************
                                 eepromFit.h

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

/*******************************************************************
 *
 *    DESCRIPTION:
 *      This is a pseudo-struct for reading and writing ATC 5.2b
 *      Host Board EEPROM information as used in Siemens ATC, and
 *      2070LX controllers.  This struct should NOT be used to
 *      attempt to read EEPROM data from other vendors; it repre-
 *      sents only one of many valid EEPROM layouts.
 *
 *    AUTHORS:
 *      Jonathan Grant
 *      Jack McCarthy
 *
 *    NOTES:
 *      Text in [brackets] is not from the standard and represents
 *      Siemens interpretations of, or extensions to, the fields.
 *      ATC 5.2b defines the image as being variable in size:  it
 *      potentially contains a variable number of hardware modules,
 *      Ethernet devices, and I/O ports.  There is a value preceed-
 *      ing each of these three groups which indicates how many
 *      items are present.  Unfortunately the text strings in the
 *      module descriptions are of indefinite length.  This makes
 *      parsing of the data very cumbersome since each module des-
 *      cription may be of a different size.  There is also the
 *      requirement that the data must fit into a 256 byte EEPROM.
 *
 *      To meet the space limit, the following compromises have
 *      been made:
 *      1. module descriptions are of fixed size; the text string
 *          must always be 8 bytes including the terminating null.
 *      2. the number of modules is fixed at 4; this has the unfor-
 *          tutnate effect of leaving no space to describe any card
 *          which might be installed in slot A1.
 *      3. the number of Ethernet devices is fixed at 2.
 *      4. the number of ports is fixed at 11 even though 12 could
 *          be used in most cases.
 *
 *******************************************************************/

#pragma pack(1)

typedef struct atc_52b_module_version /* 7 bytes */
{
    u_int16 year_bcd;   /* BCD 4 digit year                                   */
    u_int8  mon_bcd;    /* BCD 2 digit month; range 1-12                      */
    u_int8  day_bcd;    /* BCD 2 digit day; range 1-31                        */
    u_int8  major_bcd;  /* BCD 2 digit S/W major version number               */
    u_int8  minor_bcd;  /* BCD 2 digit S/W minor version number               */
    u_int8  patch_bcd;  /* BCD 2 digit S/W patch number                       */
} mod_v;

typedef struct atc_52b_module_desc /* 18 bytes */
{
    u_int8  location; /* Module location: 1=Other, 2=Host, 3=Display
                             4=Power Supply, 5=Slot A1, 6=Slot A2             */
    u_int8  make;     /* NEMA NTCIP Mfg ID                                    */
    char    model[8]; /* Module model string                                  */
    mod_v   version;  /* Module version numbers                               */
    u_int8  type;     /* Module type: 1=Other, 2=Hardware, 3=Soft/Firmware    */
} mod_d;

typedef struct atc_52b_ethernet_dev /* 20 bytes */
{
    u_int8 type;        /* Device type: 1=Other, 2=Hub/Phy/Other Direct Port,
                                3=Unmanaged Switch, 4=Managed Switch,
                                5=Router; default 2                           */
    u_int32 ip_addr;    /* IPv4 address: default 10.20.70.51
                                [use 0.0.0.0 if type = 2]                     */
    u_int8 mac_addr[6]; /* Ethernet MAC add: use 00:00:00:00:00:00 if type=2,
                                default 00:00:00:00:00:00                     */
    u_int32 ip_mask;  /* IPv4 netmask: default 255.255.255.0
                                [use 0.0.0.0 if type = 2]                     */
    u_int32 ip_gate;  /* IPv4 gateway: [use 0.0.0.0 if type = 2],
                                default 10.20.70.254                          */
    u_int8 interface;/* Engine board interface: 1=Other, 2=Phy, 3=RMII,
                                4=MII; default 2                              */
} eth_d;

typedef struct atc_52b_port_bitmap /* 2 bytes */
{   /*lint --e{46} unsigned short bitfields are ok with this compiler */
    u_int16 spares:5;  /* always 0 */
    u_int16 sp8s:1;    /* Bit 10: /dev/sp8s */
    u_int16 sp8:1;     /* Bit  9: /dev/sp8  */
    u_int16 sp6:1;     /* Bit  8: /dev/sp6  */
    u_int16 sp5s:1;    /* Bit  7: /dev/sp5s */
    u_int16 sp4:1;     /* Bit  6: /dev/sp4  */
    u_int16 sp3s:1;    /* Bit  5: /dev/sp3s */
    u_int16 sp3:1;     /* Bit  4: /dev/sp3  */
    u_int16 sp2s:1;    /* Bit  3: /dev/sp2s */
    u_int16 sp2:1;     /* Bit  2: /dev/sp2  */
    u_int16 sp1s:1;    /* Bit  1: /dev/sp1s */
    u_int16 sp1:1;     /* Bit  0: /dev/sp1  */
} portbits;

typedef union _portmap
{
    portbits  port;
    u_int16   ports;
} portmap;


typedef struct atc_52b_port_descr  /* 6 bytes */
{
    u_int8 id;      /* Port identifier: 0=none, 1=sp1, 2=sp2, 3=sp3, 4=sp4,
                                5=sp5, 6=sp6, 7=resvd, 8=sp8, 9=eth0, 10=eth1,
                                11=spi3, 12=spi4, 13=usb, 14-255=resvd        */
    u_int8 mode;    /* Operating mode: 0=other, 1=asynch 2=sync, 3=SDLC
                                4=HDLC, 5-255=resvd                           */
    u_int32 speed;  /* Port baud rate [or max rate] for serial ports;
                                n/a [use 0] for Ethernet, USB, [or SPI].      */
} port_d;

typedef struct atc_52b_eeprom
{
    u_int8  version;    /* EEPROM version #: 1=v5.2, 2=6.10, default 1        */
    u_int16 size;       /* EEPROM size: 0=use default                         */
    u_int8  modules;    /* # of module descriptions  [always 4]               */
    mod_d   module[4];  /* Array of module descriptions              72 bytes */
    u_int8  t_lines;    /* Display lines: 0=no display, default 8             */
    u_int8  t_cols;     /* Display columns: 0=no display, default 40          */
    u_int8  g_rows0;    /* Graphic rows-1: 0=no display, default 63           */
    u_int8  g_cols0;    /* Graphic cols-1: 0=no display, default 239          */
    u_int8  ethdevs;    /* # of Ethernet device descriptions  [always 2]      */
    eth_d   ethdev[2];  /* Array of Ethenet dev descriptions         40 bytes */
    u_int8  spi3use;    /* SPI 3 purpose: 0=unused, 1-255 res, default 0      */
    u_int8  spi4use;    /* SPI 4 purpose: 0=unused, 1-255 mfg. specific       */
    portmap ports_used; /* Host board serial ports used               2 bytes */
    u_int8  ports;      /* # of port descriptions  [always 11]                */
    port_d  port[11];   /* Array of port descriptons                 66 bytes */
    portmap ports_pres; /* Host board serial ports present            2 bytes */
    portmap sb1;        /* Serial Bus 1 port                          2 bytes
                                [for FIO and ITS SB1, default sp5s]           */
    portmap sb2;        /* Serial Bus 2 port                          2 bytes
                                [for ITS SB2, default sp3s]                   */
    portmap ts2;        /* TS 2 PORT1 port [default sp3s]             2 bytes */
    u_int8  expbus;     /* [CPU] expansion bus: 1=none, 2=other, 3=VME;
                                default 1                                     */
    u_int16 crc1;       /* CRC [for EEPROM to this point; presumably
                                per clause 4.6.2 of ISO/IEC 3309, i.e.,
                                same as for Datakey]                          */
    /*    ----------=========== 203 bytes to here ============-------------   */
    float32 latitude;   /* Latitude, default 0.0 or from Datakey              */
    float32 longitude;  /* Longitude, default 0.0 or from Datakey             */
    u_int16 contr_id;   /* Controller ID #: default 65535 or from Datakey     */
    u_int16 com_drop;   /* [Serial] comm drop: default 65535 or Datakey       */
    u_int8  res_agency[35]; /* Reserved for Agency use or from Datakey,
                                default 0xFF for all bytes                    */
    u_int16 crc2;       /* CRC [for Datakey header] or from Datakey           */
    /*    ----------=========== 252 bytes to here ============-------------   */
    u_int8 user_data[4];/* User data to end of device: default 0xFF           */
    /*    ----------=========== 256 bytes to here ============-------------   */
} eeprom_v1;
