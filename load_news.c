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

#include "rec.h"
#include "window.h"
#include "ed_dec.h"	/* global data */
#include "handy.h"
#include "file.h"
#include "memory.h"
#include "ctyp_dec.h"

#define GROUPNAME_OFFSET 10	/* this is the offset from the beginning of the record to the group name */

typedef struct sort_str *sort_ptr;	/* structures for sorting by keys */
typedef struct sort_str
{
	rec_ptr rec;
	Char *key;
	Int length,recflags;
} sort_node;

static Char servername[128];

extern Int *cqsort_s();
#ifndef VMS
extern char *sys_errlist[];
#endif

/******************************************************************************\
|Routine: get_servername
|Callby: load_post
|Purpose: Returns the news server's name.
|Arguments:
|    none
\******************************************************************************/
Char *get_servername()
{
	return(servername);
}

/******************************************************************************\
|Routine: groupname_offset
|Callby: command edit
|Purpose: Returns the offset to the group name in diredit records.
|Arguments:
|    none
\******************************************************************************/
Int groupname_offset()
{
	return(GROUPNAME_OFFSET);
}

/******************************************************************************\
|Routine: get_artnum
|Callby: load_news
|Purpose: Parses read-article numbers from .newsrc entries. Returns 0 when there
|         are no further numbers, else 1.
|Arguments:
|    p is the pointer in the .newsrc entry. It is returned modified.
|    n1,n2 are the returned read-article range values. A single article returns
|    identical values for n1,n2.
\******************************************************************************/
Int get_artnum(p,n1,n2)
Char **p;
Int *n1,*n2;
{
	Char *q,c;
	Int i1,i2;
	
	q = *p;
	while(!isdigit(*q))	/* find a decimal digit */
		if(!*++q)
			return(0);
	for(i1 = 0;isdigit((c = *q));q++)
		i1 = 10 * i1 + c - '0';
	if(c == '-')
	{
		while(!isdigit(*q))	/* this shouldn't be necessary, but why not do it? */
			if(!*++q)
				return(0);
		for(i2 = 0;isdigit((c = *q));q++)
			i2 = 10 * i2 + c - '0';
	}
	else
		i2 = i1;
	*n1 = i1;
	*n2 = i2;
	*p = q;
	return(1);
}

