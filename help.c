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
#include "ctyp_dec.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "library.h"

static topic_ptr base = NULL;

extern Char *exefile();
extern Char *filename();

/******************************************************************************\
|Routine: help_disp
|Callby: help
|Purpose: Displays help text.
|Arguments:
|    cur is the topic containing the text to be displayed.
\******************************************************************************/
void help_disp(cur)
topic_ptr cur;
{
	Int i;
	Char *p,*q;
	Schar key;
	
	next();
	putz(cur->topic);
	next();
	i = 3;
	p = (char *)cur->text;
	while((q = strchr(p,'\n')))
	{
		*q = '\0';
		putz(p);
		next();
		*q = '\n';
		if(!*(p = ++q))
			return;
		if(++i >= NROW)
		{
			reverse();
			putz(" ==<space> to continue,b to back up,q to quit==> ");
			normal();
			get_next_key(&key);
			cr();
			ers_end();
			if(tolower(key) == 'q' || key == 3 || key == 25)	/* emulate more quit command */
				return;
			if(tolower(key) == 'b')	/* emulate more backup command */
			{
				q = p;
				for(i = NROW * 5 / 3;i--;)
				{
					if(q == (Char *)cur->text)
						break;
					q--;
					while(*--q != '\n')
						if(q == (Char *)cur->text)
							break;
					if(q == (Char *)cur->text)
						break;
					q++;
				}
				p = q;
			}
			i = 1;
		}
	}
}

/******************************************************************************\
|Routine: help_keywords
|Callby: help
|Purpose: Displays the keywords of additional topics that are available.
|Arguments:
|    cur is the topic for which additional subtopics might be displayed.
\******************************************************************************/
void help_keywords(cur)
topic_ptr cur;
{
	Char buf[512],*p;
	Int col,i,l,size;
	subtopic_ptr scan;

	col = 0;
	for(scan = cur->first;scan != 0;scan = scan->next)
	{
		if(!col)
		{
			putz("Additional information available:");
			next();
			next();
		}
		l = strlen(scan->keyword);
		if(l <= 10)
			size = 12;
		else
			size = 24;
		if(col + size >= NCOL)
		{
			putz(buf);
			next();
			col = 0;
		}
		sprintf(buf + col,"  %s",scan->keyword);
		for(i = 0,p = buf + col + l + 2;i < size - (l + 2);i++)
			*p++ = ' ';
		*p++ = '\0';
		col += size;
	}
	if(col != 0)
	{
		putz(buf);
		next();
	}
	next();
}

/******************************************************************************\
|Routine: help
|Callby: command
|Purpose: Implements VMS-style online help.
|Arguments:
|    token is a string that may contain keywords to be searched for.
\******************************************************************************/
void help(token)
Char *token;
{
	FILE *fp;
	Int j,k,l,ncom,hits,save_botrow;
	topic_ptr cur,temp;
	subtopic_ptr scan;
	Char *c;
	Char helpfile[512],prompt[256],buf[512],tok[128],commands[64][5],hit[128];	/* no more than 128 subtopics are allowed */

/* load the help file if it isn't already loaded */
	if(!base)
	{
		strcpy(helpfile,exefile());
		strcpy(filename(helpfile),"ed.hlp");
		if(!(fp = fopen(helpfile,"r")))
		{
			sprintf(buf,"Unable to open the help file '%s'.",helpfile);
			slip_message(buf);
			wait_message();
			return;
		}
		help_load(fp,&base);
		fclose(fp);
	}
	if(!base)
		return;
/* remove trailing whitespace */
	for(c = token + strlen(token);isspace(*--c);)
		if(c == token)
		{
			c--;
			break;
		}
	*++c = '\0';
/* prepare for search */
	marge(1,NROW);
	move(NROW,1);
	save_botrow = BOTROW;
	BOTROW = NROW;
	next();
	putout();
	cur = base;
	if(!*token)
	{
		help_disp(cur);	/* display help text for this node in tree */
		help_keywords(cur);	/* display keywords for additional info */
	}
	while(1)
	{
		if(!cur->first)	/* no additional exploration possible from here, pop up in tree */
		{
			cur = cur->parent;
			continue;
		}
		if(!*token)	/* if no more input is in the buffer, ask for more */
		{
			c = prompt + (sizeof(prompt) - sizeof("subtopic? ") - 1); /* build the prompt string backwards */
			strcpy(c,"subtopic? ");
			for(temp = cur;temp;temp = temp->parent)
			{
				*--c = ' ';
				l = strlen(temp->topic);
				c -= l;
				memcpy(c,temp->topic,l);
			}
			l = inquire(c,buf,sizeof(buf),1);	/* get user keyword prompt response */
			token = buf;
		}
/* parse the next token from whichever input */
		while(isspace(*token))
			token++;
		c = tok;
		while(!isspace(*token) && *token)
			*c++ = *token++;
		*c = '\0';
		l = c - tok;
		while(isspace(*token))	/* this is so we can decide whether to display help yet below */
			token++;
/* no input means pop up or out */
		if(!l)
		{
			if(!(cur = cur->parent))	/* he hits return, go up in tree or exit if at root */
			{
				marge(TOPROW,BOTROW = save_botrow);
				ref_display();
				return;
			}
			continue;
		}
/* redisplay keywords if they enter ? */
		c = tok;	/* this is expected in the keyword comparisons below */
		if(*c == '?')
		{
			help_keywords(cur);	/* display keywords for additional info */
			continue;
		}
/* load all the keywords at this level into the array */
		for(ncom = 0,scan = cur->first;scan != 0;scan = scan->next)
		{
			memcpy(commands[ncom],scan->keyword,4);
			commands[ncom++][4] = '\0';
		}
		memset(hit,1,ncom);
		for(j = 0;j < 4;j++)	/* examine four columns in command array */
		{
			for(k = 0;k < ncom;k++)	/* knock out mismatches */
				if(tolower(commands[k][j]) != tolower(*c))
					hit[k] = 0;
			for(hits = k = 0;k < ncom;k++)	/* count remaining matches */
				hits += hit[k];
			if(!hits)
			{
				sprintf(buf,"'%s' doesn't match any of the choices.",tok);
				putz(buf);
				next();
				next();
				help_keywords(cur);
				*token = '\0';
				break;
			}
			else if(hits == 1)
			{
				for(scan = cur->first,c = hit;!*c++;scan = scan->next);	/* find the entry for their keyword */
				cur = scan->child;
				if(!*token)
				{
					help_disp(cur);	/* display help text for this node in tree */
					help_keywords(cur);	/* display keywords for additional info */
				}
				break;
			}
			c++;
			if(!*c || isspace(*c))
			{
				sprintf(buf,"'%s' is ambiguous, please supply more characters.",tok);
				putz(buf);
				next();
				next();
				help_keywords(cur);
				*token = '\0';
				break;
			}
		}
	}
}

