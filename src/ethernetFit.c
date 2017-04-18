/******************************************************************************
                                 ethernetFit.c

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
 * ethernetFit.c
 *
 * Minimal test to ensure ethernet interfaces are functioning.
 *
 * Configures two virtual interfaces (eth0:5/eth1:5) and then
 * performs a simple ping to ensure that they are both working.
 */

#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>
#include "fit.h"

// Used to increment octet of IP Address
#define INC_SECOND_OCTET  0x00010000uL

#define DEFAULT_PING_TIMEOUT 5u

#define MY_NETMASK         "255.255.0.0"
#define DEFAULT_PING_OCTET "254"

static const char *processName = "ethernetFit";

static ftResults_t ftResults = {0};
static ftResults_t *ftrp = &ftResults;

static char configFile[50] = {'\0'};
static char pingOctet[8] = {'\0'};
static char if_uped_name[2][IFNAMSIZ] = {{'\0'}};
static u_int32  pingTimeout = DEFAULT_PING_TIMEOUT;

// Struct to hold IP configuration items
typedef struct
{
  char if_name[IFNAMSIZ];

  struct sockaddr if_addr;
  struct sockaddr if_broadaddr;
  struct sockaddr if_netmask;
} ethInfo;

static void printUsage (char * const argv[])
{
  fitPrint(USER, "Usage: fit %s [OPTION]\n", argv[0]);
  fitPrint(USER, "Pings IP addresses based on pre-configured eth0 address.\n");
  fitPrint(USER, "Configures eth0:5 and eth1:5 to identical IP's as eth0 but with\n");
  fitPrint(USER, "the second octet incremented once for eth0:5 and twice for eth1:5.\n");
  fitPrint(USER, "Once the interfaces are configured it pings an IP address on that\n");
  fitPrint(USER, "new network.\n");
  fitPrint(USER, "\n");
  fitPrint(USER, "  -c file  use IP addresses specified in file. Overrides pre-configuration scheme\n");
  fitPrint(USER, "  -o       specify last octet to ping (default = %s)\n", DEFAULT_PING_OCTET);
  fitPrint(USER, "  -t       specify timeout in seconds for ping (default = %u)\n", DEFAULT_PING_TIMEOUT);
  fitPrint(USER, "  -h,-?    show this usage text and exit\n");
  fitPrint(USER, "\n");
  fitPrint(USER, "Examples:\n");
  fitPrint(USER, "fit %s -o 230 -t 10\n", argv[0]);
  fitPrint(USER, "  If eth0 is configured for 10.227.3.39 then:\n");
  fitPrint(USER, "\teth0:5 is configured for 10.228.3.39 and pings 10.228.3.230 with timeout of 10 seconds\n");
  fitPrint(USER, "\teth1:5 is configured for 10.229.3.39 and pings 10.229.3.230 with timeout of 10 seconds\n");
  fitPrint(USER, "\n");
  fitPrint(USER, "fit %s\n", argv[0]);
  fitPrint(USER, "  If eth0 is configured for 10.227.3.39 then:\n");
  fitPrint(USER, "\teth0:5 is configured for 10.228.3.39 and pings 10.228.3.%s with timeout of %u seconds\n",
           DEFAULT_PING_OCTET, DEFAULT_PING_TIMEOUT);
  fitPrint(USER, "\teth1:5 is configured for 10.229.3.39 and pings 10.229.3.%s with timeout of %u seconds\n",
           DEFAULT_PING_OCTET, DEFAULT_PING_TIMEOUT);
  fitPrint(USER, "\n");
  fitPrint(USER, "Optional config file:\n");
  fitPrint(USER, "An optional configuration file can be used to specify the exact IP addresses to use\n");
  fitPrint(USER, "for the ethernet interfaces. The file must be in the following format:\n");
  fitPrint(USER, "[eth0 IP address]\n");
  fitPrint(USER, "[eth0 netmask]\n");
  fitPrint(USER, "[eth0 ping target address]\n");
  fitPrint(USER, "[eth1 IP address]\n");
  fitPrint(USER, "[eth1 netmask]\n");
  fitPrint(USER, "[eth1 ping target address]\n\n");
  fitPrint(USER, "A file matching the configuration for the first example above would be:\n");
  fitPrint(USER, "10.228.3.39\n");
  fitPrint(USER, "255.255.0.0\n");
  fitPrint(USER, "10.228.3.230\n");
  fitPrint(USER, "10.229.3.39\n");
  fitPrint(USER, "255.255.0.0\n");
  fitPrint(USER, "10.229.3.230\n");
  fitPrint(USER, "\n");
  fitLicense();
}


