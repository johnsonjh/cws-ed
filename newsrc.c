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

#include "memory.h"
#include "ctyp_dec.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"	/* global data */
#include "buffer.h"
#include "buf_dec.h"	/* kill buffers */
#include "cmd_enum.h"
#include "handy.h"	/* for definition of max() */
#include "keyvals.h"	/* for numeric values corresponding to editor commands */
#include "file.h"

static rec_ptr netrc_rec = NULL;	/* this gets the NETRC entry when they select a group */
static Int *first,*last,pairs = 0,allocated = 0;

/******************************************************************************\
|Routine: newsrc_encode
|Callby: newsrc_mark_read newsrc_mark_unread
|Purpose: Formats read-article list into NETRC entry.
|Arguments:
|    none
\******************************************************************************/
void newsrc_encode()
{
	Int groupl,count,i;
	Char numbuf[32],*p,*buf;
	
	groupl = count = strchr(netrc_rec->data,' ') - netrc_rec->data + 1;
	for(i = 0;i < pairs;i++)
	{
		if(first[i] == last[i])
		{
			sprintf(numbuf,"%d,",first[i]);
			count += strlen(numbuf);
		}
		else
		{
			sprintf(numbuf,"%d-%d,",first[i],last[i]);
			count += strlen(numbuf);
		}
	}
	buf = (Char *)imalloc(count + 1);
	memcpy(buf,netrc_rec->data,groupl);
	for(p = buf + groupl,i = 0;i < pairs;i++)
		if(first[i] == last[i])
		{
			sprintf(p,"%d,",first[i]);
			p += strlen(p);
		}
		else
		{
			sprintf(p,"%d-%d,",first[i],last[i]);
			p += strlen(p);
		}
	if(pairs)
		*--p = '\0';	/* trim the last comma */
	else
		*p = '\0';
	ifree(netrc_rec->data);
	netrc_rec->data = buf;
}

/******************************************************************************\
|Routine: newsrc_make_room
|Callby: newsrc_mark_read newsrc_mark_unread
|Purpose: Insures we have enough room for a given number of pairs.
|Arguments:
|    n is the number of pairs required.
\******************************************************************************/
void newsrc_make_room(n)
Int n;
{
	Int *nf,*nl;
	
	if(n > allocated)
	{
		nf = (Int *)imalloc((pairs + 1) * sizeof(Int));
		nl = (Int *)imalloc((pairs + 1) * sizeof(Int));
		if(pairs)
		{
			memcpy(nf,first,pairs * sizeof(Int));
			memcpy(nl,last,pairs * sizeof(Int));
		}
		if(first)
			ifree(first);
		if(last)
			ifree(last);
		first = nf;
		last = nl;
		allocated = n;
	}
}

/******************************************************************************\
|Routine: newsrc_low
|Callby: news_group
|Purpose: Marks article 1 through article low-1 as read.
|Arguments:
|    low is the lowest-numbered article in the group (returned by GROUP).
\******************************************************************************/
void newsrc_low(low)
Int low;
{
	Int i,j,k;
	
	if(!pairs)
	{
		newsrc_make_room(1);
		first[0] = 1;
		last[0] = low - 1;
		pairs = 1;
	}
	else if(low == first[0])
		first[0] = 1;
	else if(low < first[0])
	{
		newsrc_make_room(pairs + 1);
		for(i = pairs;i > 0;i--)
		{
			first[i] = first[i - 1];
			last[i] = last[i - 1];
		}
		first[0] = 1;
		last[0] = low - 1;
		pairs++;
	}
	else
	{
		for(i = 0;i < pairs;i++)
			if(first[i] >= low)
				break;
		if(first[i] == low)
			first[i] = 1;
		else
		{
			first[i - 1] = 1;
			last[i - 1] = low - 1;
		}
		if(i > 0)
		{
			for(j = i,k = 0;j < pairs;j++)
			{
				first[k] = first[j];
				last[k++] = last[j];
			}
			pairs--;
		}
	}
	newsrc_encode();
}

/******************************************************************************\
|Routine: newsrc_save
|Callby: command
|Purpose: Saves a new .newsrc.
|Arguments:
|    none
\******************************************************************************/
Int newsrc_save()
{
	Char fname[512],fname2[512],buf[512],*r;
	rec_ptr p,q;
	FILE *fp;
	Int l;
	
#ifdef VMS
	strcpy(fname,"sys$login:.newsrc-new");
	strcpy(fname2,"sys$login:.newsrc");
#else
	strcpy(fname,"$HOME/.newsrc.new");
	strcpy(fname2,"$HOME/.newsrc");
#endif
	envir_subs(fname);
	envir_subs(fname2);
	if(!(fp = fopen(fname,"w")))
	{
		slip_message("I am unable to create the file $HOME/.newsrc.new.");
		wait_message();
		return(0);
	}
/* examine all visible groups, checking for deletions */
	for(p = WINDOW[0].base->next,q = NETRC->next;p != WINDOW[0].base;p = p->next)
	{
		r = p->data + groupname_offset();
		l = strlen(r);	/* get the length of the group name */
		while(strncmp(r,q->data,l))	/* advance to matching entry in newsrc database */
			q = q->next;
		if(p->recflags & 2)	/* this newsgroup has been marked for unsubscription, insure it's unsubscribed */
		{
			if((r = strchr(q->data,':')))
				*r = '!';
		}
		else	/* this newsgroup has been marked for subscription, insure it's subscribed */
		{
			if((r = strchr(q->data,'!')))
				*r = ':';
		}
	}
/* load all the entries to the file */
	for(q = NETRC->next;q != NETRC;q = q->next)
		fprintf(fp,"%s\n",q->data);
	fclose(fp);
/* rename the new file to the old file */
	if(rename(fname,fname2))
	{
		sprintf(buf,"I was unable to rename '%s' to '%s'. '%s' has your saved changes.",fname,fname2,fname);
		wait_message();
		return(0);
	}
	return(1);
}

