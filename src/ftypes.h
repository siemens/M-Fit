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

#ifndef FTYPES_H
  #define FTYPES_H

  // global type definitions
  #if (__PPC__ == 1) && (__linux__ == 1)
    // type definitions for PowerPC 603e core
    typedef signed char     int8;
    typedef signed short    int16;
    typedef signed int      plint;   // plain int of indefinite size
    typedef signed long     int32;
    typedef unsigned char   u_int8;
    typedef unsigned short  u_int16;
    typedef unsigned int    u_plint; // plain unsigned of indefinite size
    typedef unsigned long   u_int32;
    typedef float           float32;
    typedef double          float64;
  #else
    #error Unsupported CPU or OS
  #endif

#endif // FTYPES_H