static void printEthInfo (ethInfo *myInfo)
{
  char if_addr_str[24], broadaddr_str[24], netmask_str[24];

  //lint --e{740,9087}  Ignore unusual pointer casts required by BSD sockets
  strcpy (if_addr_str, inet_ntoa (((struct sockaddr_in *) &myInfo->if_addr)->sin_addr));
  strcpy (broadaddr_str, inet_ntoa (((struct sockaddr_in *) &myInfo->if_broadaddr)->sin_addr));
  strcpy (netmask_str, inet_ntoa (((struct sockaddr_in *) &myInfo->if_netmask)->sin_addr));

  fitPrint(VERBOSE, "%s\n\taddr %s\n\tbroadcast %s\n\tnetmask %s\n",
           myInfo->if_name, if_addr_str, broadaddr_str, netmask_str);
}

static int32 getInterfaceInfo (ethInfo *destInfo)
{
  struct ifreq eth_ifr;
  int32 fd, ret;

  // Get file descriptor for socket
  fd = socket (AF_INET, SOCK_DGRAM, 0);

  if (fd < 0)
  {
    fitPrint(ERROR, "%s: can't open socket, err %d, %s\n",
             processName, errno, strerror (errno));
    return -1;
  }

  // Setup interface req for ioctl call
  memset (&eth_ifr, 0, sizeof (struct ifreq));
  strncpy (eth_ifr.ifr_name, destInfo->if_name, IFNAMSIZ);

  // Get and save pertinent interface info
  ret = ioctl (fd, SIOCGIFADDR, &eth_ifr);

  if (ret == -1)
  {
    fitPrint(ERROR, "%s: failed ioctl SIOCGIFADDR failed %s, err %d, %s\n",
             processName, destInfo->if_name, errno, strerror (errno));
  }
  else
  {
    memcpy (&destInfo->if_addr, &eth_ifr.ifr_addr, sizeof (destInfo->if_addr));

    // Setup interface req for ioctl call
    memset (&eth_ifr, 0, sizeof (struct ifreq));
    strncpy (eth_ifr.ifr_name, destInfo->if_name, IFNAMSIZ);

    ret = ioctl (fd, SIOCGIFBRDADDR, &eth_ifr);

    if (ret == -1)
    {
      fitPrint(ERROR, "%s: failed ioctl SIOCGIFBRDADDR failed %s, err %d, %s\n",
               processName, destInfo->if_name, errno, strerror (errno));
    }
    else
    {
      memcpy (&destInfo->if_broadaddr, &eth_ifr.ifr_broadaddr, sizeof (destInfo->if_broadaddr));

      // Setup interface req for ioctl call
      memset (&eth_ifr, 0, sizeof (struct ifreq));
      strncpy (eth_ifr.ifr_name, destInfo->if_name, IFNAMSIZ);

      ret = ioctl (fd, SIOCGIFNETMASK, &eth_ifr);

      if (ret == -1)
      {
        fitPrint(ERROR, "%s: failed ioctl SIOCGIFNETMASK failed %s, err %d, %s\n",
                 processName, destInfo->if_name, errno, strerror (errno));
      }
      else
      {
        memcpy (&destInfo->if_netmask, &eth_ifr.ifr_netmask, sizeof (destInfo->if_netmask));
        fitPrint(VERBOSE, "Successfully got configuration for %s\n", destInfo->if_name);
      }
    }
  }

  // Close socket, we are done with it
  close (fd);
  return ret;
}

