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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef NO_FTP
#include "unistd.h"
#include "memory.h"
#include "file.h"
#include <signal.h>
#include <errno.h>

#ifdef VMS
#define NO_FTP_FILE		/* CWS - 11/93 */

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

#ifdef WIN32
#define NO_FTP_FILE
#include <winsock.h>
#define read(a,b,c) recv(a,b,c,0)
#define write(a,b,c) send(a,b,c,0)
#define close closesocket
#define errno WSAGetLastError()
#define signal(a,b)
static Char WSAInited = 0;
#else

#include <setjmp.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif
#endif

#include "ctyp_dec.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"

static Int inputchan = 0,outputchan = 0;
#ifndef NO_FTP_FILE
static FILE *inputfp = NULL;
#endif
static struct sockaddr_in controlsocket,datasocket;
static Int controls,datas = 0,newdatas = 0;
static Char curhost[64],curuser[64],curpass[64],curdir[128];
static Char respbuf[512];
static Int pipe_broken = 0;
#ifdef VMS
static Char systemtype = 'V';
#else
static Char systemtype = 'U';
#endif
static short ftpport = 0;
static Int ascii_nbytes = 0;
#ifdef DEBUG
static FILE *dfp = NULL;
#endif

#ifndef VMS
extern char *sys_errlist[];
#endif
extern Char *netrc_password();

#else	/* NO_FTP */
/******************************************************************************\
|Routine: ftp_system
|Callby: edit load_file
|Purpose: Returns U for unix server, M for msdos, W for WNT, O for os/2, and
|         V for VMS system.
|Arguments:
|    none
\******************************************************************************/
Int ftp_system(file)
Char *file;
{
#ifdef VMS
	return('V');
#else
#ifdef GNUOS2
	return('O');
#else
#ifdef GNUDOS
	return('M');
#else
#ifdef WNT
	return('W');
#else
	return('U');
#endif	/* WNT */
#endif	/* GNUDOS */
#endif	/* GNUOS2 */
#endif	/* VMS */
}
#endif	/* NO_FTP */

