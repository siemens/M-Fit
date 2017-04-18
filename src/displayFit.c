/******************************************************************************
                                  displayTest.c

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

/* displayTest.c
 *
 * displayTest is a test application for the ATC 16-line display.
 *
 * With no arguments given, the application displays default special characters
 * and cycles through all printable ASCII chars on the display in standard video,
 * reverse video, and character blink mode. This is the same as the -d option.
 *
 * Other options are as follows.
 * -k Keyboard Test: Allows you to press all keys and have their corresponding
 *    value shown on the display.
 * -a Aux switch Status: will print out on the console the status (ON/OFF) of the
 *    aux switch.
 * -s Screen type: will print out on the console the screen type of the display.
 *    Valid values are 'A' (4-line), 'B' (8-line), and 'D' (16-line).
 * -t Tab Stop Testing: interactive test that requires the user to validate that
 *    tabbing is working as expected. Tests setting and clearing of tab stops on
 *    all fields.
 * -v Prints the version of this application to the console.
 * -z Turns on auto-scroll and auto-wrap. These can be disabled by resetting the
 *    board.
 */

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>

#include "fit.h"
#include "displayLib.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 3

#define REFRESH_MS 250000
#define MAX_SPECIAL_CHARS 8u
#define CHARS_PER_LINE 40u

int32 fpd;  // file descriptor for the display

static const char *displayDev = "/dev/sp6";

static void displayTest(void);
static void printFullCharSet(u_int32 lines);
static void defaultSpecialCharsTest(void);
static void printEndBanner(void);
static void keypadTest(void);
static void reportAuxSwitch(void);
static void initSpecialChars(void);
static void clearAllTabs(void);
static void writeSpecialChars(u_int32 startChar, u_int32 writeNum);
static void writeSpecialChar(u_int32 specialChar);
static void moveCursorUp(int32 count);

static int32 initPort(void);
static int32 getScreenSize(void);
static int32 decodeCharSequence(const char *buffer, char *outbuf, size_t bsize);
static int32 runTabs(void);


static bool useRevVid;
static bool useCharBlink;
static bool showAux;
static bool runDisplayTest;
static bool runTabProg;
static bool runKeypadTest;
static bool tellType;
static bool autoScroll;

static u_int32 num_o_lines;
static char screenType;
static const char *screenName;

static ftResults_t ftResults = {0};
static ftResults_t *ftrp = &ftResults;

/*
*
* Display test entry
*
*/