static int32 setInterfaceInfo (ethInfo *srcInfo)
{
  struct ifreq eth_ifr;
  int32 fd, ret;

  // Get file descriptor for socket
  fd = socket (AF_INET, SOCK_DGRAM, 0);

  if (fd < 0)
  {
    fitPrint(ERROR, "%s: can't open socket, err %d, %s\n",
             processName, errno, strerror (errno));
    return -1;
  }

  // Setup interface req for ioctl call
  memset (&eth_ifr, 0, sizeof (struct ifreq));
  strncpy (eth_ifr.ifr_name, srcInfo->if_name, IFNAMSIZ);
  memcpy (&eth_ifr.ifr_addr, &srcInfo->if_addr, sizeof (srcInfo->if_addr));

  // Get and save pertinent interface info
  ret = ioctl (fd, SIOCSIFADDR, &eth_ifr);

  if (ret == -1)
  {
    fitPrint(ERROR, "%s: failed ioctl SIOCSIFADDR failed %s, err %d, %s\n",
             processName, srcInfo->if_name, errno, strerror (errno));
  }
  else
  {
    // Setup interface req for ioctl call
    memset (&eth_ifr, 0, sizeof (struct ifreq));
    strncpy (eth_ifr.ifr_name, srcInfo->if_name, IFNAMSIZ);
    memcpy (&eth_ifr.ifr_broadaddr, &srcInfo->if_broadaddr, sizeof (srcInfo->if_broadaddr));

    ret = ioctl (fd, SIOCSIFBRDADDR, &eth_ifr);

    if (ret == -1)
    {
      fitPrint(ERROR, "%s: failed ioctl SIOCSIFBRDADDR failed %s, err %d, %s\n",
               processName, srcInfo->if_name, errno, strerror (errno));
    }
    else
    {
      // Setup interface req for ioctl call
      memset (&eth_ifr, 0, sizeof (struct ifreq));
      strncpy (eth_ifr.ifr_name, srcInfo->if_name, IFNAMSIZ);
      memcpy (&eth_ifr.ifr_netmask, &srcInfo->if_netmask, sizeof (srcInfo->if_netmask));

      ret = ioctl (fd, SIOCSIFNETMASK, &eth_ifr);

      if (ret == -1)
      {
        fitPrint(ERROR, "%s: failed ioctl SIOCSIFNETMASK failed %s, err %d, %s\n",
                 processName, srcInfo->if_name, errno, strerror (errno));
      }
      else
      {
        ret = ioctl(fd, SIOCGIFFLAGS, &eth_ifr);

        if (ret == -1)
        {
          fitPrint(ERROR, "%s: failed ioctl SIOCGIFFLAGS failed %s, err %d, %s\n",
                   processName, srcInfo->if_name, errno, strerror (errno));
        }
        else
        {
          //lint -e{9027,9034} MISRA hates bit ops on signed, but can't be helped
          eth_ifr.ifr_flags |= IFF_UP | IFF_RUNNING;

          ret = ioctl(fd, SIOCSIFFLAGS, &eth_ifr);

          if (ret == -1)
          {
            fitPrint(ERROR, "%s: failed ioctl SIOCSIFFLAGS failed %s, err %d, %s\n",
                     processName, srcInfo->if_name, errno, strerror (errno));
          }
          else
          {
            fitPrint(VERBOSE, "Successfully configured %s\n", srcInfo->if_name);
          }
        }
      }
    }
  }

  // Close socket, we are done with it
  close (fd);
  return ret;
}

static int32 if_isUp (char *if_name)
{
  char *ch_ptr;
  struct ifreq eth_ifr;
  int32 fd, ret;

  // Get file descriptor for socket
  fd = socket (AF_INET, SOCK_DGRAM, 0);

  if (fd < 0)
  {
    fitPrint(ERROR, "%s: can't open socket, err %d, %s\n",
             processName, errno, strerror (errno));
    return -1;
  }

  // Setup interface req for ioctl call
  memset (&eth_ifr, 0, sizeof (struct ifreq));
  strncpy (eth_ifr.ifr_name, if_name, IFNAMSIZ);

  ch_ptr = strstr (eth_ifr.ifr_name, ":");
  if (ch_ptr != NULL)
  {
    *ch_ptr = '\0'; // For this function end the interface name at the colon
  }

  ret = ioctl(fd, SIOCGIFFLAGS, &eth_ifr);

  if (ret == -1)
  {
    fitPrint(ERROR, "%s: failed ioctl SIOCGIFFLAGS failed %s, err %d, %s\n",
             processName, if_name, errno, strerror (errno));
  }
  else
  {
    //lint -e{9027,9034} MISRA doesn't like bit ops on signed
    if ((eth_ifr.ifr_flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING))
    {
      ret = 1;
    }
    else
    {
      ret = 0;
    }
  }

  close (fd);
  return ret;
}

