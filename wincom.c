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

#ifdef VMS
typedef struct chardesc {unsigned short length,class;Char *string;} desc_node;
typedef struct chardesc *desc;
#endif

#include "ctyp_dec.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"

static Char *closefirst = "You must close a window first.";
static Int subproc[MAX_WINDOWS];	/* process ID of windows that contain subprocesses */
static Int substat[MAX_WINDOWS];	/* completion status values for subprocesses */
static Char dont_ask = 0;	/* prevent redisplay of the Open,... prompt */

/******************************************************************************\
|Routine: wincom_special
|Callby: edit
|Purpose: This is a one-shot to prevent ED from putting up the Open,... prompt
|         is the requested file open fails.
|Arguments:
|    none
\******************************************************************************/
void wincom_special()
{
	dont_ask = 1;
}

/******************************************************************************\
|Routine: wincom
|Callby: edit
|Purpose: Handles the user interface for window management.
|Arguments:
|    none
\******************************************************************************/
void wincom()
{
	Int i,j,k,inipos,inibyt,binary,close,excess,total,explicitpos,explicitbinary;
	Char diredit,*filebuf;
	rec_ptr base;
	Char buf[512],shell[512],book[512];
#ifdef VMS
	desc_node dsc;
#endif

	save_window();
	while(1)
	{
		explicitpos = inipos = inibyt = explicitbinary = binary = 0;
		if(!(i = inquire("Open,Close,Move,Spawn",buf,sizeof(buf),1)))
		{
			paint(BOTROW,BOTROW,FIRSTCOL);
			return;
		}
		for(j = 0;isspace(buf[j]) && j < i;j++);	/* find a non-blank character */
		if(j == i)
		{
			paint(BOTROW,BOTROW,FIRSTCOL);
			return;
		}
		for(k = 0;j < i;j++)	/* left-justify starting with first non-blank */
			buf[k++] = buf[j];
		buf[k] = '\0';
		switch(buf[0])	/* k has the number of characters typed */
		{
			case 'O':
			case 'o':
				for(total = i = 0;i < NWINDOWS;i++)
					if((excess = WINDOW[i].botrow - WINDOW[i].toprow - 2) > 0)	/* there is space available here */
						total += excess;
				if(total < 4)
				{
					abort_key();
					paint(BOTROW,BOTROW,FIRSTCOL);
					bell();
					return;
				}
				if(NWINDOWS >= MAX_WINDOWS)
				{
					slip_message(closefirst);
					wait_message();
					if(dont_ask)
					{
						dont_ask = 0;
						return;
					}
					continue;
				}
				for(j = 0;j < k;j++)	/* handle O -innn  or -b or -h */
					if(buf[j] == '-')
						break;
				if(j != k)
				{
					j++;
					if(buf[j] == 'i' || buf[j] == 'I')
					{
						if(my_sscanf(buf + ++j,"%d",&i) == 1)
						{
							explicitpos = 1;
							if((inipos = i - 1) < 0)
								inipos = 0;
						}
					}
					else if(buf[j] == 'b' || buf[j] == 'B')
					{
						binary = 1;
						explicitbinary = 1;
					}
					else if(buf[j] == 'h' || buf[j] == 'H')
					{
						binary = 2;
						explicitbinary = 1;
					}
				}
				unselect();
				i = inquire("Open file",buf,sizeof(buf),1);
				strip_quotes(buf);
				envir_subs(buf);
				if(host_in_name(buf))
					dont_ask = 1;
				paint(BOTROW,BOTROW,FIRSTCOL);
				base = (rec_ptr)imalloc(sizeof(rec_node));
				base->length = 0;
				base->data = NULL;
				if(i > 0)
				{
					if(load_file(buf,base,book,&inipos,&inibyt,&diredit,binary,&filebuf) == -2)
					{
						ifree(base);
						if(dont_ask)
						{
							dont_ask = 0;
							return;
						}
						continue;
					}
					if(was_binary())
						binary = 1;
				}
				else
				{
					filebuf = NULL;
					strcpy(buf,WINDOW[CURWINDOW].filename);
					if(host_in_name(WINDOW[CURWINDOW].filename) && WINDOW[CURWINDOW].diredit)	/* don't let them think they've duped a remote dir */
					{
						ifree(base);
						if(dont_ask)
							dont_ask = 0;
						return;
					}
					if(!explicitbinary)
						binary = WINDOW[CURWINDOW].binary;
					copy_records(BASE,base,WINDOW[CURWINDOW].binary,binary);
					diredit = WINDOW[CURWINDOW].diredit;
				}
				dont_ask = 0;
				insert_window(buf,base,binary,diredit);	/* note, this routine may call set_window in order to scroll reduced windows */
				if((i > 0) && !explicitpos)
				{
					if(--inipos < 0)
						inipos = 0;
					if((i = strlen(book)))
					{
						WINDOW[CURWINDOW].bookmark = (Char *)imalloc(++i);
						strcpy(WINDOW[CURWINDOW].bookmark,book);
					}
					else
						WINDOW[CURWINDOW].bookmark = NULL;
				}
				WINDOW[CURWINDOW].filebuf = filebuf;
				WINDOW[CURWINDOW].binary = binary;
				WINDOW[CURWINDOW].diredit = diredit;
				subproc[CURWINDOW] = 0;	/* this window is not a subprocess */
				if(inipos)
				{
					down_arrow(inipos);
					if(inibyt)
						right_arrow(inibyt);
				}
				return;
			case 'C':
			case 'c':
				paint(BOTROW,BOTROW,FIRSTCOL);
				if(NWINDOWS == 2 && k == 1)
					close = 1;
				else
				{
					close = CURWINDOW;
					for(j = 1;j < k;j++)	/* characters follow the 'c' command */
					{
						if(tolower(buf[j]) == 'n')
							close++;
						else if(tolower(buf[j]) == 'l')
							close--;
					}
					if(close <= 0)
					{
						slip_message("You cannot close the main window.");
						wait_message();
						return;
					}
					if(close >= NWINDOWS)
					{
						slip_message("Window improperly selected.");
						wait_message();
						return;
					}
				}
#ifndef GNUDOS
				if(WINDOW[close].bookmark)
					update_bookmark(close);
#endif
				if(close == CURWINDOW)
					unselect();
#ifdef VMS
				if(subproc[close])
				{
					unmatchable(WINDOW[close].toprow,WINDOW[close].botrow);
					if(close == CURWINDOW)
					{
						move(WINDOW[close].toprow,1);
						ers_bottom();
					
					}
					else
					{
						for(i = WINDOW[close].toprow;i <= WINDOW[close].botrow;i++)
						{
							move(i,1);
							ers_end();
						}
					}
				}
#endif
				dir_destroy(close);
				remove_window(close);
#ifdef VMS
				if(subproc[close])
				{
					SYS$DELPRC(subproc + close,0);
					subproc[close] = 0;
					goto find_real_window;
				}
#endif
				for(i = 0;i < NMARK;i++)
				{
					if(MARKWINDOW[i] == close)
					{
						for(j = i + 1;j < NMARK;j++)
						{
							MARKWINDOW[j - 1] = MARKWINDOW[j];
							MARKREC[j - 1] = MARKREC[j];
							MARKBYT[j - 1] = MARKBYT[j];
						}
						NMARK--;
						i--;
					}
				}
				return;
			case 'M':
			case 'm':
				paint(BOTROW,BOTROW,FIRSTCOL);
				close = CURWINDOW;
				if(k > 1)
				{
					for(j = 1;j < k;j++)	/* characters follow the 'm' command */
					{
						if(tolower(buf[j]) == 'n')
							close++;
						else if(tolower(buf[j]) == 'l')
							close--;
					}
				}
				else
					if(--close < 0)
						close = NWINDOWS - 1;
				if(close < 0)
					close = 0;
				if(close >= NWINDOWS)
					close = NWINDOWS - 1;
				set_window(close);
				if(subproc[close])
				{
#ifdef VMS
					move(WINDOW[close].botrow,1);
					vtend();
					putout();
					LIB$ATTACH(subproc + close);
					vtstart();
#endif
finish_window_maybe:
					unmatchable(WINDOW[close].toprow,WINDOW[close].botrow);
					if(substat[close])	/* the subprocess completed, remove the window */
					{
						move(WINDOW[close].toprow,1);
						ers_bottom();
						remove_window(close);
						subproc[close] = 0;	/* this window is not a subprocess */
					}
#ifdef VMS
find_real_window:
#endif
					for(i = close - 1;;i--)
						if(!subproc[i])
							break;
					set_window(i);
				}
				return;
			case 'S':
			case 's':
				if(k > 1)	/* check for a shell command */
				{
					for(i = 0;i < k;i++)	/* find a space */
						if(isspace(buf[i]))
							break;
					if(i < k)	/* if space found... */
					{
						for(;i < k;i++)	/* find a non-space */
							if(!isspace(buf[k]))
								break;
						if(i < k)	/* if non-space found... */
						{
#ifdef VMS
							dsc.length = strlen(buf) - i;
							dsc.class = 0x010e;
							dsc.string = buf + i;
							substat[CURWINDOW] = 0;
							LIB$SPAWN(&dsc,0,0,0,0,subproc + CURWINDOW,substat + CURWINDOW);
#else
							envir_subs(buf + i);
							vtend();
							putout();
							system(buf + i);
#ifdef GNUDOS
							sleep(1);	/* This is because Sandmann wants to be able to press "suspend" to keep the output on the screen */
#endif
							vtstart();
#endif
							continue;
						}
					}
				}
				if(NWINDOWS >= MAX_WINDOWS)
				{
					slip_message(closefirst);
					wait_message();
					continue;
				}
				paint(BOTROW,BOTROW,FIRSTCOL);
				base = (rec_ptr)imalloc(sizeof(rec_node));
				base->length = 0;
				base->data = NULL;
				base->next = base->prev = base;
				insert_window("Subprocess",base,0,0);
				move(TOPROW,1);
				ers_bottom();
				vtend();
				putout();
#ifdef VMS
				substat[CURWINDOW] = 0;
				LIB$SPAWN(0,0,0,0,0,subproc + CURWINDOW,substat + CURWINDOW);
#else
				strcpy(shell,USERSHELL);
				strip_quotes(shell);
				envir_subs(shell);
#ifdef GNUDOS
				i = system(shell);
#else
#ifdef sgi
				sprintf(buf,"export LINES;LINES=%d;%s;",BOTROW - TOPROW + 1,shell);
#else
				sprintf(buf,"stty rows %d; %s; stty rows %d cols %d",BOTROW - TOPROW + 1,shell,NROW,NCOL);
#endif
				i = system(buf);
#endif
				substat[CURWINDOW] = 1;	/* so the window gets trashed */
#endif
				vtstart();
				close = CURWINDOW;
				goto finish_window_maybe;
		}
	}
}

/******************************************************************************\
|Routine: winsub_clean
|Callby: main
|Purpose: Removes any unkilled subprocesses.
|Arguments:
|    none
\******************************************************************************/
void winsub_clean()
{
#ifdef VMS
	Int i;
	
	for(i = 0;i < MAX_WINDOWS;i++)
	{
		if(subproc[i])
		{
			SYS$DELPRC(subproc + i,0);
			subproc[i] = 0;
		}
	}
#endif
}

/******************************************************************************\
|Routine: set_non_subp
|Callby: command
|Purpose: Flags most-recently-created window as a non-subprocess window.
|Arguments:
|    none
\******************************************************************************/
void set_non_subp()
{
	subproc[CURWINDOW] = 0;
}

