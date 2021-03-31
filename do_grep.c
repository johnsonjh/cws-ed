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

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "rec.h"
#include "buffer.h"
#include "window.h"
#include "ed_dec.h"
#include "cmd_enum.h"

/******************************************************************************\
|Routine: grep_report_skip
|Callby: do_grep
|Purpose: Reports how many lines did/didn't match.
|Arguments:
|    LINES - the number of lines to report.
|    FUNCTION - either 'skip' or 'match'.
|    BASE - the grep buffer base.
\******************************************************************************/
void grep_report_skip(lines,function,base)
Int lines;
Char *function;
rec_ptr base;
{
	rec_ptr new;
	Char buf[128];
	Int length;
	
	new = (rec_ptr)imalloc(sizeof(rec_node));
	sprintf(buf,"<%s %d %s>",function,lines,(lines > 1)? "lines" : "line");
	new->data = (Char *)imalloc((length = strlen(buf)) + 1);
	memcpy(new->data,buf,length);
	new->data[length] = '\0';
	new->length = length;
	new->recflags = 1;	/* it is a freeable buffer */
	insq(new,base->prev);
}

/******************************************************************************\
|Routine: grep_report_line
|Callby: do_grep
|Purpose: Puts a matching/non-matching line in the grep record queue.
|Arguments:
|    REC - the record to add to the queue.
|    BASE - the grep buffer base.
\******************************************************************************/
void grep_report_line(rec,base)
rec_ptr rec,base;
{
	rec_ptr new;
	
	new = (rec_ptr)imalloc(sizeof(rec_node));	/* report contents of line */
	new->data = (Char *)imalloc(rec->length + 1);
	memcpy(new->data,rec->data,new->length = rec->length);
	new->data[rec->length] = '\0';
	new->recflags = 1;	/* it is a freeable buffer */
	insq(new,base->prev);
}

/******************************************************************************\
|Routine: do_grep
|Callby: command
|Purpose: Generates a buffer with grep'd records and spacers.
|Arguments:
|    OLDBASE - the buffer the user is searching for ALL occurrences of the string in.
|    NEWBASE - the buffer that will be created.
|    STRING - the string to search for.
|    GREPPERG - =1 means GREP, =0 means PERG (opposite of GREP).
|    SILENT - flag that <skip...> lines should not appear.
\******************************************************************************/
void do_grep(oldbase,newbase,string,grepperg,silent)
rec_ptr oldbase,newbase;
Int grepperg;
Char *string;
Int silent;
{
	rec_ptr prev,rec,save_prev;
	buf_node search;
	Int byt,lines,hits;
	Char search_flags[5];
	
/* initialize */
	rec = (save_prev = prev = oldbase)->next;
	byt = 0;
	hits = 0;
/* make an empty buffer to hold the search string */
	search.first = (rec_ptr)&search.first;
	search.last = (rec_ptr)&search.first;
	string_to_buf(string,&search);
/* copy current search flags */
	memcpy(search_flags,SEARCH_FLAGS,((int)NUM_SMODES) >> 1);
	search_flags[1] = 'e';	/* set end mode for search, so we can match first record if it matches the search string at offset 0 */
/* find instances of the search string */
	while(find_string(rec,byt,1,&search,search_flags,1))
	{
		get_string_loc(&rec,&byt);	/* get new location */
		save_prev = prev;
		for(lines = -1;prev != rec;prev = prev->next,lines++);	/* count lines skipped */
		if(lines >= 0)	/* if we're not still on the same record... */
		{
			if(grepperg)
			{
				if(lines > 0 && !silent)	/* if we skipped more than one line forward, report skip */
					grep_report_skip(lines,"skip",newbase);
				grep_report_line(rec,newbase);
			}
			else
			{
				if(lines > 0)
				{
					if(hits && !silent)	/* report skip over matching lines */
						grep_report_skip(hits,"match",newbase);
					while(lines--)
						grep_report_line(save_prev = save_prev->next,newbase);
					hits = 1;
				}
				else
					hits++;
				save_prev = prev;	/* we have to do this, in case this was the last match in the file. it prevents the code at the end
				                       from reporting the line containing the search string. */
			}
		}
	}
/* report on the tail end of the file */
	if(grepperg)
	{
		for(lines = -1;prev != oldbase;prev = prev->next,lines++);	/* count lines skipped */
		if(lines > 0 && !silent)
			grep_report_skip(lines,"skip",newbase);
	}
	else
	{
		if(hits && !silent)	/* report skip over matching lines */
			grep_report_skip(hits,"match",newbase);
		while((save_prev = save_prev->next) != oldbase)
			grep_report_line(save_prev,newbase);
	}
}