static int32 if_setUp (char *if_name)
{
  char *ch_ptr;
  struct ifreq eth_ifr;
  int32 fd, ret;

  // Get file descriptor for socket
  fd = socket (AF_INET, SOCK_DGRAM, 0);

  if (fd < 0)
  {
    fitPrint(ERROR, "%s: can't open socket, err %d, %s\n",
             processName, errno, strerror (errno));
    return -1;
  }

  // Setup interface req for ioctl call
  memset (&eth_ifr, 0, sizeof (struct ifreq));
  strncpy (eth_ifr.ifr_name, if_name, IFNAMSIZ);

  ch_ptr = strstr (eth_ifr.ifr_name, ":");
  if (ch_ptr != NULL)
  {
    *ch_ptr = '\0'; // For this function end the interface name at the colon
  }

  ret = ioctl(fd, SIOCGIFFLAGS, &eth_ifr);

  if (ret == -1)
  {
    fitPrint(ERROR, "%s: failed ioctl SIOCGIFFLAGS failed %s, err %d, %s\n",
             processName, if_name, errno, strerror (errno));
  }
  else
  {
    //lint -e{9027,9034} MISRA doesn't like bit ops on signed
    eth_ifr.ifr_flags |= IFF_UP | IFF_RUNNING;

    ret = ioctl(fd, SIOCSIFFLAGS, &eth_ifr);

    if (ret == -1)
    {
      fitPrint(ERROR, "%s: failed ioctl SIOCSIFFLAGS failed %s, err %d, %s\n",
               processName, if_name, errno, strerror (errno));
    }
    else
    {
      u_int32 i;

      for (i = 0; i < (sizeof (if_uped_name) / sizeof (if_uped_name[0])); i++)
      {
        if (if_uped_name[i][0] == '\0')
        {
          strncpy (if_uped_name[i], eth_ifr.ifr_name, IFNAMSIZ);
          break;
        }
      }
    }
  }

  close (fd);
  return ret;
}

static int32 if_down (char *if_name)
{
  struct ifreq eth_ifr;
  int32 ret, fd;

  // Get file descriptor for socket
  fd = socket (AF_INET, SOCK_DGRAM, 0);

  if (fd < 0)
  {
    fitPrint(ERROR, "%s: can't open socket, err %d, %s\n",
             processName, errno, strerror (errno));
    return -1;
  }

  // Setup interface req for ioctl call
  memset (&eth_ifr, 0, sizeof (struct ifreq));
  strncpy (eth_ifr.ifr_name, if_name, IFNAMSIZ);

  ret = ioctl(fd, SIOCGIFFLAGS, &eth_ifr);

  if (ret == -1)
  {
    fitPrint(ERROR, "%s: failed ioctl SIOCGIFFLAGS failed %s, err %d, %s\n",
             processName, if_name, errno, strerror (errno));
  }
  else
  {
    //lint -e{9027,9034} MISRA doesn't like bit ops on signed
    eth_ifr.ifr_flags &= ~(IFF_UP | IFF_RUNNING);

    ret = ioctl(fd, SIOCSIFFLAGS, &eth_ifr);

    if (ret == -1)
    {
      fitPrint(ERROR, "%s: failed ioctl SIOCSIFFLAGS failed %s, err %d, %s\n",
               processName, if_name, errno, strerror (errno));
    }
  }

  close (fd);
  return ret;
}

static int32 if_setDown (void)
{
  u_int32 i;
  int32 ret;

  ret = 0;

  for (i = 0; i < (sizeof (if_uped_name) / sizeof (if_uped_name[0])); i++)
  {
    if (if_uped_name[i][0] != '\0')
    {
      ret = (if_down (if_uped_name[i]) != 0) ? -1 : ret;
    }
  }

  return ret;
}

