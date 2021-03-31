/*
 * Copyright (C) 1992 by Rush Record (rhr@clio.rice.edu)
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

#include "handy.h"
#include "memory.h"
#include "ctyp_dec.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"	/* global data */
#include "buffer.h"

static Int articlesread;

/******************************************************************************\
|Routine: hash_mesg
|Callby: news_group
|Purpose: Generates a hash value from a message ID.
|Arguments:
|    buf is the buffer containing the message ID.
\******************************************************************************/
unsigned long hash_mesg(buf)
Char *buf;
{
	unsigned long hcalc;
	Char *p,*q;
	
	hcalc = 0;
	if((p = strrchr(buf,'<')) && (q = strrchr(buf,'>')))	/* hash the last message id */
		if(q > p)
			for(p++;*p != '>';p++)
			{
				hcalc ^= (unsigned long)*p;
				if(hcalc & 0x80000000)
				{
					hcalc <<= 1;
					hcalc++;
				}
				else
					hcalc <<= 1;
			}
	return(hcalc);
}

/******************************************************************************\
|Routine: news_get_headers
|Callby: news_group
|Purpose: Generates a list of articles.
|Arguments:
|    count is the count of headers to read.
|    base is the base of the buffer we're generating.
|    total is the total number of headers being read for the newsgroup.
|    key is the key the user pressed to select the group (either 'r' or 'R').
\******************************************************************************/
void news_get_headers(count,base,total,key)
Int count;
rec_ptr base;
Int total;
Schar key;
{
	Int i,l,lines,code,artnum,day,year,unread;
	unsigned long msghash,refhash;
	Char from[512],subj[512],resp[512],buf[512],token[512],month[16],date[32],*p,*q;
	rec_ptr new;

	for(i = 0;i < count;i++)
	{
		news_response(resp,sizeof(resp));
		if(resp[0] != '2')	/* an unretrievable header */
			continue;
		my_sscanf(resp,"%d%d",&code,&artnum);
		from[0] = subj[0] = date[0] = '\0';
		msghash = 0;
		refhash = 0;
		while(1)
		{
			news_response(resp,sizeof(resp));
			if(strlen(resp) == 1 && resp[0] == '.')
				break;
			else if(!strncmp(resp,"From: ",strlen("From: ")))
				strcpy(from,resp + strlen("From: "));
			else if(!strncmp(resp,"Subject: ",strlen("Subject: ")))
				strcpy(subj,resp + strlen("Subject: "));
			else if(!strncmp(resp,"Lines: ",strlen("Lines: ")))
				my_sscanf(resp + strlen("Lines: "),"%d",&lines);
			else if(!strncmp(resp,"Message-ID: ",strlen("Message-ID: ")))
				msghash = hash_mesg(resp);
			else if(!strncmp(resp,"References: ",strlen("References: ")))
				refhash = hash_mesg(resp);
/*			else if(!strncmp(resp,"In-Reply-To: ",strlen("In-Reply-To: ")))
				refhash = hash_mesg(resp);*/
			else if(!strncmp(resp,"Date: ",strlen("Date: ")))
			{
				p = resp + strlen("Date: ");
				if((q = strchr(p,',')))
					p = ++q;
				while((q = strchr(p,'-')))
					*q = ' ';
				get_token(p,token);
				if(my_sscanf(token,"%d",&day) != 1)
					day = 0;
				get_token(p,month);
				get_token(p,token);
				if(my_sscanf(token,"%d",&year) != 1)
					year = 0;
				if(year < 70)
					year += 2000;
				else if(year < 100)
					year += 1900;
				sprintf(date,"%2d-%s-%d",day,month,year);
			}
		}
		unread = newsrc_unread(artnum);
		if(key == 'R' || key == 'V' || unread)
		{
			if(!from[0])
				strcpy(from,"???");
			else if((p = strchr(from,'@')))
				strcpy(p,"            ");
			if(!subj[0])
				strcpy(subj,"<unknown>");
			if(!date[0])
				strcpy(date,"??-???-??");
			strcat(from,"                  ");
			from[12] = '\0';
/*
initial layout of the display:		
hashval refval  lv article    sender        date      lines                    subject
xxxxxxxxyyyyyyyyll nnnnnn  ssssssssssss  dd-mmm-yyyy  nnnnn  aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
*/
			sprintf(buf,"%08x%08x00 %6d  %s  %s  %5d  %s",msghash,refhash,artnum,from,date,lines,subj);
			new = (rec_ptr)imalloc(sizeof(rec_node));
			new->data = (Char *)imalloc((l = strlen(buf)) + 1);
			strcpy(new->data,buf);
			new->length = l;
			if(unread)
				new->recflags = 1;
			else
				new->recflags = 3;
			insq(new,base->prev);
		}
		if(!(++articlesread % 10))
		{
			move(WINDOW[CURWINDOW].toprow - 1,1);
			ers_end();
			sprintf(buf,"Total headers: %d, total read so far: %d.",total,articlesread);
			putz(buf);
			putout();
		}
	}
}

