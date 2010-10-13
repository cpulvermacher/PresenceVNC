/*
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * listen.c - listen for incoming connections
 */

#ifdef __STRICT_ANSI__
#define _BSD_SOURCE
#endif
#include <unistd.h>
#include <sys/types.h>
#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <sys/wait.h>
#include <sys/utsname.h>
#endif
#include <sys/time.h>
#include <rfb/rfbclient.h>

/*
 * listenForIncomingConnections() - listen for incoming connections from
 * servers, and fork a new process to deal with each connection.
 */

void
listenForIncomingConnections(rfbClient* client)
{
#ifdef __MINGW32__
  /* FIXME */
  rfbClientErr("listenForIncomingConnections on MinGW32 NOT IMPLEMENTED\n");
  return;
#else
  int listenSocket;
  fd_set fds;

  client->listenSpecified = TRUE;

  listenSocket = ListenAtTcpPort(client->listenPort);

  if ((listenSocket < 0))
    return;

  rfbClientLog("Listening on port %d\n",
	  client->listenPort);
  rfbClientLog("Command line errors are not reported until "
	  "a connection comes in.\n");

  while (client->listenSpecified) {

    /* reap any zombies */
    int status, pid;
    while ((pid= wait3(&status, WNOHANG, (struct rusage *)0))>0);

    /* TODO: callback for discard any events (like X11 events) */

    FD_ZERO(&fds); 

    FD_SET(listenSocket, &fds);

	//100ms
    struct timeval timeout;
    timeout.tv_sec=0;
    timeout.tv_usec=100000;

    select(FD_SETSIZE, &fds, NULL, NULL, &timeout);

    if (FD_ISSET(listenSocket, &fds)) {
      client->sock = AcceptTcpConnection(listenSocket);
      if (client->sock < 0)
	return;
      if (!SetNonBlocking(client->sock))
	return;

	//modified to accept only a single connection
	//if something goes wrong, we can always create another listening socket
	rfbClientLog("Accepted connection.\n");
	break;

	}
  }

  close(listenSocket);
#endif
}