/* Pings the supplied ip address */
static int32 ping (char *pingAddress)
{
  int32 ret = 0;
  pid_t pingPid, timeoutPid;

  /*
   * Below forks off one process for doing the ping (pindPid) and one process
   * to wait a timeout (timeoutPid). Then the main process waits for one of the
   * two to finish first (because ping goes continuously until you kill it).
   */

  // Fork off ping process
  pingPid = fork ();

  if (pingPid < 0)
  {
    fitPrint(ERROR, "Error forking ping process: %s", strerror(errno));
    ftUpdateTestStatus(ftrp,ftError,NULL);
  }
  else if (pingPid > 0)
  {
    plint retStatus = 0;
    pid_t waitRet;

    // Fork off timeout process
    timeoutPid = fork ();

    if (timeoutPid < 0)
    {
      fitPrint(ERROR, "Error forking timeout process: %s", strerror(errno));
      ftUpdateTestStatus(ftrp,ftError,NULL);
    }
    else if (timeoutPid == 0)
    {
      // Timeout process, so sleep the timeout and then exit ()
      sleep (pingTimeout);
      exit(-1);
    }
    else
    {
      // Wait for either the timeout process or the ping process to exit first
      waitRet = wait(&retStatus);  //lint !e64 int* is indeed correct as arg

      if (waitRet == timeoutPid)
      {
        // Timeout process exited first, which means we timed out
        ftUpdateTestStatus(ftrp,ftError,NULL);
        fitPrint(VERBOSE, "PING FAILED (timeout) for %s\n\n", pingAddress);
        kill (pingPid, SIGKILL); // Timedout so kill the ping process
        ret = -1;
      }
      else if (waitRet == pingPid)
      {
        // Ping process exited first, which means we were successful
        fitPrint(VERBOSE, "PING SUCCESSFUL for %s returned %d\n\n",
                 pingAddress, retStatus);
        ftUpdateTestStatus(ftrp,ftPass,NULL);
        kill (timeoutPid, SIGKILL); // Kill the timeoutPid
      }
      else
      {
        fitPrint(ERROR, "PING FAILED: wait returned %d, unknown error %d : %s\n\n",
                 waitRet, errno, strerror (errno));
        ftUpdateTestStatus(ftrp,ftError,NULL);

        // Unknown error, kill both processes
        kill (timeoutPid, SIGKILL); // Kill the timeoutPid
        kill (pingPid, SIGKILL); // kill the ping process

        // Since we are killing both we need to do an extra wait() here
        (void)wait (NULL);  //lint !e64  this is not a type mismatch
        ret = -1;
      }

      // Collect dying process(es) to confirm they are gone.
      (void)wait (NULL);  //lint !e64  this is not a type mismatch
    }
  }
  else //pingPid == 0, I am the ping process
  {
    if (verboseFlag)
    {
      // Redirect STDOUT to /dev/null, we don't want the output of the ping to come out
      int32 devNull = open("/dev/null", O_WRONLY);
      (void)dup2(devNull, STDOUT_FILENO);
    }

    (void)execlp ("ping", "ping", "-c1", pingAddress, (char *) 0);
    // execlp() only returns if it fails and always returns -1

    fitPrint(ERROR, "PING FAILED: execlp returned error %d : %s\n",
             errno, strerror (errno));
    ftUpdateTestStatus(ftrp,ftError,NULL);
    ret = -1;
  }

  return ret;
}