/******************************************************************************\
|Routine: load_news
|Callby: main
|Purpose: Reads news group names from the user's .newsrc file (if it exists) and
|         from the server. Processes the two lists to eliminate obsolete groups
|         appearing in .newsrc, and adds new groups to the .newsrc list. Returns
|         1 if successful, else 0.
|Arguments:
|    server is the server name.
|    base is the main buffer base.
|    rcbase is the base of the NETRC buffer.
|    title is the returned window title (the server's connect message).
|    size is the size of title.
|    news is a flag indicating whether to just look at subscribed groups (if 1),
|    or display all groups (2).
\******************************************************************************/
Int load_news(server,base,rcbase,title,size,news)
Char *server;
rec_ptr base,rcbase;
Char *title;
Int size;
Char news;
{
	FILE *fp;
	rec_ptr new,r,rcr;
	Char buf[512],serv[512],netrc[512],response[512],group[512],*p,*q;
	Int i,j,l,nrecs,nrcrecs,first,last,*tag,unread,n1,n2,c;
	struct stat statbuf;
	sort_ptr s;
	
	strcpy(serv,server);
	strip_quotes(serv);
	envir_subs(serv);
	strcpy(servername,serv);	/* save for posting */
	
	nrecs = nrcrecs = 0;
	move(NROW,1);
	sprintf(buf,"Connecting to server '%s'...",server);
	putz(buf);
	next();
	putout();
	if(!news_open(server,title,size))
	{
		slip_message("I was not able to connect to the news server.");
		wait_message();
		return(0);
	}
/* get the complete list from the server */
	putz("Getting groups from the server...");
	next();
	putout();
	news_command("LIST");
	news_response(response,sizeof(response));
	if(response[0] != '2')
		return(1);
	while((l = news_response(response,sizeof(response))))
	{
		if(l == 1 && response[0] == '.')	/* server terminates the list */
			break;
		if(!strchr(response,'='))
		{
			nrecs++;
			new = (rec_ptr)imalloc(sizeof(rec_node));	/* get new record */
			insq(new,base->prev);
			new->data = (Char *)imalloc(strlen(response) + 10);
			strcpy(new->data,response);
			new->length = strlen(new->data);
			new->recflags = 1;	/* this is a freeable buffer */
		}
	}
/* get the list from .newsrc */
	putz("Reading .newsrc...");
	next();
	putout();
#ifdef VMS
	strcpy(netrc,"SYS$LOGIN:.NEWSRC");
#else
	strcpy(netrc,"$HOME/.newsrc");
#endif
	envir_subs(netrc);
	if(!stat(netrc,&statbuf))	/* if there's a .newsrc file... */
	{
		if(!(fp = fopen(netrc,"r")))
		{
			slip_message("I am unable to open your .newsrc file.");
			wait_message();
			toss_data(base);
			return(0);
		}
		while(fgets(buf,sizeof(buf),fp))	/* get contents of .newsrc */
		{
			if((p = strchr(buf,'\n')))
				*p = '\0';
/* verify the syntax of the entry */
			for(p = buf;(c = *p++);)
			{
				if(c == '!' || c == ':')
					break;
				if(!isalpha(c) && !isdigit(c) && c != '.' && c != '-' && c != '_' && c != '+')
					break;
			}
			if(c != '!' && c != ':')
				continue;
			if(!*p)
			{
				*p++ = ' ';
				*p = '\0';
			}
			else if(*p != ' ')
			{
				for(q = buf + strlen(buf) + 1;q != p;q--)
					*q = *(q - 1);
				*p = ' ';
			}
			nrcrecs++;
			new = (rec_ptr)imalloc(sizeof(rec_node));	/* get new record for netrc buffer */
			insq(new,rcbase->prev);
			new->data = (Char *)imalloc(strlen(buf) + 2);
			strcpy(new->data,buf);
			new->length = strlen(new->data);
			new->recflags = 1;	/* this is a freeable buffer */
		}
	}
/* sort what the server lists */
	if(nrecs > 1)
	{
		putz("Sorting the server list...");
		next();
		putout();
		s = (sort_ptr)imalloc((nrecs + 1) * sizeof(sort_node));	/* get sort structures */
		for(i = 0,r = base->next;i < nrecs;r = r->next)
		{
			s[i].rec = r;	/* load sort structures */
			s[i++].key = r->data;
		}
		tag = cqsort_s(nrecs,(Char *)&s[0].key,sizeof(s[0]));
		for(i = 0;i < nrecs;i++)
		{
			s[i].key = s[tag[i]].rec->data;
			s[i].recflags = s[tag[i]].rec->recflags;
			s[i].length = s[tag[i]].rec->length;
		}
		for(i = 0,r = base->next;i < nrecs;i++,r = r->next)
		{
			r->data = s[i].key;
			r->length = s[i].length;
			r->recflags = s[i].recflags;
		}
		ifree(s);
		ifree(tag);
	}
/* sort what's in .newsrc */
	if(nrcrecs > 1)
	{
		putz("Sorting the newsrc list...");
		next();
		putout();
		s = (sort_ptr)imalloc((nrcrecs + 1) * sizeof(sort_node));	/* get sort structures */
		for(i = 0,r = rcbase->next;i < nrcrecs;r = r->next)
		{
			s[i].rec = r;	/* load sort structures */
			s[i++].key = r->data;
		}
		tag = cqsort_s(nrcrecs,(Char *)&s[0].key,sizeof(s[0]));
		for(i = 0;i < nrcrecs;i++)
		{
			s[i].key = s[tag[i]].rec->data;
			s[i].recflags = s[tag[i]].rec->recflags;
			s[i].length = s[tag[i]].rec->length;
		}
		for(i = 0,r = rcbase->next;i < nrcrecs;i++,r = r->next)
		{
			r->data = s[i].key;
			r->length = s[i].length;
			r->recflags = s[i].recflags;
		}
		ifree(s);
		ifree(tag);
	}
/* now merge entries from server's list into newsrc list, setting them subscribed */
	putz("Merging the two lists...");
	next();
	putout();
	r = base->next;
	rcr = rcbase->next;
	while(1)
	{
		if(r == base)	/* hit end of server's list, no more insertions will be required */
			break;
		if(rcr == rcbase)	/* hit end of newsrc list, may need to add server entries */
		{
			while(r != base)	/* transfer any remaining server list entries */
			{
				new = (rec_ptr)imalloc(sizeof(rec_node));
				*(strchr(r->data,' ')) = '\0';
				new->data = (Char *)imalloc(strlen(r->data) + 3);
				sprintf(new->data,"%s: ",r->data);
				*(r->data + strlen(r->data)) = ' ';
				new->length = strlen(new->data);
				new->recflags = 1;	/* this is a freeable buffer */
				insq(new,rcr->prev);	/* insert at end of list */
				r = r->next;
			}
			break;
		}
/*    server list (r)                     newsrc list (rcr)

alt.3d 0000003197 03020 y                 alt.1d!
alt.abortion.inequity 0000009578 09523 y  alt.2d!
alt.activism 0050022 46558 y              alt.3d: 1-2785,2787,2791-2803
alt.activism.d 0000008839 08731 y         alt.59.79.99!
alt.aeffle.und.pferdle 0000000376 00351 y alt.abortion.inequity!
                                          alt.abuse-recovery:
                                          alt.abuse.offender.recovery!
                                          alt.abuse.recovery!
                                          alt.activism!
*/
		if(!(p = strchr(r->data,' ')))	/* prepare to null-terminate server list entry group name */
			continue;
		if(!(q = strchr(rcr->data,'!')))	/* prepare to null-terminate newsrc list entry group name */
			if(!(q = strchr(rcr->data,':')))
				continue;
		*p = '\0';
		c = *q;	/* save one of !, : */
		*q = '\0';
		i = strcmp(r->data,rcr->data);	/* compare the group names */
/* restore NULL-ified characters */
		*p = ' ';
		*q = c;
		if(!i)
		{
			r = r->next;	/* they match, just advance */
			rcr = rcr->next;
		}
		else if(i < 0)	/* server entry is less than newsrc entry */
		{
			new = (rec_ptr)imalloc(sizeof(rec_node));
			*p = '\0';
			new->data = (Char *)imalloc(strlen(r->data) + 3);
			sprintf(new->data,"%s: ",r->data);
			*p = ' ';
			new->length = strlen(new->data);
			new->recflags = 1;	/* this is a freeable buffer */
			insq(new,rcr->prev);	/* insert before current newsrc entry */
			r = r->next;	/* advance in server list */
		}
		else	/* server entry is greater than newsrc entry */
		{
			new = rcr->next;	/* toss the newsrc entry, it's obsolete */
			remq(rcr);
			ifree(rcr->data);
			ifree(rcr);
			rcr = new;
		}
	}
/* insure the two lists now contain the same number of entries */
	for(i = 0,r = base->next;r != base;r = r->next,i++);
	for(j = 0,r = rcbase->next;r != rcbase;r = r->next,j++);
	if(i != j)
	{
		sprintf(buf,"We had %d server entries and %d newsrc entries.",i,j);
		slip_message(buf);
		wait_message(buf);
		return(0);
	}
/*    server list (r)                     newsrc list (rcr)

alt.3d 0000003197 03020 y                 alt.3d: 1-2785,2787,2791-2803
alt.abortion.inequity 0000009578 09523 y  alt.abortion.inequity!
alt.activism 0050022 46558 y              alt.activism:
*/
/* calculate unread article counts, and reformat the server list. remove !'ed entries if appropriate */
	putz("Removing killed groups...");
	next();
	putout();
	for(r = base->next,rcr = rcbase->next;rcr != rcbase;rcr = rcr->next)
	{
        if(strchr(rcr->data,'!'))	/* remove !'ed entry */
        {
        	if(news == 1)	/* if display is limited to subscribed groups... */
        	{
	        	new = r->next;
	        	remq(r);
	        	ifree(r->data);
	        	ifree(r);
	        	r = new;
        	}
        	else	/* display all groups, but flag the unsubscribed ones */
        	{
        		r->recflags ^= 2;
	        	q = strchr(rcr->data,'!');
	        	goto do_anyway;
        	}
        }
        else	/* not a !'ed entry, process newsrc article numbers to modify article count */
        {
        	if(!(q = strchr(rcr->data,':')))
        	{
        		slip_message("The following line in the .newsrc file is incorrect:");
        		slip_message(rcr->data);
        		wait_message();
        		return(0);
        	}
do_anyway:
        	q++;
/* parse the server list entry */
        	if(my_sscanf(r->data,"%s%d%d",group,&last,&first) != 3)
        	{
        		slip_message("The server returned a bad group listing:");
        		slip_message(r->data);
        		wait_message();
        		return(0);
        	}
			unread = last - first + 1;
/* get tokens from the newsrc entry */
        	while(get_artnum(&q,&n1,&n2))
        		if(n2 >= first)	/* overlaps current range */
					unread -= n2 - max(first,n1) + 1;
/* if there are no articles, don't display the entry */
			if(unread <= 0)
			{
				new = r->next;
				remq(r);
				ifree(r->data);
				ifree(r);
				r = new;
			}
			else	/* generate what the user will see */
			{
				sprintf(buf," %6d   %s",unread,group);
				ifree(r->data);
				r->data = (Char *)imalloc((r->length = strlen(buf)) + 1);
				strcpy(r->data,buf);
				r = r->next;
			}
        }
	}
	if(base->next == base)
	{
		slip_message("There are no newsgroups to read. Perhaps your .newsrc file killed them all?");
		wait_message();
		return(0);
	}
    return(1);
}
#endif