ftRet_t displayFit (plint argc, char * const argv[] )
{
  bool     getOut = false;
  ftRet_t  retVal = ftError;
  const char *flagOpts = "-adhkstvz";

  // initialize variables
  useRevVid = false;
  useCharBlink = false;
  runDisplayTest = false;
  runTabProg = false;
  num_o_lines = 8;
  tellType = false;
  screenType = 'U';
  screenName = "Unknown-";
  autoScroll = false;
  showAux = false;

  if (argc == 1)
  {
    // if no args given, do the display test
    useRevVid = true;
    useCharBlink = true;
    runDisplayTest = true;
  }
  else
  {
    // parse the options.
    int32 opt = getopt(argc, argv, flagOpts);

    while ((opt != -1) && !getOut)
    {
      switch ((char)opt)
      {
        case 'a':
          showAux = true;
          break;

        case 's':
          //s for screen? tell me the type.
          tellType = true;
          break;

        case 'd':
          runDisplayTest = true;
          useRevVid = true;
          useCharBlink = true;
          break;

        case 't':
          runTabProg = true;
          break;

        case 'k':
          runKeypadTest = true;
          break;

        case 'v':
          fitPrint(USER, "%s v%d.%d\n", argv[0], VERSION_MAJOR, VERSION_MINOR);
          ftUpdateTestStatus(ftrp,ftPass,NULL);
          retVal = ftUpdateTestStatus(ftrp,ftComplete,NULL);
          getOut = true;
          break;

        case 'z':
          autoScroll = true;
          break;

        case 'h':
        default:
          fitPrint(USER, "Usage : %s [%s]\n", argv[0], flagOpts);
          fitPrint(USER, " a - Display aux switch state\n");
          fitPrint(USER, " d - Run display test. Default when no args given.\n");
          fitPrint(USER, " k - Keypad test\n");
          fitPrint(USER, " s - Inquire about screen type\n");
          fitPrint(USER, " t - Tab stop testing\n");
          fitPrint(USER, " v - Display version of application\n");
          fitPrint(USER, " z - Enable auto-scroll and auto-wrap\n");
          fitPrint(USER, " h - This usage help message\n\n");
          fitLicense();
          ftUpdateTestStatus(ftrp,ftError,NULL);
          retVal = ftUpdateTestStatus(ftrp,ftComplete,NULL);
          getOut = true;
          break;
      } // switch(opt)

      opt = getopt(argc, argv, flagOpts);
    } // while(opt != -1)
  }

  if (getOut)
  {
    return retVal;
  }

  if (initPort() < 0)
  {
    // something went bad
    ftUpdateTestStatus(ftrp,ftError,NULL);
    return (ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }

  if (showAux)
  {
    reportAuxSwitch();
  }

  if (autoScroll)
  {
    setAutoScroll(true);
    setAutoWrap(true);
  }

  if (tellType)
  {
    // all they want to know is how many lines.
    fitPrint(USER, "Screen is type %s with %lu lines.\n",
             screenName, num_o_lines);
  }

  if (runKeypadTest)
  {
    keypadTest();
  }

  if (runTabProg)
  {
    (void) runTabs();
  }

  if (runDisplayTest)
  {
    // Test default special chars
    defaultSpecialCharsTest();

    // run the display test
    displayTest();
  }

  clearScreen();
  printEndBanner();
  ftUpdateTestStatus(ftrp,ftPass,NULL);
  return (ftUpdateTestStatus(ftrp,ftComplete,NULL));
}

/* prints out a banner saying the test is over. */
static void printEndBanner(void)
{
  char string[41];
  char centerCommand[20];
  size_t stringSize;

  snprintf(string, sizeof(string), "This concludes the display test.");
  stringSize = strlen(string);

  // center the cursor for printing
  snprintf(centerCommand, sizeof(centerCommand), "\x1b" "[%d;%uf", 3,
           ((CHARS_PER_LINE - stringSize)/2u) + 1u);
  writeOut(centerCommand);

  // finally, print
  writeOut(string);
  //fitPrint(VERBOSE, "Display Test finished successfully.\n");
}

static int32 getScreenSize(void)
{
  u_int32 i;
  int32   retVal;
  int32   retry = 0;
  int32   lErrno;
  bool    loopOn = true;
  char    buffer[50];
  const char *cp;

  memset(buffer, 0, sizeof(buffer));

  writeOut("\x1b" "[c");  // ask for terminal type

  while (loopOn)
  {
    retVal = readTimeout (fpd, buffer, sizeof(buffer), 10);

    if (retVal > 0) // Valid read
    {
      loopOn = false;
    }
    else // Error on read
    {
      lErrno = errno;
      if ((lErrno == ETIMEDOUT) || (lErrno == 0))
      {
        retVal = -1;
        loopOn = false;
      }

      retry++;
      if (retry >= 5)
      {
        retVal = -1;
        loopOn = false;
      }
    }
  }

  if (retVal == -1)
  {
    return retVal;
  }

  // Check first for known non-standard terminals
  if (strncmp(buffer,"\x1b" "[?1;0c", 7u) == 0)
  { // Eagle/Siemens M50 8-line display or honest VT-101
    num_o_lines = 8;
    screenType = 'm';  // Type M is M-50 display
    screenName = "Siemens M50";
  }
  else if (strncmp(buffer, "2070 8-Line",10u) == 0)
  { // Eagle/Siemens old 2070-3B
    num_o_lines = 8;
    screenType = 'b';
    screenName = "Old 2070-3B";
  }
  else if (strncmp(buffer, "\x01" "B[?1;2070_4",12u) == 0)
  { // Eagle/Siemens old 2070-3A
    num_o_lines = 4;
    screenType = 'a';
    screenName = "Old 2070-3A";
  }
  else if (strncmp(buffer,"\x1b" "[?", 3u) == 0)
  { // probably a real terminal
    num_o_lines = 16;
    screenType = 'c';  // Type C is front panel w/o display
    screenName = "2070-3C/TTY";
  }
  else
  {
    // looking for ESC [ x R
    // where x is A, B or D
    cp=buffer;
    for (i=0;i < 4u;i++)
    {
      if (cp[0] == '\x1b')
      {
        if (cp[1] == '[')
        {
          if (cp[3] == 'R')
          {
            switch (cp[2])
            {
              case 'A':
                num_o_lines = 4;
                screenType = 'A';
                screenName = "2070-3A";
                break;
              case 'B':
                num_o_lines = 8;
                screenType = 'B';
                screenName = "2070-3B";
                break;
              case 'D':
                num_o_lines = 16;
                screenType = 'D';
                screenName = "2070-3D";
                break;
              default:
                fitPrint(ERROR, "WARNING!!! Bad response on screen type inquiry: %c\n",
                         cp[2]);
                break;
            }

            if (!tellType)
            {
              fitPrint(VERBOSE, "Screen type: '%c' %s, number of lines: %lu\n",
                        screenType, screenName, num_o_lines);
            }
            break;// found the screen type
          }
          else
          {
            fitPrint(ERROR, "WARNING!!! Unknown screen type response:\n" \
                     "\t0x %02x %02x %02x %02x\n", cp[0],cp[1],cp[2],cp[3]);
          }
        }
      } // if(<esc>) ...

      cp++;
    } // for() ...
  } // if/elseif/else ...

  return 0;
}

static int32 initPort(void)
{
  struct termios terms;

  // open sp6
  fpd = open(displayDev, O_RDWR);

  if (fpd == -1)
  {
    // an error has occurred. bye bye
    fitPrint(ERROR, "Error: could not open sp6.\n");
    return errno;
  }

  // init the display everything
  initDisplay();

  // set for input for nonblocking
  tcgetattr (fpd, &terms);
  terms.c_lflag &= ~((u_int32)ECHO | (u_int32)ICANON | (u_int32)ISIG);
  terms.c_cc[5] = 4;
  terms.c_cc[7] = 1;
  terms.c_oflag |= (u_int32)O_NDELAY;
  tcsetattr (fpd, TCSANOW, &terms);

  if (getScreenSize() < 0)
  {
    fitPrint(ERROR, "No response from display\n");
    return -1;
  }

  return 0;
}

static void reportAuxSwitch(void)
{
  char  buffer[5];
  bool  found;
  int32 i, numRead, lErrno;

  // inquiry about the aux switch
  writeOut("\x1b" "[An");

  found = false;

  for (i = 0; i < 100; i++)
  {
    memset(buffer, 0, sizeof(buffer));

    numRead = readTimeout (fpd, buffer, sizeof(buffer) - 1u, 1);
    if (numRead > 0)
    {
      if ((buffer[0] == '\x1b') && (buffer[1] == '[') && (buffer[3] == 'R'))
      {
        fitPrint(USER, "Aux switch is %s.\n", ((buffer[2] == 'h')?"ON":"OFF"));
        found = true;
        break;
      }
      else
      {
        // this is not what we expected! print out these false alarms.
        // typically this wouldnt happen.
        int32 j;
        fitPrint(USER, "false alarm: ");
        for (j = 0; j < numRead; j++)
        {
          fitPrint(USER, "%02x ", buffer[j]);
        }
        fitPrint(USER, "\n");
      }
    }
    else
    {
      lErrno = errno;
      if ((lErrno == ETIMEDOUT) || (lErrno == 0))
      {
        return;
      }

      // Error in the read
      usleep (10000);
    }
  }

  if (!found)
  {
    fitPrint(ERROR, "Timed out waiting for proper response from display.\n");
    ftUpdateTestStatus(ftrp,ftError,NULL);
  }
}


static void initSpecialChars(void)
{
  char string[50];
  u_int32 i;

  /* Initialize the special characters to be displayed for testing.
   * This for loop sets each special character to be a filled in box
   * with a height relative to the special character number.
   */
  for (i = 1; i <= MAX_SPECIAL_CHARS; i++)
  {
    snprintf(string, sizeof(string), "\x1b" "P%lu[%u;%u;%u;%u;%u;%uf", i,
            (1u<<i)-1u,(1u<<i)-1u,(1u<<i)-1u,(1u<<i)-1u,(1u<<i)-1u,(1u<<i)-1u);
    writeOut(string);
  }
}

/* special char 1 relative */
static void writeSpecialChar(u_int32 specialChar)
{
  if ((specialChar > 0u) && (specialChar <= MAX_SPECIAL_CHARS))
  {
    char string[10];
    snprintf(string, sizeof(string), "\x1b" "[<%luV",specialChar);

    writeOut(string);
  }
}

/* special char 1 relative */
static void writeSpecialChars(u_int32 startChar, u_int32 writeNum)
{
  u_int32 specialCharNum;
  u_int32 count;

  if ((startChar > 0u) && (writeNum <= MAX_SPECIAL_CHARS))
  {
    specialCharNum = startChar;
    for (count = 0; count < writeNum; count++)
    {
      writeSpecialChar(specialCharNum);
      specialCharNum++;
    }
  }
}

static void defaultSpecialCharsTest(void)
{
  initDisplay();
  backlightOn();
  //move to the middle of the screen
  moveCursor(2,3);
  writeOut("Special Chars shown under their number");
  moveCursor(16,4);
  writeOut("12345678");
  moveCursor(16,5);

  // write out special chars, whatever they are
  initSpecialChars();
  writeSpecialChars(1, 8);

  fitPrint(VERBOSE, "All special characters were printed out in the middle of " \
           "the screen. Check.\n");
  sleep(10);
}

static void moveCursorUp(int32 count)
{
  char string[10];

  snprintf(string, sizeof(string), "\x1b" "[%ldA", count);
  writeOut(string);
}

static void displayTest(void)
{
  initDisplay();
  backlightOn();
  // move to last line and begin.
  moveCursorUp(1);
  setAutoScroll(true);
  initSpecialChars();

  // print standard
  printFullCharSet(96);

  // check reverse video
  if (useRevVid)
  {
    // set reverse vid, run again
    backlightOn();
    setReverseVideo(true);
    printFullCharSet(4);
    sleep(1);

    // reverse and blink?
    if (useCharBlink)
    {
      // set char blink on, with reverse vid
      // run again
      setCharBlink(true);
      printFullCharSet(4);
      setCharBlink(false);
      sleep(1);
    }

    // done with all possible reverse video mumbo jumbo
    setReverseVideo(false);
  }

  // run only char blink with possible underline
  if (useCharBlink)
  {
    // set char blink on
    // run again
    backlightOn();
    setCharBlink(true);
    printFullCharSet(4);
    setCharBlink(false);
    sleep(10);
  }

  // turn off auto scroll. we're done.
  setAutoScroll(false);
}

static void printFullCharSet(u_int32 lines)
{
  const char *asciiString = " !\"#$%&'()*+,-./0123456789:;<=>?" \
                            "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_" \
                            "'abcdefghijklmnopqrstuvwxyz{|}~\x7f";
  char    string[41];
  u_int32 indx;
  size_t  stringSize;
  const char *newline = "\n";

  /* length of the string minus the escape slashes */
  for (indx = 0u; (indx < lines) && (keepGoing); indx++)
  {
    snprintf(string, sizeof(string), "%s", &asciiString[indx]);
    writeOut(string);

    // check to see if string didnt fill up the whole CHARS_PER_LINE chars
    stringSize = strlen(string);

    if (stringSize < CHARS_PER_LINE)
    {
      u_int32 charsNeeded = CHARS_PER_LINE - stringSize;

      if (charsNeeded <= MAX_SPECIAL_CHARS)
      {
        // write as many special chars as need to fill up the rest of the screen
        writeSpecialChars(1, charsNeeded);
      }
      else
      {
        // we need more than 8 chars, so print all special chars
        // then print the beginning section again
        writeSpecialChars(1, MAX_SPECIAL_CHARS);
        // add one to the snprintf size to account for null termination
        snprintf(string, (charsNeeded + 1u - MAX_SPECIAL_CHARS), "%s", asciiString);
        writeOut(string);
      }
    }

    usleep(REFRESH_MS);
    writeOut(newline);
  }

  // start line with special chars
  for (indx = 0; (indx < MAX_SPECIAL_CHARS) && (!keepGoing); indx++)
  {
    u_int32 specialCharsToPrint = MAX_SPECIAL_CHARS - indx;
    writeSpecialChars(indx + 1u, specialCharsToPrint);

    // fill up the remainder with the beginning of asciiString
    // add one to the snprintf size to account for null termination
    snprintf(string, ((CHARS_PER_LINE + 1u) - specialCharsToPrint), "%s", asciiString);
    writeOut(string);

    usleep(REFRESH_MS);
    writeOut(newline);
  }

  // continue with linefeeds until the last special char propogates to (1,1)
  for (indx = 0; (indx < num_o_lines) && (!keepGoing); indx++)
  {
    usleep(REFRESH_MS);
    writeOut(newline);
  }
}

static void clearAllTabs()
{
  clearTabStop(3);
}

static int32 runTabs(void)
{
  u_int32 i, j, k;

  resetDisplay();
  initDisplay();
  setCursor(true);
  backlightOn();

  fitPrint(VERBOSE, "We will now begin tab testing.\n");
  fitPrint(VERBOSE, "Default tabs should be at columns 9, 17, 25, and 33.\n");

  fitPrint(VERBOSE, "\nIf 1,2,3,4 aren't separated by tabs(spaces), " \
           "then reset the board to default.\n");

  writeOut("1234567890123456789012345678901234567890\n");

  for (i = 0; i < num_o_lines; i++)
  {
    writeOut("\t1\t2\t3\t4\n");
    usleep(500000);
  }

  fitPrint(VERBOSE, "Now clearing all tab stops and running same test.\n");

  // clear all tab stops
  clearAllTabs();
  clearScreen();

  for (i = 0; i < num_o_lines; i++)
  {
    writeOut("\t1\t2\t3\t4");
        if (i < (num_o_lines - 1u))
        {
          writeOut("\n");
        }
    usleep(500000);
  }

  fitPrint(VERBOSE, "\nVerify no spacing (tabs) in between 1,2,3, and 4.\n");

  // all location tab stop test
  fitPrint(VERBOSE, "\n\nWe will now test all columns and rows for tab stops.\n");
  fitPrint(VERBOSE, "WARNING: This will take roughly 10 minutes.\n");

  clearAllTabs();
  clearScreen();
  setCursor(true);

  for (i = 0; (i < num_o_lines) && (keepGoing); i++)
  {
    backlightOn();
    for (j = 0; (j < CHARS_PER_LINE) && (keepGoing); j++)
    {
      clearScreen();
      fitPrint(VERBOSE, "setting tabstop on row %lu column %lu\n", i+1u, j+1u);
      // set cursor to this location and set a tab stop
      moveCursor(j+1u, i+1u);
      setTabStop();

      //move the cursor back home to start the test
      moveCursorHome();

      for (k = 0; k < num_o_lines; k++)
      {
        if (i == k)
        {
          writeOut("\t\x1b" "[27hT\x1b" "[27l"); // reverse video
        }
        else
        {
          writeOut("\tT");
        }

        if (k < (num_o_lines - 1u))
        {
          writeOut("\n");
        }

        //usleep(10000);
      }

      usleep(500000);

      // clear the tab from this location
      moveCursor(j+1u, i+1u);
      fitPrint(VERBOSE, "\t\tclearing tabstop on row %lu column %lu\n", i+1u, j+1u);
      clearTabStop((int32)j % 3); // this is to test all
    }
  }

#if 0
  fitPrint(VERBOSE, "Tabs testing is complete.");
  fitPrint(VERBOSE, "Enter 'q' to quit now, otherwise, display testing will commence.");

  if (getchar() == 'q')
  {
    return 1;
  }
#endif

  return 0;
}

static void keypadTest(void)
{
  char buffer[10];
  char outbuf[10];
  int32 numRead;
  int32 count = 0;

  initDisplay();
  setCursor(true);
  setAutoWrap(true);
  backlightOn();

  moveCursor(15,1);
  writeOut("Keypad Test");
  moveCursor(8,2);
  writeOut("Press all keys on keypad and");
  moveCursor(12,3);
  writeOut("toggle AUX switch\n");
  fitPrint(VERBOSE, "Running Keypad Test\n");
  fitPrint(VERBOSE, "Verify that all keys are reporting expected values.\n");

  while (count < 37)
  {
    memset(buffer, 0, sizeof(buffer));

    do {
      numRead = read (fpd, buffer, sizeof(buffer));
    } while (numRead < 1);

    if (numRead > 0)
    {
      // just one normal escaped char
      numRead = decodeCharSequence(buffer, outbuf, sizeof(outbuf));
      if (numRead > 0)
      {
        strcat (outbuf, ", ");
        writeOut (outbuf);
        count++;
      }
    }
    else if (numRead < 0)
    {
      fitPrint(VERBOSE, "an error occurred reading from keypad. Quitting...");
      return;
    }
    else //numRead == 0
    {
      // do nothing
    }
  }

  // done
  writeOut("\n\n... test ending.");
  sleep(3);
}

static int32 decodeCharSequence(const char *buffer, char *outbuf, size_t bsize)
{
  u_int32 i;
  int32 ret = 0;

  if (buffer[0] == '\x0a')
  {
    // we dont want to print <CR> so substitute the key's label
    snprintf(outbuf, bsize, "ENT");
    ret = 1;  // took up 1 char of buffer
  }
  else if (isgraph(buffer[0]) )
  {
    // printout reg characters
    snprintf(outbuf, bsize, "%c", buffer[0]);
    ret = 1;  // took up 1 char of buffer
  }
  else if (buffer[0] == '\x1b') // ESC
  {
    if ((buffer[1] == '['))
    {
      ret = 3; // this should take up 3 chars of buffer
      switch(buffer[2])
      {
        case 'A':
          snprintf (outbuf, bsize, "UP");
          break;

        case 'B':
          snprintf (outbuf, bsize, "DOWN");
          break;

        case 'C':
          snprintf (outbuf, bsize, "RIGHT");
          break;

        case 'D':
          snprintf (outbuf, bsize, "LEFT");
          break;

        case 'K': // Double [simultaneous?] key press
          snprintf (outbuf, bsize, "DOUBLE");
          ret = 5; // double press takes up 5 chars of buffer
          break;

        default:
          // Unknown <esc> [ sequence
          // This should not happen
          snprintf (outbuf, bsize, "UNK-esc[");
          break;
      }

    }
    else if (buffer[1] == 'O')
    {
      ret = 3; // this should take up 3 chars of buffer
      switch(buffer[2])
      {
        case 'P':
          snprintf (outbuf, bsize, "NEXT");
          break;

        case 'Q':
          snprintf (outbuf, bsize, "YES");
          break;

        case 'R':
          snprintf (outbuf, bsize, "NO");
          break;

        case 'S':
          snprintf (outbuf, bsize, "ESC");
          break;

        case 'T':
          snprintf (outbuf, bsize, "AUX-ON");
          break;

        case 'U':
          snprintf (outbuf, bsize, "AUX-OFF");
          break;

        // Vendor Specific Keys
        case 'V':
          snprintf (outbuf, bsize, "VSK1/F1");
          break;

        case 'W':
          snprintf (outbuf, bsize, "VSK2/F2");
          break;

        case 'X':
          snprintf (outbuf, bsize, "VSK3/F3");
          break;

        case 'Y':
          snprintf (outbuf, bsize, "VSK4/F4");
          break;

        case 'Z':
          snprintf (outbuf, bsize, "VSK5/F5");
          break;

        case '[':
          snprintf (outbuf, bsize, "VSK6/HELP");
          break;

        case '\\':
          snprintf (outbuf, bsize, "VSK7/PREV");
          break;

        case '^':
          snprintf (outbuf, bsize, "VSK8");
          break;

        case ']':
          snprintf (outbuf, bsize, "VSK9");
          break;

        case '_':
          snprintf (outbuf, bsize, "VSK10");
          break;

        case '`':
          snprintf (outbuf, bsize, "VSK11");
          break;

        case 'a':
          snprintf (outbuf, bsize, "VSK12");
          break;

        case 'b':
          snprintf (outbuf, bsize, "VSK13");
          break;

        case 'c':
          snprintf (outbuf, bsize, "VSK14");
          break;

        case 'd':
          snprintf (outbuf, bsize, "VSK15");
          break;

        case 'e':
          snprintf (outbuf, bsize, "VSK16");
          break;

        default:
          // Unknown <esc> O sequence
          // This should not happen
          snprintf (outbuf, bsize, "Unkn-escO");
          break;
      }
    }
    else
    {
      // Unknown <esc> sequence
      snprintf (outbuf, bsize, "Unkn-esc");
      fitPrint(VERBOSE, "WARNING [0]!!! we just received an invalid sequence from the keypad\n");
      fitPrint(VERBOSE, "recieved: ");
      for (i = 0; i < strlen(buffer); i++)
      {
        fitPrint(VERBOSE, "%02x ", buffer[i]);
      }
      fitPrint(VERBOSE, "\n");
      ret = -1;
    }
  }
  else
  {
    // Unknown non-<esc> sequence
    snprintf (outbuf, bsize, "Unknown");
    fitPrint(VERBOSE, "WARNING [1]!!! we just received an invalid sequence from the keypad\n");

    fitPrint(VERBOSE, "recieved: ");
    for (i = 0; i < strlen(buffer); i++)
    {
      fitPrint(VERBOSE, "%02x ", buffer[i]);
    }
    fitPrint(VERBOSE, "\n");
    ret = -1;
  }

  return ret;
}