static int32 parseEthArguments (plint argc, char * const argv[])
{
  int32 result;
  int32 ret = 0;
  int32 c = getopt (argc, argv, "-o:t:c:h");

  while ((c != -1) && (ret == 0))
  {
    switch ((char)c)
    {
      default:
        fitPrint(ERROR, "Unknown argument: %s\n", argv[optind - 1]);
        ftUpdateTestStatus(ftrp,ftError,NULL);
        printUsage(argv);
        ret = -1; // Return -1 to notify main to stop
        break;
      case '?':
      case 'h':
        printUsage(argv);
        ret = -1; // Return -1 to notify main to stop
        break;

      case 'c':
        strncpy (configFile, optarg, sizeof (configFile));
        break;

      case 'o':
        strncpy (pingOctet, optarg, sizeof (pingOctet));
        break;

      case 't':
        result = sscanf (optarg, "%lu", &pingTimeout);
        if (result == 0)
        {
          fitPrint(ERROR, "Bad Argument for timeout: %s", strerror(errno));
          ret = -1; //Failed to convert arg to integer
                    // Return -1 to notify main to stop
        }
        break;

    } // switch(c)

    c = getopt (argc, argv, "-o:t:c:h");
  }

  if (pingOctet[0] == '\0') // Ping octet un-initialized
  {
    strcpy (pingOctet, DEFAULT_PING_OCTET);
  }

  return ret;
}

/*
 * importFromFile(...)
 *
 * Reads in the user supplied config file and populates the ethInfos and ping addresses given.
 * NOTE: Ping addresses must be 16 bytes each
 *
 * Config file is as follows:
 * [eth0 ip address]
 * [eth0 netmask]
 * [eth0 ping address]
 * [eth1 ip address]
 * [eth1 netmask]
 * [eth1 ping address]
 */
static int32 importFromFile (ethInfo * eth0, ethInfo *eth1, char *eth0PingAddress, char *eth1PingAddress)
{
  char address[16] = {'\0'};
  const char *result;
  FILE *fp = fopen(configFile, "r");

  if (fp == NULL)
  {
    fitPrint(ERROR, "Could not open %s: %s\n", configFile, strerror(errno));
    return -1;
  }

  // get eth0 ip
  result = fgets(address, (int32)sizeof(address), fp);
  if ((result == NULL) || (strlen(address) == 0u))  //lint !e9007 strlen has no side effects
  {
    fitPrint(ERROR, "Could not read eth0 ip address from %s\n", configFile);
    fclose(fp);
    return -1;
  }

  //lint -e{740,9087}  Ignore unusual pointer casts required by BSD sockets
  ((struct sockaddr_in *) &eth0->if_addr)->sin_addr.s_addr = inet_addr (address);

  memset(address, 0, sizeof(address));

  // get eth0 netmask
  result = fgets(address, (int32)sizeof(address), fp);
  if ((result == NULL) || (strlen(address) == 0u))  //lint !e9007 strlen
  {
    fitPrint(ERROR, "Could not read eth0 netmask from %s\n", configFile);
    fclose(fp);
    return -1;
  }

  //lint -e{740,9087}  Ignore unusual pointer casts required by BSD sockets
  ((struct sockaddr_in *) &eth0->if_netmask)->sin_addr.s_addr = inet_addr(address);

  //lint -e{740,9087}  Ignore unusual pointer casts required by BSD sockets
  ((struct sockaddr_in *) &eth0->if_broadaddr)->sin_addr.s_addr =
      ( ((struct sockaddr_in *) &eth0->if_netmask)->sin_addr.s_addr &
        ((struct sockaddr_in *) &eth0->if_addr)->sin_addr.s_addr) |
      (~((struct sockaddr_in *) &eth0->if_netmask)->sin_addr.s_addr & 0xFFFFFFFFu);

  // get eth0 ping address
  result = fgets(eth0PingAddress, 16, fp);
  if ((result == NULL) || (strlen(eth0PingAddress) == 0u)) //lint !e9007 strlen
  {
    fitPrint(ERROR, "Could not read eth0 ping address from %s\n", configFile);
    fclose(fp);
    return -1;
  }

  memset(address, 0, sizeof(address));

  // get eth1 ip
  result = fgets(address, (int32)sizeof(address), fp);
  if ((result == NULL) || (strlen(address) == 0u))  //lint !e9007 strlen
  {
    fitPrint(ERROR, "Could not read eth1 ip address from %s\n", configFile);
    fclose(fp);
    return -1;
  }

  //lint -e{740,9087}  Ignore unusual pointer casts required by BSD sockets
  ((struct sockaddr_in *) &eth1->if_addr)->sin_addr.s_addr = inet_addr(address);

  memset(address, 0, sizeof(address));

  // get eth1 netmask
  result = fgets(address, (int32)sizeof(address), fp);
  if ((result == NULL) || (strlen(address) == 0u)) //lint !e9007 strlen
  {
    fitPrint(ERROR, "Could not read eth1 netmask from %s\n", configFile);
    fclose(fp);
    return -1;
  }

  //lint -e{740,9087}  Ignore unusual pointer casts required by BSD sockets
  ((struct sockaddr_in *) &eth1->if_netmask)->sin_addr.s_addr = inet_addr (address);

  //lint -e{740,9087}  Ignore unusual pointer casts required by BSD sockets
  ((struct sockaddr_in *) &eth1->if_broadaddr)->sin_addr.s_addr =
      ( ((struct sockaddr_in *) &eth1->if_netmask)->sin_addr.s_addr &
        ((struct sockaddr_in *) &eth1->if_addr)->sin_addr.s_addr) |
      (~((struct sockaddr_in *) &eth1->if_netmask)->sin_addr.s_addr & 0xFFFFFFFFu);

  // get eth1 ping address
  result = fgets(eth1PingAddress, 16, fp);

  if ((result == NULL) || (strlen(eth1PingAddress) == 0u)) //lint !e9007 strlen
  {
    fitPrint(ERROR, "Could not read eth1 ping address from %s\n", configFile);
    fclose(fp);
    return -1;
  }

  fclose(fp);

  return 0;
}

