/*
 * Copyright (C) 1992 by Rush Record
 * Copyright (C) 1993 by Charles Sandmann (sandmann@clio.rice.edu)
 * 
 * This file is part of ED.
 * 
 * ED is free software; you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation.
 * 
 * ED is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with ED
 * (see the file COPYING).  If not, write to the Free Software Foundation, 675
 * Mass Ave, Cambridge, MA 02139, USA.
 */
#include "opsys.h"

#ifndef NO_NEWS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef VMS

#ifdef MULTINET
#include "multinet_root:[multinet.include.sys]types.h"
#include "multinet_root:[multinet.include.sys]socket.h"
#include "multinet_root:[multinet.include.netinet]in.h"
#include "multinet_root:[multinet.include]netdb.h"
#define read socket_read
#define write socket_write
#define close socket_close
#endif

#ifdef WOLLONGONG
#include <in.h>
#include <netdb.h>
#include <socket.h>
#include <inet.h>
#define read netread
#define write netwrite
#define close netclose
#endif

#ifdef UCX
#include <in.h>
#include <netdb.h>
#include <socket.h>
#include <inet.h>
#endif

#else

#include <setjmp.h>
#include <netdb.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#endif

static Int inputchan = 0,outputchan = 0,controls;
static FILE *inputfp = NULL;
static struct sockaddr_in controlsocket;
static struct hostent *host_info_ptr;
static struct servent *svc_info;
static short port;
static Int posting_ok = 0;
#ifdef DEBUG
static FILE *dfp;
#endif

/******************************************************************************\
|Routine: posting_allowed
|Callby: edit
|Purpose: Returns 1 if posting is allowed, else 0.
|Arguments:
|    none
\******************************************************************************/
Int posting_allowed()
{
	return(posting_ok);
}

/******************************************************************************\
|Routine: news_close
|Callby: news_open
|Purpose: Closes connection to news server.
|Arguments:
|    none
\******************************************************************************/
void news_close()
{
	if(inputchan)
	{
		close(inputchan);
/*		fclose(inputfp);*/
		inputchan = 0;
	}
	if(outputchan)
	{
		close(outputchan);
		outputchan = 0;
	}
	if(controls)
	{
		close(controls);
		controls = 0;
	}
}

/******************************************************************************\
|Routine: news_command
|Callby: load_news
|Purpose: Sends a command to the news server. Returns 0 if error, else 1.
|Arguments:
|    buf is a string containing the command.
\******************************************************************************/
Int news_command(buf)
Char *buf;
{
	static Char *term = "\r\n";
	
	if(!outputchan)
		return(0);
#ifdef DEBUG
	if(!dfp)
		dfp = fopen("nntp.dat","w");
	fprintf(dfp,"send:%s\n",buf);
	fflush(dfp);
#endif
	if(write(outputchan,buf,strlen(buf)) != strlen(buf))
		return(0);
	if(write(outputchan,term,strlen(term)) != strlen(term))
		return(0);
	return(1);
}

/******************************************************************************\
|Routine: news_response
|Callby: load_news news_open
|Purpose: Reads a response from the news server. Returns the length of the
|         response, or -1 if the connection wasn't open.
|Arguments:
|    buf is the buffer to store the response in.
|    size is the size of buf.
\******************************************************************************/
Int news_response(buf,size)
Char *buf;
Int size;
{
#ifdef VMS
	static Int cur = 0,remain = 0;
	static Char vbuf[512];
	Int bufptr;
#endif
	
	buf[0] = '\0';
	if(!inputchan)
		return(-1);
#ifdef VMS
	bufptr = 0;
again:
	if(!remain)
	{
		while(!remain)
			remain = read(inputchan,vbuf,sizeof(vbuf));
		cur = 0;
	}
	while(remain--)
	{
		if((buf[bufptr++] = vbuf[cur++]) == '\n')
		{
			buf[bufptr - 2] = '\0';
			return(strlen(buf));
		}
	}
	remain = 0;
	goto again;
#else
	fgets(buf,size,inputfp);
	*(strchr(buf,'\r')) = '\0';
#ifdef DEBUG
	if(!dfp)
		dfp = fopen("nntp.dat","w");
	fprintf(dfp,"response:%s\n",buf);
	fflush(dfp);
#endif
#endif	/* VMS */
	return(strlen(buf));
}

/******************************************************************************\
|Routine: news_open
|Callby: load_news
|Purpose: Opens a connection to the news server. Returns 0 for error, else 1.
|Arguments:
|    server is the name of the news server.
|    title is the server's connect greeting, which becomes the window title.
|    size is the size of title.
\******************************************************************************/
Int news_open(server,title,size)
Char *server,*title;
Int size;
{
	Char buf[512],*from,*to;
	Int i;

	news_close();
/* get host info */
	if(!(host_info_ptr = gethostbyname(server)))
	{
		sprintf(buf,"Unknown news server: %s",server);
		slip_message(buf);
		goto abort;
	}
/* get info about service (this should be called only once) */
	if(!port)
	{
#ifdef _AIX
		port = htons(119);
#else
#ifdef UCX
		if((svc_info = getservbyname("nntp","tcp")) == (struct servent *)(-1))
#else
		if(!(svc_info = getservbyname("nntp","tcp")))
#endif
			port = htons(119);	/* use port 119 if getservbyname fails */
		else
			port = svc_info->s_port;
#endif
	}
	memset(&controlsocket,0,sizeof(controlsocket));
	memcpy((char *)&controlsocket.sin_addr,host_info_ptr->h_addr,host_info_ptr->h_length);
	controlsocket.sin_family = host_info_ptr->h_addrtype;
	controlsocket.sin_port = port;
/* create control path */
	if((controls = socket(host_info_ptr->h_addrtype,SOCK_STREAM,0)) < 0)
	{
		slip_message("Error creating socket for control path.");
		goto abort;
	}
loop:
	if(connect(controls,(struct sockaddr *)&controlsocket,sizeof(controlsocket))== -1)
	{
		if(errno == EINTR)
			goto loop;
		slip_message("Control connection failed.");
		goto abort;
	}
	i = sizeof(controlsocket);
	if(getsockname(controls,(struct sockaddr *)&controlsocket,&i) < 0)
	{
		slip_message("Couldn't get control socket name.");
		goto abort;
	}
	inputchan = controls;
	inputfp = fdopen(inputchan,"r");
	outputchan = dup(controls);
	news_response(title,size);	/* get the greeting, make it the buffer title */
	if(title[0] != '2')
	{
		slip_message(title);
		goto abort;
	}
	if(title[2] == '0')
		posting_ok = 1;
	else
		posting_ok = 0;
	for(from = title + 4,to = title;(*to++ = *from++););	/* shift out the response code */
	return(1);
abort:
	wait_message();
	news_close();
	return(0);
}
#endif