/******************************************************************************\
|Routine: news_group
|Callby: edit
|Purpose: Creates/Updates a newsgroup window.
|Arguments:
|    group is the newsgroup name.
|    n is either the window number of an existing newsgroup window, or NWINDOWS
|        if there is no newsgroup window.
|    key is the key the user pressed to select the group ('r', 'R', 'v' or 'V').
\******************************************************************************/
void news_group(group,n,key)
Char *group;
Int n;
Schar key;
{
	rec_ptr base,r,s,t,dest;
	Int total,i,j,l,a1,excess,code,count,lev,moveback,last,this,low,high;
	Char prompt[512],buf[512],resp[512],*p,*q;
	
	base = (rec_ptr)imalloc(sizeof(rec_node));
	base->length = 0;
	base->data = NULL;
	base->next = base->prev = base;
	if(key == 'r' || key == 'R')
		moveback = CURWINDOW;	/* we have to move back to the top level window */
	else
		moveback = -1;	/* we must not do so */
/* generate the buffer to hold the newsgroup postings */
	sprintf(buf,"GROUP %s",group);
	news_command(buf);
	news_response(resp,sizeof(resp));
	if(resp[0] != '2')
	{
		slip_message("The news server has a problem with that group. It said:");
		slip_message(resp);
		wait_message();
		return;
	}
/* get the approx article count */
	my_sscanf(resp,"%d%d%d%d",&code,&count,&low,&high);
/* mark 1 through low-1 as read */
	newsrc_low(low);
/* check for no articles */
	if(count <= 0)
	{
		slip_message("There do not appear to be any articles in that group.");
		wait_message();
		return;
	}
/* if they're not getting everything, subtract the ones we've already read */
	if(islower(key))
		for(i = low;i <= high;i++)
			if(!newsrc_unread(i))
				count--;
	if(count <= 0)
	{
		slip_message("You have already read all the articles in that group.");
		wait_message();
		return;
	}
/* if the group is large, ask them how many articles they want */
	if(count > 50)
	{
retry:
		sprintf(prompt,"There are %d articles in this group, how many do you want? [%d] ",count,count);
		if(!inquire(prompt,buf,sizeof(buf),1))
			a1 = 1;
		else
		{
			if(my_sscanf(buf,"%d-%d",&i,&j) == 2)
			{
				a1 = i;
				count = j - i + 1;
			}
			else if(my_sscanf(buf,"%d",&i) == 1)
			{
				a1 = 1;
				count = i;
			}
			else
				goto retry;
		}
		paint(BOTROW,BOTROW,FIRSTCOL);
		if(!count)
			return;
	}
	else
		a1 = 1;
/* get all articles */
	articlesread = 0;
	news_command("STAT");
	news_response(resp,sizeof(resp));
	if(resp[0] != '2')
	{
		slip_message("I was unable to STAT the first article in that group.");
		wait_message();
		return;
	}
/* skip articles if indicated */
	if(a1 > 1)
	{
		for(i = 0;i < a1 - 1;i++)
		{
			news_command("NEXT");
			if((i > 0) && !(i % 400))	/* wait until all of these have been dealt with */
				for(j = 0;j < 400;j++)
				{
					if(!(++articlesread % 10))
					{
						move(WINDOW[CURWINDOW].toprow - 1,1);
						ers_end();
						sprintf(buf,"Total headers: %d, total skipped so far: %d.",count,articlesread);
						putz(buf);
						putout();
					}
					news_response(resp,sizeof(resp));
				}
		}
/* get any remaining responses */
		if((j = (i % 400)))
			while(j--)
			{
				if(!(++articlesread % 10))
				{
					move(WINDOW[CURWINDOW].toprow - 1,1);
					ers_end();
					sprintf(buf,"Total headers: %d, total skipped so far: %d.",count,articlesread);
					putz(buf);
					putout();
				}
				news_response(resp,sizeof(resp));
			}
	}
/* send all the commands */
	for(i = 0;i < count;i++)
	{
		news_command("HEAD");
		news_command("NEXT");
		if((i > 0) && !(i % 400))	/* wait until all of these have been dealt with */
			news_get_headers(400,base,count,key);
	}
/* get any remaining responses */
	if((j = (i % 400)))
		news_get_headers(j,base,count,key);
/* get the final error message or stat response */
	news_response(resp,sizeof(resp));
/* mark all missing articles as 'read' */
	if((r = base->next) != base)
	{
		my_sscanf(strchr(r->data,' '),"%d",&last);	/* first article number */
		for(r = r->next;r != base;r = r->next)
		{
			my_sscanf(strchr(r->data,' '),"%d",&this);
			if(this > last + 1)
				for(i = last + 1;i < this;i++)
					newsrc_mark_read(i);
			last = this;
		}
	}
/* restore the window title */
	move(WINDOW[CURWINDOW].toprow - 1,1);
	ers_end();
	reverse();
	putz(WINDOW[CURWINDOW].filename);
	normal();
/*
final layout of the display:		
article    sender        date      lines                 subject
nnnnnn  ssssssssssss  dd-mmm-yyyy  nnnnn  aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
*/
/* rearrange everything on the basis of references */
	for(r = base->next;r != base;r = r->next)
	{
		for(dest = r,s = r->next;s != base;)	/* flag all entries that refer to this one */
			if(!strncmp(r->data,s->data + 8,8))
			{
				my_sscanf(r->data + 16,"%d",&lev);
				sprintf(buf,"%02d",++lev);
				memcpy(s->data + 16,buf,2);
				t = s->next;
				remq(s);
				insq(s,dest);
				dest = s;
				s = t;
			}
			else
				s = s->next;
	}
/* reformat to remove the hash values */
	for(r = base->next;r != base;r = r->next)
	{
		my_sscanf(r->data + 16,"%d",&lev);
		p = q = (Char *)imalloc((l = strlen(r->data) - 19 + lev) + 1);
		memcpy(q,r->data + 19,42);
		q += 42;
		while(lev--)
			*q++ = '=';
		strcpy(q,r->data + 61);
		ifree(r->data);
		r->data = p;
		r->length = l;
	}
	save_window();
/* generate a new window if there isn't one already */
	if(n == NWINDOWS)
	{
		for(total = i = 0;i < NWINDOWS;i++)
			if((excess = WINDOW[i].botrow - WINDOW[i].toprow - 2) > 0)	/* there is space available here */
				total += excess;
		if(total < 4 || NWINDOWS >= MAX_WINDOWS)
		{
			slip_message("There isn't enough room for the newsgroup window.");
			wait_message();
			return;
		}
		insert_window_percent(80);
		insert_window(group,base,0,0);	/* zeroes refer to binary and diredit flags */
		WINDOW[CURWINDOW].filebuf = NULL;
		WINDOW[CURWINDOW].binary = 0;
		WINDOW[CURWINDOW].diredit = 0;
		WINDOW[CURWINDOW].news = 2;
		if(moveback >= 0)
		{
			save_window();
			set_window(moveback);
			save_window();
		}
	}
	else
	{
		toss_data(WINDOW[n].base);
		WINDOW[n].base = base;
		WINDOW[n].currec = WINDOW[n].toprec = base->next;
		new_botrec(n);
		WINDOW[n].curbyt = 0;
		WINDOW[n].currow = WINDOW[n].toprow;
		WINDOW[n].curcol = WINDOW[n].firstcol = WINDOW[n].wantcol = 1;
		WINDOW[n].modified = WINDOW[n].diredit = WINDOW[n].binary = 0;
		WINDOW[n].news = 2;
		if(WINDOW[n].filename)
			ifree(WINDOW[n].filename);
		WINDOW[n].filename = (Char *)imalloc(strlen(group) + 1);
		strcpy(WINDOW[n].filename,group);
		ref_window(n);
		save_window();
		if(moveback < 0)
		{
			set_window(n);	/* move to article list window */
			save_window();
		}
	}
}
#endif