/*
 * importDefaultSettings (...)
 *
 * Populates arguments with the default settings.
 * NOTE: eth0PingAddress and eth1PingAddress both must be 16 bytes long
 */
static void importDefaultSettings (ethInfo * eth0, ethInfo *eth1, char *eth0PingAddress, char *eth1PingAddress)
{
  char * lastOctetPtr;
  //lint --e{740,9087}  Ignore unusual pointer casts required by BSD sockets

  // Edit eth0 virtual interface for ping test
  ((struct sockaddr_in *) &eth0->if_netmask)->sin_addr.s_addr =    //lint !e740
                          inet_addr (MY_NETMASK);
  ((struct sockaddr_in *) &eth0->if_addr)->sin_addr.s_addr +=      //lint !e740
                          INC_SECOND_OCTET;
  ((struct sockaddr_in *) &eth0->if_broadaddr)->sin_addr.s_addr += //lint !e740
                          INC_SECOND_OCTET;

  // adjust the ping address for eth1
  strncpy (eth0PingAddress,
           inet_ntoa (((struct sockaddr_in *) &eth0->if_addr)->sin_addr), 16); //lint !e740

  // Get pointer to start of last octet in IP Address
  //lint -e{613}  eth0PingAddress is known to be well formed at this point.
  {
    lastOctetPtr = strstr (eth0PingAddress, ".");
    lastOctetPtr = strstr (&lastOctetPtr[1], ".");
    lastOctetPtr = strstr (&lastOctetPtr[1], ".");
    lastOctetPtr ++;
  }

  // Replace last octet with our configured octet
  snprintf (lastOctetPtr, 4, "%s", pingOctet); // 4 because (3+null) is the biggest an octet should be

  // Set up eth1 virtual interface

  // Edit eth1 virtual interface for ping test
  ((struct sockaddr_in *) &eth1->if_netmask)->sin_addr.s_addr =    //lint !e740
                          inet_addr (MY_NETMASK);

  // Increment eth1:5 twice
  ((struct sockaddr_in *) &eth1->if_addr)->sin_addr.s_addr +=      //lint !e740
                          INC_SECOND_OCTET + INC_SECOND_OCTET;

  // Increment eth1:5 twice
  ((struct sockaddr_in *) &eth1->if_broadaddr)->sin_addr.s_addr += //lint !e740
                          INC_SECOND_OCTET + INC_SECOND_OCTET;

  // adjust the ping address for eth1
  strncpy (eth1PingAddress,
           inet_ntoa (((struct sockaddr_in *) &eth1->if_addr)->sin_addr), //lint !e740
           16);

  // Get pointer to start of last octet in IP Address
  //lint -e{613}  eth1PingAddress is known to be well formed at this point.
  {
    lastOctetPtr = strstr (eth1PingAddress, ".");
    lastOctetPtr = strstr (&lastOctetPtr[1], ".");
    lastOctetPtr = strstr (&lastOctetPtr[1], ".");
    lastOctetPtr ++;
  }

  // Replace last octet with our configured octet
  snprintf (lastOctetPtr, 4, "%s", pingOctet); // 4 because (3+null) is the biggest an octet should be
}

