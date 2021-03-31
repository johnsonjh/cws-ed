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
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "unistd.h"
#include "memory.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"	/* global data */

extern Char *get_servername();
extern Char *version();

/******************************************************************************\
|Routine: count_equals
|Callby: load_post
|Purpose: Returns a count of how many '='s precede a subject entry.
|Arguments:
|    rec is the record being examined.
\******************************************************************************/
Int count_equals(rec)
rec_ptr rec;
{
	Char *p,*q;
	
	p = q = rec->data + 42;
	while(*q++ == '=');
	return(--q - p);
}

/******************************************************************************\
|Routine: get_post_info
|Callby: load_post
|Purpose: Returns the username and host.
|Arguments:
|    user is the returned user name.
|    host is the returned host name.
|    hostsize is the size of host.
\******************************************************************************/
void get_post_info(user,host,hostsize)
Char *user,*host;
Int hostsize;
{
	Char *p;
	
#ifdef CUSERID_ENV
	strcpy(user,getenv(CUSERID_ENV));
#else
	strcpy(user,cuserid(NULL));
#endif
	gethostname(host,hostsize);
	if(!strchr(host,'.'))
	{
#ifdef UCX
		if((p = getenv("UCX$BIND_DOMAIN")))
		{
			strcat(host,".");
			strcat(host,p);
		}
#else
#ifdef WOLLONGONG
		if((p = getenv("INET_DOMAIN_NAME")))
		{
			strcat(host,".");
			strcat(host,p);
		}
#else
/*		getdomainname(domain,domainsize); */
#endif
#endif
	}
}
	
/******************************************************************************\
|Routine: load_post_string
|Callby: load_post
|Purpose: Creates a new records, loads a string, appends to a buffer.
|Arguments:
|    base is the base of the buffer that receives the new record.
|    string is the string to append.
\******************************************************************************/
void load_post_string(base,string)
rec_ptr base;
Char *string;
{
	rec_ptr new;
	
	new = (rec_ptr)imalloc(sizeof(rec_node));
	new->data = (Char *)imalloc((new->length = strlen(string)) + 1);
	strcpy(new->data,string);
	new->recflags = 1;
	insq(new,base->prev);
}
	
/******************************************************************************\
|Routine: load_post
|Callby: edit
|Purpose: Loads the headers of a news posting.
|Arguments:
|    base is the base of the buffer that receives the headers.
|    subject is the subject of the posting.
\******************************************************************************/
void load_post(base,subject)
rec_ptr base;
Char *subject;
{
	Char buf[512],subjbuf[512],resp[512],user[128],host[128],email[256],name[256];
	Char *p,*q,*msg,*tmp,*m,*new;
	rec_ptr r;
	FILE *fp;
	time_t t;
#ifdef VMS
	Char wdy[8],mon[8];
	Int day,hour,min,sec,year;
#endif
	Int artnum,i;

/* get the identifying information */
	get_post_info(user,host,sizeof(host));
/* try to get the real name from the passwd file */
	name[0] = '\0';
	if((fp = fopen("/etc/passwd","r")))
	{
		while(fgets(buf,sizeof(buf),fp))
			if((p = strchr(buf,':')))
			{
				*p = '\0';
				if(!strcmp(user,buf))
					if((p = strchr(++p,':')))
						if((p = strchr(++p,':')))
							if((p = strchr(++p,':')))
								if((q = strchr(++p,':')))
								{
									*q = '\0';
									strcpy(name,p);
									break;
								}
			}
		fclose(fp);
	}
	sprintf(email,"%s@%s",user,host);
	sprintf(buf,"Relay-Version: version ED-%s; site %s",version(),host);
	load_post_string(base,buf);
	sprintf(buf,"Posting-Version: version ED-%s; site %s",version(),host);
	load_post_string(base,buf);
	if(name[0] != '\0')
		sprintf(buf,"From: %s <%s>",name,email);
	else
		sprintf(buf,"From: <%s>",email);
	load_post_string(base,buf);
	t = time(0);
#ifdef VMS
/*Sun Sep 16 01:03:52 1973\n\0*/
	my_sscanf(ctime(&t),"%s%s%d%d:%d:%d%d",wdy,mon,&day,&hour,&min,&sec,&year);
	sprintf(buf,"Date: %s, %2d %s %4d %2d:%2d:%2d",wdy,day,mon,year,hour,min,sec);
#else
	strftime(buf,sizeof(buf),"Date: %a, %d %b %Y %T %Z",localtime(&t));
#endif
	load_post_string(base,buf);
	sprintf(buf,"Newsgroups: %s",WINDOW[CURWINDOW].filename);
	load_post_string(base,buf);
	sprintf(buf,"Message-ID: <%08x.%s>",time(NULL),email);
	load_post_string(base,buf);
	sprintf(buf,"Path: %s!%s",get_servername(),user);
	load_post_string(base,buf);
/* handle posting references */
	if(CURREC != BASE)
	{
		if(strncmp(subject,"Re: ",4) && strncmp(subject,"re: ",4))
		{
			sprintf(subjbuf,"Re: %s",subject);
			subject = subjbuf;
		}
		r = CURREC;
		msg = NULL;
		while(1)
		{
			my_sscanf(r->data,"%d",&artnum);
			sprintf(buf,"STAT %d",artnum);
			news_command(buf);
			news_response(resp,sizeof(resp));
			if(resp[0] == '2')
			{
				news_command("HEAD");
				news_response(resp,sizeof(resp));
				if(resp[0] == '2')
					while(1)
					{
						news_response(resp,sizeof(resp));
						if(!strcmp(resp,"."))
							break;
						if(!strncmp(resp,"Message-ID: ",strlen("Message-ID: ")))
							if((m = strchr(resp,'<')))
								if(!msg)
								{
									msg = (Char *)imalloc(strlen(m) + 1);
									strcpy(msg,m);
								}
								else
								{
									new = (Char *)imalloc(strlen(msg) + 1 + strlen(m) + 1);
									sprintf(new,"%s %s",m,msg);
									ifree(msg);
									msg = new;
								}
					}
			}
/* new back up to record with one less '=' */
			if(!(i = count_equals(r)))
				break;
			do
			{
				if((r = r->prev) == BASE)
					break;
			} while(count_equals(r) != i - 1);
		}
		tmp = (Char *)imalloc(strlen("References: ") + strlen(msg) + 1);
		sprintf(tmp,"References: %s",msg);
		load_post_string(base,tmp);
		ifree(msg);
		ifree(tmp);
	}
	sprintf(buf,"Subject: %s",subject);
	load_post_string(base,buf);
	sprintf(buf,"Reply-To: %s",email);
	load_post_string(base,buf);
	buf[0] = '\0';
	load_post_string(base,buf);
}
#endif