#ifndef NO_FTP
/******************************************************************************\
|Routine: ftp_close
|Callby: ftp
|Purpose: Cleans up when an error occurs.
|Arguments:
|    none
\******************************************************************************/
void ftp_close()
{
	if(inputchan)
	{
		close(inputchan);
#ifndef NO_FTP_FILE
		fclose(inputfp);
#endif
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
	if(datas)
	{
		close(datas);
		datas = 0;
	}
	if(newdatas)
	{
		close(newdatas);
		newdatas = 0;
	}
	curhost[0] = '\0';	/* prevent assumption of an open connection */
	respbuf[0] = '\0';	/* prevent subsequent failure from reporting old error message */
	signal(SIGPIPE,SIG_DFL);
}

/******************************************************************************\
|Routine: ftp_broken
|Callby: signal mechanism
|Purpose: Handles SIGPIPE while FTP connections are open.
|Arguments:
|    none
\******************************************************************************/
void ftp_broken(i)
Int i;
{
#ifdef DEBUG
	fprintf(dfp,"ftp_broken was called.\n");
	fflush(dfp);
#endif
	ftp_close();
	pipe_broken = 1;
	return;
}

/******************************************************************************\
|Routine: ftp_reopen
|Callby: ftp_command
|Purpose: Reopens a connection that has been closed by the server.
|Arguments:
|    none
\******************************************************************************/
Int ftp_reopen()
{
	Char buf[512],chost[512],cuser[512];
	
#ifdef DEBUG
	fprintf(dfp,"Trying to reopen the connection...\n");
	fflush(dfp);
#endif
	strcpy(chost,curhost);
	strcpy(cuser,curuser);
	ftp_close();
	if(!ftp_open(chost,cuser))
		return(0);
	sprintf(buf,"CWD %s",curdir);
	if(!ftp_command(buf))
		return(0);
	return(1);
}

/******************************************************************************\
|Routine: ftp_abort
|Callby: load_file
|Purpose: Aborts FTP transfer.
|Arguments:
|    none
\******************************************************************************/
void ftp_abort()
{
	Char buf[512];

/*
      By convention the sequence [IP, Synch] is to be used as such a
      signal.  For example, suppose that some other protocol, which uses
      TELNET, defines the character string STOP analogously to the
      TELNET command AO.  Imagine that a user of this protocol wishes a
      server to process the STOP string, but the connection is blocked
      because the server is processing other commands.  The user should
      instruct his system to:

         1. Send the TELNET IP character;

         2. Send the TELNET SYNC sequence, that is:

            Send the Data Mark (DM) as the only character
            in a TCP urgent mode send operation.

         3. Send the character string STOP; and

         4. Send the other protocol's analog of the TELNET DM, if any.

      The user (or process acting on his behalf) must transmit the
      TELNET SYNCH sequence of step 2 above to ensure that the TELNET IP
      gets through to the server's TELNET interpreter.

   The following are the defined TELNET commands.  Note that these codes
   and code sequences have the indicated meaning only when immediately
   preceded by an IAC.

      NAME               CODE              MEANING

      SE                  240    End of subnegotiation parameters.
      NOP                 241    No operation.
      Data Mark           242    The data stream portion of a Synch.
                                 This should always be accompanied
                                 by a TCP Urgent notification.
      Break               243    NVT character BRK.
      Interrupt Process   244    The function IP.
After the abort, we get from server either success, or 426 err followed by success.
*/
	if(newdatas)
	{
#ifdef DEBUG
		fprintf(dfp,"Aborting transfer.\n");
		fflush(dfp);
#endif
		sprintf(buf,"%c%c",255,244);	/* this is the IP command */
		write(outputchan,buf,strlen(buf));
		sprintf(buf,"%c%c",255,242);	/* this is the DM command */
		send(outputchan,buf,2,MSG_OOB);
		sprintf(buf,"ABOR\r\n");	/* this is the ABORT command */
		write(outputchan,buf,strlen(buf));
		do
		{
			read_resp(buf);	/* there may be one or two responses, depending on whether the transfer just completed */
#ifdef DEBUG
			fprintf(dfp,"abort response:%s\n",buf);
			fflush(dfp);
#endif
		} while(strncmp(buf,"226",strlen("226")));
		close(newdatas);
		newdatas = 0;
		ascii_nbytes = 0;	/* so the next transferred file doesn't appear to contain data from this file */
		bell();
	}
}

/******************************************************************************\
|Routine: ftp_system
|Callby: edit load_file
|Purpose: Returns U for unix server and V for VMS server.
|Arguments:
|    none
\******************************************************************************/
Int ftp_system(file)
Char *file;
{
	if(host_in_name(file))
		return(systemtype);
#ifdef VMS
	return('V');
#else
#ifdef GNUOS2
	return('O');
#else
#ifdef GNUDOS
	return('M');
#else
#ifdef WNT
	return('W');
#else
	return('U');
#endif	/* WNT */
#endif	/* GNUDOS */
#endif	/* GNUOS2 */
#endif	/* VMS */
}

/******************************************************************************\
|Routine: last_message
|Callby: read_in_diredit
|Purpose: Returns the last message from the FTP server.
|Arguments:
|    none
\******************************************************************************/
Char *last_message()
{
	return(respbuf);
}

/******************************************************************************\
|Routine: dump_message
|Callby: ftp_binary ftp_put_binary ftp_put_record ftp_record
|Purpose: Dumps the "transfer complete" message.
|Arguments:
|    none
\******************************************************************************/
void dump_message()
{
	Char resp[4],buf[256];

	read_resp(buf);
	if(buf[3] == '-')
	{
		memcpy(resp,buf,3);
		while(1)
		{
			if(!strncmp(buf,resp,3))
				if(buf[3] == ' ')
					break;
			if(read_resp(buf) < 0)
				break;
		}
	}
}
	
/******************************************************************************\
|Routine: ftp_cwd
|Callby: load_file
|Purpose: Returns the current directory, necessary if server is VMS.
|Arguments:
|    none
\******************************************************************************/
Char *ftp_cwd()
{
	return(curdir);
}

/******************************************************************************\
|Routine: read_resp
|Callby: ftp_open open_data_path
|Purpose: Reads a response from the control path (terminated by CRLF). Returns
|         the number of bytes in the response.
|Arguments:
|    buf is the returned response from the ftp server.
\******************************************************************************/
Int read_resp(buf)
Char *buf;
{
	Char *p;
	
	p = buf;
	*p = '\0';
	if(!inputchan)
		return(-1);
#ifndef NO_FTP_FILE
	if(!fgets(buf,512,inputfp))
		return(-1);
	return(strlen(buf));
#else
	while(1)
	{
		if(read(inputchan,p,1) != 1)
			return(-1);
		if(*p++ == '\n')
		{
			*p++ = '\0';
			return(p - buf);
		}
	}
#endif
}

/******************************************************************************\
|Routine: ftp_error
|Callby: ftp
|Purpose: Reports errors involving sockets.
|Arguments:
|    string is the error message.
\******************************************************************************/
Int ftp_error(string)
Char *string;
{
	Char buf[512],*errmsg;
	
	sprintf(buf,"%s, errno=%d",string,errno);
	slip_message(respbuf);
	ftp_close();
	slip_message(buf);
#ifndef VMS
	errmsg = (Char *)sys_errlist[errno];
#else
    errmsg = strerror(errno,vaxc$errno);
#endif
	slip_message(errmsg);
	wait_message();
	return(0);
}

/******************************************************************************\
|Routine: open_data_path
|Callby: ftp_data
|Purpose: Establishes a data connection for file transfer, directory listing.
|Arguments:
|    none
\******************************************************************************/
Int open_data_path()
{
	Int i;
	Uchar *address,*port;
	Char buf[256];
	
	if((datas = socket(AF_INET,SOCK_STREAM,0)) < 0)
		return(ftp_error("Couldn't create socket"));
	datasocket = controlsocket;	/* copy everything from control socket */
	datasocket.sin_port = 0;
	if(bind(datas,(struct sockaddr *)&datasocket,sizeof(datasocket)) < 0)
		return(ftp_error("Couldn't bind socket"));
	i = sizeof(datasocket);
	if(getsockname(datas,(struct sockaddr *)&datasocket,&i) < 0)
		return(ftp_error("Couldn't get socket name"));
	if(listen(datas,1))	/* listen for connection from server */
		return(ftp_error("Couldn't listen on data socket"));
	address = (Uchar *)&datasocket.sin_addr;	/* send PORT command to server */
	port = (Uchar *)&datasocket.sin_port;
	sprintf(buf,"PORT %d,%d,%d,%d,%d,%d\r\n",address[0],address[1],address[2],address[3],port[0],port[1]);
open_retry:
	if(!outputchan || write(outputchan,buf,strlen(buf)) <= 0)
	{
		if(pipe_broken)
			if(ftp_reopen())
				goto open_retry;
		return(ftp_error("Couldn't send PORT command"));
	}
	if(read_resp(buf) < 0)
		return(0);	/* read response to port command */
	return(datas);
}

/******************************************************************************\
|Routine: ftp_open
|Callby: ftp load_file output_file
|Purpose: Sets up the control connection for ftp. Returns 0 for failure, else 1.
|Arguments:
|    host is the host on which the server is to run.
|    user is the username to use.
\******************************************************************************/
Int ftp_open(host,user)
Char *host,*user;
{
	Int i;
	Char buf[256],passw[64],*p,*q,resp[4],initrespbuf[512];
	struct hostent *host_info_ptr;

#ifdef WIN32
	if(!WSAInited)
	{
		WSADATA WSAData;
		Int status;

		if((status = WSAStartup(MAKEWORD(1,1), &WSAData)))
		{
			sprintf(buf,"Error opening WinSock library: %d",status);
			slip_message(buf);
			wait_message();
			return(0);
		}
		WSAInited = 1;
	}
#endif

#ifdef DEBUG
	if(!dfp)
		dfp = fopen("ftp.dat","w");
	fprintf(dfp,"ftp_open(%s,%s)\n",host,user);
	fflush(dfp);
#endif
	pipe_broken = 0;	/* this is a flag that the remote server has timed out the connection */
	if(!strcmp(host,curhost) && !strcmp(user,curuser))	/* if already connected, continue (blithely) */
		return(1);
	if(inputchan && outputchan)	/* terminate existing connection */
	{
		ftp_command("ABOR");
		ftp_command("QUIT");
		ftp_close();
	}
/* get info about service (this only need be called only once) */
	if(!ftpport)
		ftpport = htons(21);	/* CWS 11-93 */
/* get host info */
	memset(&controlsocket,0,sizeof(controlsocket));
	if((controlsocket.sin_addr.s_addr = inet_addr((char *)host)) != -1)	/* check for numeric address */
		controlsocket.sin_family = AF_INET;
	else	/* non-numeric address */
	{
		if(!(host_info_ptr = gethostbyname(host)))
		{
			sprintf(buf,"'%s' is an unknown host.",host);
			slip_message(buf);
			wait_message();
			return(0);
		}
		controlsocket.sin_family = host_info_ptr->h_addrtype;
		memcpy(&controlsocket.sin_addr,host_info_ptr->h_addr_list[0],host_info_ptr->h_length);
	}
	controlsocket.sin_port = ftpport;
/* create socket */
	if((controls = socket(controlsocket.sin_family,SOCK_STREAM,0)) < 0)
		return(ftp_error("Error creating socket for control path"));
/* connect to ftp service */
	while(1)
		if(connect(controls,(struct sockaddr *)&controlsocket,sizeof(controlsocket)) != -1)
			break;
		else if(errno != EINTR)
			return(ftp_error("Control connection failed"));
/* prepare for server closing connection abruptly */
	signal(SIGPIPE,ftp_broken);
/* retrieve complete socket info */
	i = sizeof(controlsocket);
	if(getsockname(controls,(struct sockaddr *)&controlsocket,&i) < 0)
		return(ftp_error("Couldn't get control socket name"));
	outputchan = inputchan = controls;
#ifndef NO_FTP_FILE
	inputfp = fdopen(inputchan,"r");
#endif
/* read the connect message(s) */
	if(read_resp(buf) < 0)	/* read the connect message */
		return(ftp_error("Couldn't connect to the FTP server"));
/* check for sparc error message */
	if(!strncmp(buf,"ld.so",5))
		read_resp(buf);
/* check for immediate rejection */
	if(buf[0] > '3')
	{
		slip_message("Unable to do FTP with that host, it said:");
		if(buf[3] == '-')
		{
			memcpy(resp,buf,3);
			while(1)
			{
				if((p = strchr(buf,'\r')))
					*p = '\0';
				if((p = strchr(buf,'\n')))
					*p = '\0';
				slip_message(buf);
				if(!strncmp(buf,resp,3))
					if(buf[3] == ' ')
						break;
				if(read_resp(buf) < 0)
					break;
			}
		}
		else
		{
			if((p = strchr(buf,'\r')))
				*p = '\0';
			if((p = strchr(buf,'\n')))
				*p = '\0';
			slip_message(buf);
		}
		wait_message();
		ftp_close();
		return(0);
	}
	strcpy(initrespbuf,buf);	/* save so we can check for FTPSERV below */
/* handle multi-line greeting */
	if(buf[3] == '-')
	{
		memcpy(resp,buf,3);
		while(1)
		{
			if(!strncmp(buf,resp,3))
				if(buf[3] == ' ')
					break;
			if(read_resp(buf) < 0)
				return(ftp_error("Couldn't connect to the FTP server"));
		}
	}
/* log in to the host */
	sprintf(buf,"USER %s",user);
	if(!ftp_command(buf))
	{
		slip_message(respbuf);
		ftp_close();
		wait_message();
		return(0);
	}
	if(respbuf[0] == '3')
	{
/* deal with passwords */
		if(strlen(p = netrc_password()))	/* get password from .netrc if possible */
			strcpy(passw,p);
		else if(!strcmp(user,"anonymous"))
		{
			gethostname(buf,sizeof(buf));
/*			if(!strchr(buf,'.'))  then add domain... */
#ifdef UCX
			if((p = getenv("UCX$BIND_DOMAIN")))
			{
				strcat(buf,".");
				strcat(buf,p);
			}
#endif
#ifdef WOLLONGONG
			if((p = getenv("INET_DOMAIN_NAME")))
			{
				strcat(buf,".");
				strcat(buf,p);
			}
#endif

#ifdef CUSERID_ENV
			sprintf(passw,"%s@%s",getenv(CUSERID_ENV),buf);
#else
			sprintf(passw,"%s@%s",cuserid(NULL),buf);
#endif
		}
		else
		{
			inquire_hidden(1);	/* prompt the user for the password */
			inquire("Password",passw,sizeof(passw),1);
			inquire_hidden(0);
		}
		sprintf(buf,"PASS %s",passw);
		if(!ftp_command(buf))
			goto badlogin;
	}
/* determine the system type */
	if(!ftp_command("SYST"))
	{
		if(!ftp_command("PWD"))
			if(!ftp_command("XPWD"))
				goto badlogin;
		if(strstr(respbuf,"defined"))
			systemtype = 'I';
		else if(strchr(respbuf,'/'))
			systemtype = 'U';
		else
			systemtype = 'V';
	}
	else
	{
		if(respbuf[4] == '\'')
			systemtype = 'U';
		else
		{
			if(respbuf[4] == 'V')
			{
				if(respbuf[6] == 'S')
					systemtype = 'V';
				else
					systemtype = 'I';	/* this is a VM system, pretend it's a VMS system */
			}
			else if(!strncmp(respbuf + 4,"MTS",strlen("MTS")))
				systemtype = 'T';
			else if(!strncmp(initrespbuf + 4,"FTPSERV",strlen("FTPSERV")))
				systemtype = 'I';
			else
				systemtype = respbuf[4];
		}
	}
/* make AIX look like UNIX */
	if(systemtype == 'A')
		systemtype = 'U';
/* get the current directory */
	if(!ftp_command("PWD"))
		if(!ftp_command("XPWD"))
			goto badlogin;
	if(strstr(respbuf,"defined"))
		curdir[0] = '\0';
	else
	{
		strcpy(curdir,respbuf + 4);
		if(systemtype == 'T')	/* MTS reports cwd as DISKNAME.: */
		{

			if(curdir[0] == '"')	/* strip quotes if present */
			{
				for(p = curdir + 1;*p != '"';p++)
					*(p - 1) = *p;
				*p = '\0';
			}
			if((p = strchr(curdir,':')))
				if(*(p - 1) == '.')
					p--;
			*p = '\0';
			sprintf(buf,"[%s]",curdir);
			strcpy(curdir,buf);
		}
		else
		{
			for(p = curdir;!isspace(*p);p++);	/* trim trailing space */
			*p = '\0';
			if(curdir[0] == '"')	/* strip quotes if present */
			{
				for(q = curdir,p = q + 1;*p != '"';*q++ = *p++);
				*q++ = '\0';
			}
			if(systemtype == 'I' && curdir[0] != '[')	/* make VM look like VMS */
			{
				sprintf(buf,"[%s]",curdir);
				strcpy(curdir,buf);
			}
		}
	}
/* save params for comparison on next try */
	strcpy(curhost,host);
	strcpy(curuser,user);
	strcpy(curpass,passw);
	return(1);
/* a problem logging in */
badlogin:
	if((p = strchr(respbuf,'\r')))
		*p = '\0';
	if((p = strchr(respbuf,'\n')))
		*p = '\0';
	slip_message(respbuf);
	ftp_close();
	wait_message();
	return(0);
}

/******************************************************************************\
|Routine: ftp_command
|Callby: ftp load_file output_file
|Purpose: Sends a command to the ftp server, and gets the response.
|Arguments:
|    cmd is the command to send.
\******************************************************************************/
Int ftp_command(cmd)
Char *cmd;
{
	static Int recurse = 0;	/* flags a recursive call */
	static Char *retrypass = "pass rhr";

	Int l;
	Char resp[4],buf[256],chost[256],cuser[256],*p;
	
#ifdef DEBUG
	if(!dfp)
		dfp = fopen("ftp.dat","w");
	fprintf(dfp,"ftp_command(%s)\n",cmd);
	fflush(dfp);
#endif
/* be sure the channels are open */
	if(!inputchan)
		return(0);
/* send the command */
retry:
	strcpy(respbuf,cmd);
	strcat(respbuf,"\r\n");
	if((l = strlen(respbuf)))
		if(!outputchan || write(outputchan,respbuf,l) <= 0)
		{
/* try to reopen the connection */
			if(pipe_broken)
				if(ftp_reopen())
					goto retry;
			return(ftp_error("It appears the remote host has closed the connection"));
		}
/* get the response */
	if(read_resp(respbuf) < 0)
	{
#ifdef DEBUG
		fprintf(dfp,"read_resp response:%s\n",respbuf);
		fflush(dfp);
#endif
		strcpy(chost,curhost);
		strcpy(cuser,curuser);
		ftp_close();
		if(!(ftp_open(chost,cuser)))
			return(0);
		goto retry;
	}
#ifdef DEBUG
	fprintf(dfp,"response:%s\n",respbuf);
	fflush(dfp);
#endif
/* remove \r\n if it's there */
	if((p = strchr(respbuf,'\r')))
		*p = '\0';
	if((p = strchr(respbuf,'\n')))
		*p = '\0';
/* handle server timeout */
	if(!strncmp(respbuf,"421",3))
	{
#ifdef DEBUG
		fprintf(dfp,"server timeout\n");
		fflush(dfp);
#endif
		if(!ftp_reopen())
			return(ftp_error("It appears the remote host has closed the connection"));
		goto retry;
	}
/* this is for servers that do not report nnn at the beginning of each record */
	if(my_sscanf(respbuf,"%d",&l) != 1)
	{
		read_resp(respbuf);
		ftp_close();
		return(0);
	}
	if(!strncmp(cmd,"PASS",4) && strstr(respbuf,"BAD SYNTAX"))
	{
		cmd = retrypass;
		goto retry;
	}
	if(!recurse && !strncmp(respbuf,"530",3) && strlen(curuser) && strncmp(cmd,"PASS",4))	/* user no longer logged in, relogin */
	{
#ifdef DEBUG
		fprintf(dfp,"reopening connection with host = %s, user = %s\n",curhost,curuser);
		fflush(dfp);
#endif
		recurse = 1;	/* prevent infinite recursion */
		sprintf(buf,"USER %s",curuser);
		if(!ftp_command(buf))
		{
			recurse = 0;
			ftp_close();
			return(0);
		}
		sprintf(buf,"PASS %s",curpass);
		if(!ftp_command(buf))
		{
			recurse = 0;
			ftp_close();
			return(0);
		}
		sprintf(buf,"CWD %s",curdir);
		if(!ftp_command(buf))
		{
			recurse = 0;
			ftp_close();
			return(0);
		}
		recurse = 0;
/* send whatever command they wanted to send, again */
		strcpy(respbuf,cmd);
		strcpy(respbuf + strlen(respbuf),"\r\n");
		if((l = strlen(respbuf)))
			if(!outputchan || write(outputchan,respbuf,l) <= 0)
				return(ftp_error("I'm unable to send a command to the remote host"));
		if(read_resp(respbuf) < 0)
			return(0);
	}
/* handle multi-line response */
	if(respbuf[3] == '-')
	{
		memcpy(resp,respbuf,3);
		while(1)
		{
			if(!strncmp(respbuf,resp,3))
				if(respbuf[3] == ' ')
					break;
			if(read_resp(respbuf) < 0)
				return(0);
#ifdef DEBUG
			fprintf(dfp,"continue:%s\n",respbuf);
			fflush(dfp);
#endif
		}
	}
/* if it's a CWD command, save the directory name */
	l = (respbuf[0] <= '3')? 1 : 0;
	if(!strncmp(cmd,"CWD ",4) && l)
		strcpy(curdir,cmd + 4);
	return(l);
}

/******************************************************************************\
|Routine: ftp_data
|Callby: load_file output_file
|Purpose: Sends a command that requires a data connection (LIST, NLST, RETR).
|Arguments:
|    cmd is the command to send.
\******************************************************************************/
Int ftp_data(cmd)
Char *cmd;
{
	Char lcmd[256],*p;
	Int i;
	
	if(!(datas = open_data_path()))	/* open data path to server */
		return(0);
	strcpy(lcmd,cmd);	/* send command to server */
	strcat(lcmd,"\r\n");
retry_data:
	if(!outputchan || write(outputchan,lcmd,strlen(lcmd)) <= 0)
	{
		if(pipe_broken)
			if(ftp_reopen())
				goto retry_data;
		return(ftp_error("The remote host appears to have closed the connection"));
	}
#ifdef DEBUG
	fprintf(dfp,"data mode command:%s\n",cmd);
	fflush(dfp);
#endif
	if(read_resp(lcmd) < 0)
	{
		sprintf(lcmd,"Unable to execute command '%s'",cmd);
		return(ftp_error(lcmd));
	}
	if(lcmd[0] > '3')	/* we sort of pray here that server that has more to say shuts up nicely */
	{
		if((p = strchr(lcmd,'\r')))
			*p = '\0';
		if((p = strchr(lcmd,'\n')))
			*p = '\0';
		slip_message(lcmd);
		wait_message();
		return(0);
	}
/* set up socket for transfer, blocking in accept() until the server initiates the connection */
	i = sizeof(datasocket);
	if((newdatas = accept(datas,(struct sockaddr *)&datasocket,&i)) < 0)
	{
		newdatas = 0;
		return(ftp_error("Couldn't accept connection"));
	}
	close(datas);
	datas = 0;
	return(1);
}
	
/******************************************************************************\
|Routine: ftp_record
|Callby: load_file
|Purpose: Reads a record from the data path.
|Arguments:
|    rec is the returned record.
\******************************************************************************/
Int ftp_record(rec)
Char *rec;
{
	static Char buffer[128],*p;
	Char *save_rec;
	
	if(!newdatas)
		return(0);
	save_rec = rec;
	while(1)
	{
		if(ascii_nbytes-- > 0)
		{
			if((*rec++ = *p++) == '\n')
			{
				*(rec - 2) = '\0';
#ifdef DEBUG
				fprintf(dfp,"ftp_record gets:%s\n",save_rec);
				fflush(dfp);
#endif
				return(1);
			}
		}
		else
		{
			if((ascii_nbytes = read(newdatas,buffer,sizeof(buffer))) <= 0)
			{
				if(rec != save_rec)	/* we have a faulty file */
				{
					*p++ = '\n';
					*p++ = '\0';
#ifdef DEBUG
					fprintf(dfp,"ftp_record gets(faulty):%s\n",save_rec);
					fflush(dfp);
#endif
					return(1);
				}
				close(newdatas);
				newdatas = 0;
				dump_message();	/* dump the "transfer complete" message */
				return(0);
			}
			p = buffer;
		}
	}
}
	
/******************************************************************************\
|Routine: ftp_ascii
|Callby: load_file
|Purpose: Reads a record from the data path.
|Arguments:
|    rec is the returned record.
|    len is the length of rec in bytes.
\******************************************************************************/
Int ftp_ascii(rec,len)
Char *rec;
Int len;
{
	static Char buffer[128],*p,*stop;
	Char *save_rec;
	
	if(!newdatas)
		return(0);
	save_rec = rec;
	stop = rec + len - 1;	/* last byte of buffer */
	while(1)
	{
		if(rec == stop)	/* the buffer is full, break the record here */
		{
			*stop = '\0';
#ifdef DEBUG
			fprintf(dfp,"ftp_ascii buffer fills\n");
			fflush(dfp);
#endif
			return(1);
		}
		if(ascii_nbytes-- > 0)
		{
			if((*rec++ = *p++) == '\n')	/* we have a record termination */
			{
				if(rec - save_rec < 2)	/* if it's just \n, return zero-length string */
					*save_rec = '\0';
				else
				{
					if(*(rec - 2) == '\r')
						*(rec - 2) = '\0';
					else
						*--rec = '\0';
				}
#ifdef DEBUG
				fprintf(dfp,"ftp_ascii gets:%s\n",save_rec);
				fflush(dfp);
#endif
				return(1);
			}
		}
		else
		{
			if((ascii_nbytes = read(newdatas,buffer,sizeof(buffer))) <= 0)
			{
				if(rec != save_rec)	/* we have a faulty file */
				{
					*rec++ = '\n';
					*rec++ = '\0';
#ifdef DEBUG
					fprintf(dfp,"ftp_record gets(faulty):%s\n",save_rec);
					fflush(dfp);
#endif
					return(1);
				}
				close(newdatas);
				newdatas = 0;
				dump_message();	/* dump the "transfer complete" message */
				return(0);
			}
			p = buffer;
		}
	}
}
	
/******************************************************************************\
|Routine: ftp_binary
|Callby: load_file
|Purpose: Reads a record from the data path, in binary mode.
|Arguments:
|    rec is the returned (usually 512-byte) record.
\******************************************************************************/
Int ftp_binary(rec)
Char *rec;
{
	Int l;
	
	if(!newdatas)
		return(0);
	if((l = read(newdatas,rec,512)) <= 0)
	{
		close(newdatas);
		newdatas = 0;
		dump_message();	/* dump the "transfer complete" message */
		return(0);
	}
	return(l);
}

/******************************************************************************\
|Routine: ftp_put_record
|Callby: read_in_diredit
|Purpose: Writes a record to the server.
|Arguments:
|    rec is the record to send. rec = NULL means write EOF.
\******************************************************************************/
Int ftp_put_record(rec)
Char *rec;
{
	Int l;
	
	if(!rec)	/* NULL argument signifies end-of-file */
	{
		close(newdatas);
		newdatas = 0;
		return(1);
	}
	l = strlen(rec);
	if(write(newdatas,rec,l) != l)
	{
		close(newdatas);
		newdatas = 0;
		dump_message();	/* dump the "transfer complete" message */
		return(0);
	}
	return(1);
}
	
/******************************************************************************\
|Routine: ftp_put_binary
|Callby: output_file
|Purpose: Writes binary data to the server.
|Arguments:
|    rec is the data to write.
|    bytes is how many to write. bytes = 0 means write EOF.
\******************************************************************************/
Int ftp_put_binary(rec,bytes)
Char *rec;
Int bytes;
{
	if(!bytes)	/* NULL argument signifies end-of-file */
	{
		close(newdatas);
		newdatas = 0;
		return(1);
	}
	if(write(newdatas,rec,bytes) != bytes)
	{
		close(newdatas);
		newdatas = 0;
		dump_message();	/* dump the "transfer complete" message */
		return(0);
	}
	return(1);
}
#endif	/* NO_FTP */

/******************************************************************************\
|Routine: ftp_unixy
|Callby: edit load_file
|Purpose: Returns 1 if the system is 'unixy'.
|Arguments:
|    system is the system type, one of V W M O U.
\******************************************************************************/
Int ftp_unixy(system)
Int system;
{
	if(strchr("WMOU",system))
		return(1);
	return(0);
}