/******************************************************************************\
|Routine: newsrc_current
|Callby: command
|Purpose: Sets the current NETRC pointer.
|Arguments:
|    group is the group name.
\******************************************************************************/
void newsrc_current(group)
Char *group;
{
	rec_ptr p;
	Char *sp;
	Int n1,n2,*i1,*i2,l;
	
/* find the matching entry in the NETRC list */
	l = strlen(group);
	for(p = NETRC->next;p != NETRC;p = p->next)
		if(!strncmp(p->data,group,l))
			break;
	netrc_rec = p;
/* count the range pairs */
	sp = netrc_rec->data;
	for(pairs = 0;get_artnum(&sp,&n1,&n2);pairs++);
/* reallocate memory */
	if(pairs > allocated)
	{
		if(first)
			ifree(first);
		if(last)
			ifree(last);
		first = (Int *)imalloc(pairs * sizeof(Int));
		last = (Int *)imalloc(pairs * sizeof(Int));
		allocated = pairs;
	}
/* load the arrays */
	sp = netrc_rec->data;
	i1 = first;
	i2 = last;
	while(get_artnum(&sp,i1++,i2++));
}

/******************************************************************************\
|Routine: newsrc_unread
|Callby: news_group
|Purpose: Returns an indication of whether the given article has been read.
|Arguments:
|    article is the article number.
\******************************************************************************/
Int newsrc_unread(article)
Int article;
{
	Int i;
	
	for(i = 0;i < pairs;i++)
		if(article >= first[i] && article <= last[i])
			return(0);
	return(1);
}

/******************************************************************************\
|Routine: newsrc_mark_read
|Callby: edit
|Purpose: Modifies the current NETRC list to mark an article as read.
|Arguments:
|    article is the article number.
\******************************************************************************/
void newsrc_mark_read(article)
Int article;
{
	Int i,j;
	
	for(i = 0;i < pairs;i++)
	{
		if(article >= first[i] && article <= last[i])
			return;	/* the article is already marked as read?? */
		if(article == first[i] - 1)	/* if the article is right before an existing pair, merge it */
		{
			first[i] = article;
			if(i > 0)	/* if the current pair is now contiguous with its predecessor, merge it */
				if(last[i - 1] == first[i] - 1)
				{
					last[i - 1] = last[i];
					for(j = i + 1;j < pairs - 1;j++)
					{
						first[j] = first[j + 1];
						last[j] = last[j + 1];
					}
					pairs--;
				}
			goto report;
		}
		if(article == last[i] + 1)
		{
			last[i] = article;
			if(i < pairs - 1)
				if(last[i] == first[i + 1] - 1)
				{
					last[i] = last[i + 1];
					for(j = i + 1;j < pairs - 1;j++)
					{
						first[j] = first[j + 1];
						last[j] = last[j + 1];
					}
					pairs--;
				}
			goto report;
		}
	}
/* it doesn't match any existing range, it needs its own entry */
	for(i = 0;i < pairs;i++)
		if(first[i] > article)
			break;
/* insure we have room to insert an entry */
	newsrc_make_room(pairs + 1);
	if(i == pairs)
		first[pairs] = last[pairs] = article;
	else	/* the new one must be inserted into the list */
	{
		for(j = pairs;j > i;j--)	/* slide everyone over */
		{
			first[j] = first[j - 1];
			last[j] = last[j - 1];
		}
		first[i] = last[i] = article;
	}
	pairs++;
/* now encode the list into a buffer and replace it in the current NETRC record */
report:
	newsrc_encode();
}

/******************************************************************************\
|Routine: newsrc_mark_unread
|Callby: edit
|Purpose: Modifies the current NETRC list to mark an article as read.
|Arguments:
|    article is the article number.
\******************************************************************************/
void newsrc_mark_unread(article)
Int article;
{
	Int i,j;
	
	for(i = 0;i < pairs;i++)
	{
		if(article > first[i] && article < last[i])	/* article is part of a range, split it */
		{
			newsrc_make_room(pairs + 1);
			for(j = pairs;j > i;j--)
			{
				first[j] = first[j - 1];
				last[j] = last[j - 1];
			}
			last[i] = article - 1;
			first[i + 1] = article + 1;
			pairs++;
			goto report;
		}
		if(article == first[i])
		{
			if(article == last[i])
			{
				for(j = i;j < pairs - 1;j++)
				{
					first[j] = first[j + 1];
					last[j] = last[j + 1];
				}
				pairs--;
			}
			else
				first[i] = article + 1;
			goto report;
		}
		if(article == last[i])
		{
			last[i] = article - 1;
			goto report;
		}
	}
/* it isn't shown as read, ignore the request */
	return;
/* now encode the list into a buffer and replace it in the current NETRC record */
report:
	newsrc_encode();
}
#endif

