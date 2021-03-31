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

#include "library.h"

extern Char *help_get_text();

/******************************************************************************\
|Routine: help_load
|Callby: help
|Purpose: Loads the online help library.
|Arguments:
|    fp is the file pointer of the online help file.
|    retbase is the returned pointer to the created data structure.
\******************************************************************************/
void help_load(fp,retbase)
FILE *fp;
topic_ptr *retbase;
{
	topic_ptr base,cur,top;
	subtopic_ptr sub,last;
	Int level,curlevel;
	Char *keyword;

/* allocate the base node */
	base = (topic_ptr)imalloc(sizeof(topic_node));
	base->first = NULL;
	base->parent = NULL;
	if(!(level = help_get_keyword(fp,&keyword)))
	{
		base->topic = keyword;	/* store top level keyword */
		base->text = (Char *)help_get_text(fp);	/* store top level text */
		level = help_get_keyword(fp,&keyword);	/* get first level 1 keyword (to match other block of the ifelse) */
	}
	else
		base->topic = base->text = NULL;
	cur = base;
	curlevel = 1;
	last = (subtopic_ptr)&cur->first;
/* get subtopics (at this point, we just got a new keyword) */
	do
	{
		if(level > curlevel)
		{
			cur = last->child;
			curlevel++;
			last = (subtopic_ptr)&cur->first;
		}
		if(level < curlevel)
		{
			while(level < curlevel)
			{
				if(!(cur = cur->parent))
				{
					slip_message("Level 0 keyword found that was not at the beginning of the help file.");
					wait_message();
					*retbase = NULL;
					return;
				}
				curlevel--;
			}
			for(last = (subtopic_ptr)&cur->first;last->next;last = last->next);
		}
		sub = (subtopic_ptr)imalloc(sizeof(subtopic_node));
		sub->keyword = keyword;
		sub->next = NULL;
		last->next = sub;	/* tie to current chain of subtopics */
		last = sub;
		sub->child = top = (topic_ptr)imalloc(sizeof(topic_node));
		top->first = NULL;
		top->parent = cur;
		top->topic = keyword;
		top->text = (Char *)help_get_text(fp);
	} while((level = help_get_keyword(fp,&keyword)) > 0);
/* return base of tree */
	*retbase = base;
}