// Ethernet test entry
ftRet_t ethernetFit (plint argc, char * const argv[])
{
  char errorStr[MUST_BE_BIG_ENOUGH];
  int32 ret, eth0Fail, eth1Fail;
  ethInfo eth0_orig_if, eth0_new_if, eth1_new_if;
  char eth0PingAddress[16] = {'\0'};
  char eth1PingAddress[16] = {'\0'};
  const char *eth0name = "eth0:5";
  const char *eth1name = "eth1:5";

  ret = parseEthArguments (argc, argv);

  if (ret != 0)
  {
    // Parse arguments says exit early
    return (ftUpdateTestStatus(ftrp,ftComplete,NULL));
  }


  // Clear out then initialize interface data
  memset (&eth0_orig_if, 0, sizeof (eth0_orig_if));

  strncpy (eth0_orig_if.if_name, "eth0", IFNAMSIZ);

  ret = getInterfaceInfo (&eth0_orig_if);
  if (ret != 0)
  {
    return (ftUpdateTestStatus(ftrp,ftError,NULL));
  }

  // Copy eth0's original for eth0 virtual interface
  memcpy (&eth0_new_if, &eth0_orig_if, sizeof(eth0_orig_if));
  strncpy (eth0_new_if.if_name, eth0name, IFNAMSIZ);

  // Copy eth0's original for eth1 virtual interface
  memcpy (&eth1_new_if, &eth0_orig_if, sizeof(eth0_orig_if));
  strncpy (eth1_new_if.if_name, eth1name, IFNAMSIZ);

  // Check to see user gave a config file. If so, use it. If not, revert to default behaviour
  if (configFile[0] != '\0')
  {
    if (importFromFile (&eth0_new_if, &eth1_new_if, eth0PingAddress, eth1PingAddress) < 0)
    {
      return (ftUpdateTestStatus(ftrp,ftComplete,NULL));
    }
  }
  else
  {
    importDefaultSettings (&eth0_new_if, &eth1_new_if, eth0PingAddress, eth1PingAddress);
  }

  ret = setInterfaceInfo (&eth0_new_if);
  if (ret != 0)
  {
    return (ftUpdateTestStatus(ftrp,ftError,NULL));
  }

  printEthInfo (&eth0_new_if);

  ret = setInterfaceInfo (&eth1_new_if);
  if (ret != 0)
  {
    return (ftUpdateTestStatus(ftrp,ftError,NULL));
  }

  printEthInfo (&eth1_new_if);

  fitPrint(VERBOSE, "\n");

  if (if_isUp (eth0_new_if.if_name) == 0)
  {
    (void)if_setUp (eth0_new_if.if_name);
  }

  if (if_isUp (eth1_new_if.if_name) == 0)
  {
    (void)if_setUp (eth1_new_if.if_name);
  }

  // Try to ping out both interfaces
  eth0Fail = ping (eth0PingAddress);
  eth1Fail = ping (eth1PingAddress);

  // ifdown both interfaces we created (eth0:5 and eth1:5)
  (void)if_down (eth0_new_if.if_name);
  (void)if_down (eth1_new_if.if_name);

  // And conditionally ifdown any other interfaces we had to bring up
  // (i.e. "eth1" instead of just "eth1:5")
  (void)if_setDown ();

  if (eth0Fail != 0)
  {
    if (eth1Fail != 0)
    {
      strcpy (errorStr, "eth0 & eth1");
    }
    else
    {
      strcpy (errorStr, "eth0");
    }
  }
  else if (eth1Fail != 0)
  {
    strcpy (errorStr, "eth1");
  }
  else
  {
    /* No Error */
    errorStr[0] = '\0';
  }

  return (ftUpdateTestStatus(ftrp,ftComplete, errorStr));
}
