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
#include <stdlib.h>
#include <string.h>

#define MAXSTRING 64	/* the longest string of chars sent by a single keystroke */

#include "memory.h"
#include "file.h"
#include "ctyp_dec.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "emul_dec.h"
#include "handy.h"
#include "term_cmd.h"
#include "keyvals.h"

typedef struct tree_str *tree_ptr;
typedef struct tree_str {Int branches,leaf,value;Schar *chars;tree_ptr *nodes;} tree_node;

static Char **coms;
static tree_node base = {0,0,0,NULL,NULL};
static Schar history[MAXSTRING];	/* the chars in the (still-being-analyzed) keystring so far */
static Schar *next_history;	/* where the next history char goes */
static Schar *report_history;	/* pointer to next history char to be returned */
static tree_ptr cur;	/* where we are in the tree */
static Schar chrbuf = 0;	/* to receive one char from keyboard */
static Int evolving = 0;	/* whether we are still getting chars from a defined key */
static Int scroll_top,scroll_bot;	/* the last set scrolling region boundaries */
static Int recovering = 0;	/* whether we are recovering from journal */
static Int processmode = 0;	/* whether we are in silent, 'process' mode */
static Int vtstartyet = 0;
static Schar *emulbuf = NULL;	/* buffer to hold emulation characters */
static Schar *emulptr;	/* points to next emulated key */
static Schar *emulend;	/* points to stopping point */
static Int *reptbuf;	/* buffer to hold repeat counts */
static Int *reptptr;	/* points to next repeat count */
static Int *reptend;	/* points to stopping point */
static Int emulsiz = 0;	/* current size of emulation buffer */

extern void up();
extern void down();
extern void vtstart();
extern Char *exefile();
extern Char *filename();

/******************************************************************************\
|Routine: emulate_abort
|Callby: abort_key
|Purpose: To stop emulation if a stop-the-defined-key situation arises.
|Arguments:
|    none
\******************************************************************************/
void emulate_abort()
{
	if(emulptr != emulend)
	{
		emulptr = emulend;
		reptptr = reptend;
		puton();
		ref_display();
	}
}

/******************************************************************************\
|Routine: emulate_key
|Callby: autowrp edit
|Purpose: To make it as though the user typed some keys.
|Arguments:
|    val is the value of the key we pretend the user pressed.
|    repeat is the repeat count for the key.
\******************************************************************************/
void emulate_key(val,repeat)
Schar val;
Int repeat;
{
	Schar *temp,*to,*from;
	Int *ltemp,*lto,*lfrom;
	
	if(repeat != 0)
	{
		if(emulend >= emulbuf + emulsiz)
		{
/* if there is space at beginning of array, slide everything down */
			if(emulptr > emulbuf)
			{
				for(from = emulptr,to = emulbuf;from != emulend;*to++ = *from++);
				emulptr  = emulbuf;
				emulend = to;
				for(lfrom = reptptr,lto = reptbuf;lfrom != reptend;*lto++ = *lfrom++);
				reptptr  = reptbuf;
				reptend = lto;
			}
			else	/* generate new buffers and copy */
			{
/* generate new char buffer */
				temp = (Schar *)imalloc(emulsiz << 1);
				memcpy(temp,emulbuf,emulsiz);
				emulend = temp + (emulend - emulbuf);
				emulptr = temp + (emulptr - emulbuf);
				ifree(emulbuf);
				emulbuf = temp;
/* generate new repeat buffer */
				ltemp = (Int *)imalloc((emulsiz << 1) * sizeof(Int));
				memcpy(ltemp,reptbuf,emulsiz * sizeof(Int));
				reptend = ltemp + (reptend - reptbuf);
				reptptr = ltemp + (reptptr - reptbuf);
				ifree(reptbuf);
				reptbuf = ltemp;
/* increase size */
				emulsiz <<= 1;
			}
		}
		*emulend++ = val;
		*reptend++ = repeat;
/*		if(repeat > 19)    11-93 CWS bug, no way to puton()!
			putoff(); */
	}
}

/******************************************************************************\
|Routine: set_processmode
|Callby: main
|Purpose: Determines whether display is turned back on after multiply-repeated
|         defined keys.
|Arguments:
|    mode is the 'process' mode flag.
\******************************************************************************/
void set_processmode(mode)
Int mode;
{
	processmode = mode;
}

/******************************************************************************\
|Routine: trans_term_string
|Callby: init_term restore_par
|Purpose: Handles the control-character quoting convention used in the
|         terminal configuration files. Control characters that might interfere
|         with reading the file are quoted with the ascii character 0x1d.
|Arguments:
|    buf is the buffer holding the string from the configuration file.
|    l is the returned (perhaps modified) length of the processed string.
\******************************************************************************/
void trans_term_string(buf,l)
Char *buf;
Int *l;
{
	register Int k,v;
	register Char *c,*start,*end;

	for(start = c = buf,end = buf + *l;buf != end;)
	{
		if((v = *buf++) == 0x1d)
			for(k = v = 0;k < 3;k++)
				v = 10 * v + *buf++ - '0';
		*c++ = v;
	}
	*c = 0;
	*l = c - start;
}

/******************************************************************************\
|Routine: init_terminal
|Callby: main
|Purpose: Initializes the terminal for editing. Processes the user-specified
|         terminal configuration file, and sets up the screen database.
|Arguments:
|    term is the name of the terminal configuration file.
|    setup is the name of the user's setup file.
|    cnrow,cncol are buffers containing the number of lines and columns on
|            the terminal screen.
\******************************************************************************/
void init_terminal(term,setup,cnrow,cncol)
Char *term,*setup,*cnrow,*cncol;
{
	FILE *fp,*ifp,*ofp;
	Int l,i,j,b,nkeys;
	tree_ptr t,*new,newnode;
	Char buf[512],defkeyfile[512];
	Schar *newchars;
	struct stat statbuf;

	if(my_sscanf(cnrow,"%d",&NROW) != 1)
	{
		printf("Number of screen lines is illegible (specified as %s).\r\n",cnrow);
		cleanup(-1);
	}
	if(my_sscanf(cncol,"%d",&NCOL) != 1)
	{
		printf("Number of screen columns is illegible (specified as %s).\r\n",cncol);
		cleanup(-1);
	}
	if(!(fp = fopen(term,"r")))
	{
		printf("Can't open configuration file %s.\r\n",term);
		cleanup(-1);
	}
	BRAINDEAD = 1;
	coms = (Char **)imalloc((int)NCOMS * sizeof(Char *));
	for(i = 0;i < (int)NCOMS;i++)
	{
		if(!fgets(buf,sizeof(buf),fp))
		{
			printf("Error reading configuration file.\r\n");
			cleanup(-1);
		}
		l = strlen(buf) - 1;
		buf[l] = '\0';
		trans_term_string(buf,&l);
		coms[i] = (Char *)imalloc(l + 1);
		strcpy(coms[i],buf);
		if(i != (int)EOB && coms[i][0] != '~')
			BRAINDEAD = 0;
	}
	my_fscanf(fp,"%d\n",&nkeys);
	for(i = 1;i <= nkeys;i++)
	{
		if(!fgets(buf,sizeof(buf),fp))
		{
			printf("Error reading key strings from configuration file.\r\n");
			cleanup(-1);
		}
		buf[(l = strlen(buf) - 1)] = '\0';
		trans_term_string(buf,&l);
		t = &base;
		for(j = 0;j < l;j++)
		{
			for(b = 0;b < t->branches;b++)
				if(t->chars[b] == buf[j])
					break;
			if(b < t->branches)	/* match for char at current node, just advance to child node */
				t = t->nodes[b];
			else	/* no match for this char at this node */
			{
/* allocate a bigger array of tree pointers, transfer the old if any, and add the new pointer at the end */
				new = (tree_ptr *)imalloc((t->branches + 1)*sizeof(tree_ptr));
				if(t->branches > 0)
				{
					memcpy(new,t->nodes,t->branches * sizeof(tree_ptr));
					ifree(t->nodes);
				}
				t->nodes = new;
				newnode = new[t->branches] = (tree_ptr)imalloc(sizeof(tree_node));
/* set the new node up as empty */
				newnode->branches = newnode->leaf = 0;
/* allocate a bigger array for characters, transfer the old, add the new char at the end */
				newchars = (Schar *)imalloc(t->branches + 1);
				if(t->branches > 0)
				{
					memcpy(newchars,t->chars,t->branches);
					ifree(t->chars);
				}
				t->chars = newchars;
				newchars[t->branches] = buf[j];
/* one more char at this node now */
				t->branches++;
/* we now sit at new node */
				t = newnode;
			}
		}	/* get next char in string */
/* flag the leaf (pointed to by t) as a termination */
		t->leaf = 1;
		t->value = -i;
	}	/* get next key string */
	fclose(fp);
	cur = &base;	/* initialize tree search */
	next_history = history;	/* point to start of history buffer */

	emulbuf = emulptr = emulend = (Schar *)imalloc(emulsiz = 512);
	reptbuf = reptptr = reptend = (Int *)imalloc(emulsiz * sizeof(Int));

	if(!NROW)
	{
		strcpy(buf,"$ROWS");
		envir_subs(buf);
		if(strcmp(buf,"$ROWS"))
		{
			if(my_sscanf(buf,"%d",&i) == 1)
				NROW = i;
		}
	}
	if(!NCOL)
	{
		strcpy(buf,"$COLS");
		envir_subs(buf);
		if(strcmp(buf,"$COLS"))
		{
			if(my_sscanf(buf,"%d",&i) == 1)
				NCOL = i;
		}
	}
	vtstart();
	ttysetsize(&NROW,&NCOL);
	scroll_top = 1;
	scroll_bot = NROW;
	EMU_NROW = NROW;	/* set up emulation layer */
	EMU_NCOL = NCOL;
	EMU_SCREEN = (Char *)imalloc(EMU_NROW*EMU_NCOL);
	EMU_ATTRIB = (Char *)imalloc(EMU_NROW*EMU_NCOL);
	memset(EMU_SCREEN,' ',EMU_NCOL * EMU_NROW);
	memset(EMU_ATTRIB,0,EMU_NCOL * EMU_NROW);
	EMU_CURROW = -1;	/* flag that it is not defined for put */
/* do the user-specific setup file */
	strcpy(KEYFILE,setup);
	strip_quotes(KEYFILE);
	envir_subs(KEYFILE);
	if(stat(KEYFILE,&statbuf))
	{
#ifdef VMS
		if((ofp = fopen(KEYFILE,"w","ctx=rec","rfm=var","rat=cr","shr=nil")))
#else
		if((ofp = fopen(KEYFILE,"w")))
#endif
		{
			strcpy(defkeyfile,exefile());
			strcpy(filename(defkeyfile),"ed.setup");
			if((ifp = fopen(defkeyfile,"r")))
			{
				while(fgets(buf,sizeof(buf),ifp))
					fputs(buf,ofp);
				fclose(ifp);
			}
			fclose(ofp);
		}
	}
/* these have to be set to make wait_message work right (restore_par() may call slip_message/wait_message) */
	BOTROW = NROW;
	FIRSTCOL = 1;
	restore_par(-1);	/* the -1 means restore all params, not just one in particular */
}

/******************************************************************************\
|Routine: command_one
|Callby: init_term
|Purpose: Issues a terminal command that has one integer parameter.
|Arguments:
|    command is the terminal command buffer.
|    parm is the single parameter.
\******************************************************************************/
void command_one(command,parm)
Char *command;
Int parm;
{
	register Int i;
	register Char *p,*q,c;
	Char buf[128];

	if(puttest())
	{
		for(p = command,q = buf;(c = *p++);)
		{
			if(c == '%')
			{
				p++;
				if(parm > 99)
				{
					*q++ = (i = (parm / 100)) + '0';
					parm -= 100 * i;
					*q++ = (i = (parm / 10)) + '0';
					parm -= 10 * i;
					*q++ = parm + '0';
				}
				else if(parm > 9)
				{
					*q++ = (i = (parm / 10)) + '0';
					parm -= 10 * i;
					*q++ = parm + '0';
				}
				else
					*q++ = parm + '0';
			}
			else
				*q++ = c;
		}
		*q = '\0';
		put(buf);
	}
}

/******************************************************************************\
|Routine: command_two
|Callby: init_term
|Purpose: Issues a terminal command that has two integer parameters.
|Arguments:
|    command is the terminal command buffer.
|    parm1,parm2 are the parameters.
\******************************************************************************/
void command_two(command,parm1,parm2)
Char *command;
Int parm1,parm2;
{
	register Int i,j;
	register Char *p,*q,c;
	Char buf[128];

	if(puttest())
	{
		j = parm1;
		for(p = command,q = buf;(c = *p++);)
		{
			if(c == '%')
			{
				p++;
				if(j > 99)
				{
					*q++ = (i = (j / 100)) + '0';
					j -= 100 * i;
					*q++ = (i = (j / 10)) + '0';
					j -= 10 * i;
					*q++ = j + '0';
				}
				else if(j > 9)
				{
					*q++ = (i = (j / 10)) + '0';
					j -= 10 * i;
					*q++ = j + '0';
				}
				else
					*q++ = j + '0';
				j = parm2;
			}
			else
				*q++ = c;
		}
		*q = '\0';
		put(buf);
	}
}

/******************************************************************************\
|Routine: vtstart
|Callby: init_term wincom
|Purpose: Sets the terminal to a known state.
|Arguments:
|    none
\******************************************************************************/
void vtstart()
{
	ttystart();
	if(!BRAINDEAD) put(coms[VTSTART]);
	vtstartyet = 1;
	EMU_CURATT = 0;
}

/******************************************************************************\
|Routine: move
|Callby: bigger_win cfg command edit fix_scroll help imalloc init_term inquire insert insert_win killer load_key match_paren paint paint_window parse_fnm ref_window remove_win scroll_down scroll_up slip_message trim unselect wincom
|Purpose: Positions the cursor.
|Arguments:
|    row is the line number on the screen. The top line is line 1.
|    col is the column number. The leftmost column is column 1.
\******************************************************************************/
void move(row,col)
Int row,col;
{
	col -= FIRSTCOL - 1;
	row = max(1,min(EMU_NROW,row));
	col = max(1,min(EMU_NCOL,col));
	if(coms[MOVE][0] != '~')
		command_two(coms[MOVE],row,col);
	EMU_CURROW = --row;
	EMU_CURCOL = --col;
}

/******************************************************************************\
|Routine: marge
|Callby: bigger_win cfg command edit help init_term insert insert_win killer parse_fnm set_window smaller_win
|Purpose: Sets the scrolling region on the screen.
|Arguments:
|    row1 is the top line of the scrolling region.
|    row2 is the bottom line of the scrolling region.
\******************************************************************************/
void marge(row1,row2)
Int row1,row2;
{
	if(coms[MARGE][0] != '~')
		command_two(coms[MARGE],row1,row2);
	scroll_top = max(1,min(EMU_NROW,row1));
	scroll_bot = max(1,min(EMU_NROW,row2));
}

/******************************************************************************\
|Routine: del_line
|Callby: bigger_win init_term insert insert_win killer scroll_up
|Purpose: Deletes lines, causing lines below to scroll up.
|Arguments:
|    lines is the number of lines to delete.
\******************************************************************************/
void del_line(lines)
Int lines;
{
	register Int i,from,to,save;

	i = scroll_bot - EMU_CURROW;
	lines = max(1,min(i,lines)); 
	if(coms[DEL_LINE][0] == '~')
	{
		if(coms[MARGE][0] == '~')
		{
			for(i = EMU_CURROW + lines;i < scroll_bot;i++)
			{
				from = i * EMU_NCOL;
				to = from - EMU_NCOL * lines;
				memcpy(EMU_SCREEN + to,EMU_SCREEN + from,EMU_NCOL);
				memcpy(EMU_ATTRIB + to,EMU_ATTRIB + from,EMU_NCOL);
			}
			for(i = scroll_bot - 1;i >= scroll_bot - lines;i--)
			{
				memset(EMU_SCREEN + i * EMU_NCOL,' ',EMU_NCOL);
				memset(EMU_ATTRIB + i * EMU_NCOL,0,EMU_NCOL);
			}
			if(!BRAINDEAD) ref_display();
		}
		else
		{
			save = scroll_top;
			marge(EMU_CURROW + 1,scroll_bot);
			move(scroll_bot,1);
			for(i = 0;i < lines;i++)
				down();
			marge((scroll_top = save),scroll_bot);
		}
	}
	else
	{
		if(coms[MARGE][0] == '~')
		{
			lines = max(1,min(lines,scroll_bot - EMU_CURROW));
			command_one(coms[DEL_LINE],lines);
			if(scroll_bot != EMU_NROW)
			{
				Int saverow,savecol;
				
				saverow = EMU_CURROW + 1;
				savecol = EMU_CURCOL + 1;
				move(scroll_bot - lines + 1,1);
				if(coms[INS_LINE][0] != '~')
					command_one(coms[INS_LINE],lines);
				move(saverow,savecol);
			}
		}
		else
			command_one(coms[DEL_LINE],lines);
		for(i = EMU_CURROW + lines;i < scroll_bot;i++)
		{
			from = i * EMU_NCOL;
			to = from - EMU_NCOL * lines;
			memcpy(EMU_SCREEN + to,EMU_SCREEN + from,EMU_NCOL);
			memcpy(EMU_ATTRIB + to,EMU_ATTRIB + from,EMU_NCOL);
		}
		for(i = scroll_bot - 1;i >= scroll_bot - lines;i--)
		{
			memset(EMU_SCREEN + i * EMU_NCOL,' ',EMU_NCOL);
			memset(EMU_ATTRIB + i * EMU_NCOL,0,EMU_NCOL);
		}
	}
}

/******************************************************************************\
|Routine: ins_line
|Callby: bigger_win init_term insert killer scroll_down
|Purpose: Inserts lines at the cursor, causing lines below to scroll down.
|Arguments:
|    lines is the number of lines to insert.
\******************************************************************************/
void ins_line(lines)
Int lines;
{
	register Int i,from,to,save;

	i = scroll_bot - EMU_CURROW;
	lines = max(1,min(i,lines)); 
	if(coms[INS_LINE][0] == '~')
	{
		if(coms[MARGE][0] == '~')
		{
			for(i = scroll_bot - 1 - lines;i >= EMU_CURROW;i--)
			{
				from = i * EMU_NCOL;
				to = from + EMU_NCOL * lines;
				memcpy(EMU_SCREEN + to,EMU_SCREEN + from,EMU_NCOL);
				memcpy(EMU_ATTRIB + to,EMU_ATTRIB + from,EMU_NCOL);
			}
			for(i = EMU_CURROW;i < EMU_CURROW + lines;i++)
			{
				memset(EMU_SCREEN + i * EMU_NCOL,' ',EMU_NCOL);
				memset(EMU_ATTRIB + i * EMU_NCOL,0,EMU_NCOL);
			}
			if(!BRAINDEAD) ref_display();
		}
		else
		{
			save = scroll_top;
			marge(EMU_CURROW + 1,scroll_bot);
			move(scroll_top,1);
			for(i = 0;i < lines;i++)
				up();
			marge((scroll_top = save),scroll_bot);
		}
	}
	else
	{
		if(coms[MARGE][0] == '~')
		{
			lines = max(1,min(lines,scroll_bot - EMU_CURROW));
			if(scroll_bot < EMU_NROW)
			{
				Int saverow,savecol;
				
				saverow = EMU_CURROW + 1;
				savecol = EMU_CURCOL + 1;
				move(scroll_bot - lines + 1,1);
				if(coms[DEL_LINE][0] != '~')
					command_one(coms[DEL_LINE],lines);
				move(saverow,savecol);
			}
		}
		command_one(coms[INS_LINE],lines);
		for(i = scroll_bot - 1 - lines;i >= EMU_CURROW;i--)
		{
			from = i * EMU_NCOL;
			to = from + EMU_NCOL * lines;
			memcpy(EMU_SCREEN + to,EMU_SCREEN + from,EMU_NCOL);
			memcpy(EMU_ATTRIB + to,EMU_ATTRIB + from,EMU_NCOL);
		}
		for(i = EMU_CURROW;i < EMU_CURROW + lines;i++)
		{
			memset(EMU_SCREEN + i * EMU_NCOL,' ',EMU_NCOL);
			memset(EMU_ATTRIB + i * EMU_NCOL,0,EMU_NCOL);
		}
	}
}

/******************************************************************************\
|Routine: up
|Callby: init_term
|Purpose: Moves the cursor up one line. Scrolls down if the cursor is at the
|         top of the scrolling region.
|Arguments:
|    none
\******************************************************************************/
void up()
{
	if(coms[MARGE][0] == '~' && EMU_CURROW == scroll_top - 1)
	{
		ins_line(1);
		return;
	}
	if(coms[UP][0] != '~')
		put(coms[UP]);
	if(EMU_CURROW == scroll_top - 1)
	{
		Int i,from,to;
		
		for(i = scroll_bot - 2;i >= EMU_CURROW;i--)
		{
			from = i * EMU_NCOL;
			to = from + EMU_NCOL;
			memcpy(EMU_SCREEN + to,EMU_SCREEN + from,EMU_NCOL);
			memcpy(EMU_ATTRIB + to,EMU_ATTRIB + from,EMU_NCOL);
		}
		memset(EMU_SCREEN + EMU_CURROW * EMU_NCOL,' ',EMU_NCOL);
		memset(EMU_ATTRIB + EMU_CURROW * EMU_NCOL,0,EMU_NCOL);
	}
	else
	{
		if(--EMU_CURROW < 0)
			EMU_CURROW = 0;
	}
}

/******************************************************************************\
|Routine: down
|Callby: init_term
|Purpose: Moves the cursor down one line. Scrolls up if the cursor is at the
|         bottom of the scrolling region.
|Arguments:
|    none
\******************************************************************************/
void down()
{
	if(coms[MARGE][0] == '~' && EMU_CURROW == scroll_bot - 1)
	{
		Int savecol;

		savecol = EMU_CURCOL + 1;
		move(scroll_top,1);
		del_line(1);
		move(scroll_bot,savecol);
		return;
	}
	if(coms[DOWN][0] != '~')
		put(coms[DOWN]);
	if(EMU_CURROW == scroll_bot - 1)
	{
		Int i,from,to;
		
		for(i = scroll_top - 1;i < scroll_bot - 1;i++)
		{
			to = i * EMU_NCOL;
			from = to + EMU_NCOL;
			memcpy(EMU_SCREEN + to,EMU_SCREEN + from,EMU_NCOL);
			memcpy(EMU_ATTRIB + to,EMU_ATTRIB + from,EMU_NCOL);
		}
		memset(EMU_SCREEN + (scroll_bot - 1) * EMU_NCOL,' ',EMU_NCOL);
		memset(EMU_ATTRIB + (scroll_bot - 1) * EMU_NCOL,0,EMU_NCOL);
	}
	else
	{
		if(++EMU_CURROW >= EMU_NROW)
			EMU_CURROW = EMU_NROW - 1;
	}
}

/******************************************************************************\
|Routine: reverse
|Callby: command edit help inquire load_key match_paren ref_window remove_win select slip_message
|Purpose: Turns on reverse video.
|Arguments:
|    none
\******************************************************************************/
void reverse()
{
	if(coms[REVERSE][0] != '~')
		put(coms[REVERSE]);
	EMU_CURATT |= 1;
}

/******************************************************************************\
|Routine: normal
|Callby: command edit help inquire load_key match_paren ref_window remove_win select slip_message
|Purpose: Turns off reverse video, bold, underline, flash.
|Arguments:
|    none
\******************************************************************************/
void normal()
{
	if(coms[NORMAL][0] != '~')
		put(coms[NORMAL]);
	EMU_CURATT = 0;
}

/******************************************************************************\
|Routine: bell
|Callby: command edit wincom
|Purpose: Beeps the terminal.
|Arguments:
|    none
\******************************************************************************/
void bell()
{
	if(coms[BELL][0] != '~')
		put(coms[BELL]);
	else
		ring_bell();
}

/******************************************************************************\
|Routine: ers_screen
|Callby: cfg ref_display
|Purpose: Erases the entire screen.
|Arguments:
|    none
\******************************************************************************/
void ers_screen()
{
	if(coms[ERS_SCREEN][0] != '~')
		put(coms[ERS_SCREEN]);
	memset(EMU_SCREEN,' ',EMU_NCOL * EMU_NROW);
	memset(EMU_ATTRIB,0,EMU_NCOL * EMU_NROW);
}

/******************************************************************************\
|Routine: ers_end
|Callby: edit help init_term inquire load_key paint paint_window parse_fnm ref_window remove_win slip_message
|Purpose: Erases everything to the right of the cursor on the current line.
|Arguments:
|    none
\******************************************************************************/
void ers_end()
{
	register Int off;
	register Char *s,*a,*end;

	off = EMU_CURROW * EMU_NCOL + EMU_CURCOL;
	s = EMU_SCREEN + off;
	a = EMU_ATTRIB + off;
	end = s + EMU_NCOL - EMU_CURCOL;
	while(s != end)
	{
		if(*s != ' ' || *a != 0)
			break;
		s++;
		a++;
	}
	if(s != end)
	{
		if(coms[ERS_END][0] != '~')
			put(coms[ERS_END]);
		memset(EMU_SCREEN + off,' ',EMU_NCOL - EMU_CURCOL);
		memset(EMU_ATTRIB + off,0,EMU_NCOL - EMU_CURCOL);
	}
}

/******************************************************************************\
|Routine: right
|Callby: paint paint_window put
|Purpose: Moves the cursor to the right.
|Arguments:
|    cols is the number of columns to move.
\******************************************************************************/
void right(cols)
Int cols;
{
	if((cols = min(EMU_NCOL - EMU_CURCOL - 1,cols)))
	{
		if(coms[RIGHT][0] != '~')
			command_one(coms[RIGHT],cols);
		EMU_CURCOL += cols;
	}
}

/******************************************************************************\
|Routine: left
|Callby: not called
|Purpose: Moves the cursor to the left.
|Arguments:
|    cols is the number of columns to move.
\******************************************************************************/
void left(cols)
Int cols;
{
	if((cols = min(EMU_CURCOL,cols)))
	{
		if(coms[LEFT][0] != '~')
			command_one(coms[LEFT],cols);
		EMU_CURCOL -= cols;
	}
}

/******************************************************************************\
|Routine: next
|Callby: cfg help paint paint_window parse_fnm
|Purpose: Moves the cursor to the beginning of the next line on the screeen.
|         Scrolls up if the cursor is at the bottom of the scrolling region.
|Arguments:
|    none
\******************************************************************************/
void next()
{
	if(coms[NEXT][0] != '~')
		put(coms[NEXT]);
	EMU_CURCOL = 0;
	if(EMU_CURROW == scroll_bot - 1)
	{
		Int i,from,to;
		
		for(i = scroll_top - 1;i < scroll_bot - 1;i++)
		{
			to = i * EMU_NCOL;
			from = to + EMU_NCOL;
			memcpy(EMU_SCREEN + to,EMU_SCREEN + from,EMU_NCOL);
			memcpy(EMU_ATTRIB + to,EMU_ATTRIB + from,EMU_NCOL);
		}
		memset(EMU_SCREEN + (scroll_bot - 1) * EMU_NCOL,' ',EMU_NCOL);
		memset(EMU_ATTRIB + (scroll_bot - 1) * EMU_NCOL,0,EMU_NCOL);
	}
	else
	{
		if(++EMU_CURROW >= EMU_NROW)
			EMU_CURROW = EMU_NROW - 1;
	}
}

/******************************************************************************\
|Routine: eob
|Callby: insert killer paint paint_window unselect
|Purpose: Displays the end-of-buffer string (default is [eob]).
|Arguments:
|    none
\******************************************************************************/
void eob()
{
	putz(coms[EOB]);
}

/******************************************************************************\
|Routine: cr
|Callby: cfg help inquire paint paint_window unselect
|Purpose: Moves the cursor to the beginning of the current line.
|Arguments:
|    none
\******************************************************************************/
void cr()
{
	if(coms[CR][0] != '~')
		put(coms[CR]);
	EMU_CURCOL = 0;
}

/******************************************************************************\
|Routine: ers_bottom
|Callby: insert_win wincom
|Purpose: Erases from the cursor to the bottom of the screen.
|Arguments:
|    none
\******************************************************************************/
void ers_bottom()
{
	register Int off,ndel;
	
	if(coms[ERS_BOTTOM][0] != '~')
		put(coms[ERS_BOTTOM]);
	off = EMU_CURROW * EMU_NCOL + EMU_CURCOL;
	ndel = EMU_NROW * EMU_NCOL - off;
	memset(EMU_SCREEN + off,' ',ndel);
	memset(EMU_ATTRIB + off,0,ndel);
}

/******************************************************************************\
|Routine: vtend
|Callby: parse_fnm restore_par wincom
|Purpose: Resets the terminal when ED terminates.
|Arguments:
|    none
\******************************************************************************/
void vtend()
{
	if(vtstartyet && !BRAINDEAD) put(coms[VTEND]);
	if(BRAINDEAD)
	{
		EMU_CURCOL = 0;
		ers_end();
		if(--EMU_CURROW < 0) EMU_CURROW = 0;
	}
	putout();
	ttyend();
	EMU_CURATT = 0;
}

/******************************************************************************\
|Routine: unmatchable
|Callby: wincom
|Purpose: Renders an area on the screen 'unmatchable' to insure new data bound
|         there is actually displayed.
|Arguments:
|    top is the top line of the area to be made unmatchable.
|    bot is the bottom line of the area.
\******************************************************************************/
void unmatchable(top,bot)
Int top,bot;
{
	memset(EMU_ATTRIB + (top - 1) * EMU_NCOL,-1,(bot - top + 1) * EMU_NCOL);
}

/******************************************************************************\
|Routine: get_next_key
|Callby: edit help inquire load_key slip_message
|Purpose: Gets the next character in the input stream (either from the terminal
|         or the journal file). Returns the repeat count to be applied to the
|         key.
|Arguments:
|    buf is the return key value.
\******************************************************************************/
Int get_next_key(buf)
Schar *buf;
{
	register Int b,i;
	Int repeat;

/* if emulated keys await, do that */
	if(emulptr < emulend)
	{
		*buf = *emulptr++;
		return(*reptptr++);
	}
/* if a defined key is evolving, get from there */
	if(evolving)
	{
		if((evolving = next_key(&chrbuf,&repeat)))
		{
			*buf = chrbuf;
			return(repeat);
		}
		if(was_putoff() && !processmode)
		{
			puton();
			ref_display();
			move(CURROW,CURCOL);
		}
	}
/* if recovery is in progress, get from there */
	if(recovering)
	{
		unjournal(buf,&repeat);
		if(repeat > 0)
		{
			if((evolving = defined_key(*buf,repeat)))
			{
				next_key(&chrbuf,&repeat);
				*buf = chrbuf;
			}
			return(repeat);
		}
		if(processmode)
		{
			printf("That journal file terminates abnormally.\r\n");
			cleanup(-1);
		}
		recovering = 0;
	}
/* dump the output buffer and prepare */
	putout();
	repeat = 1;
/* report_history contains input bytes that only partially matched a known sequence */
	if(report_history)
	{
		if(report_history == next_history)	/* we hit the end of the history buffer, wait for more input */
		{
			next_history = history;
			report_history = NULL;
			cur = &base;
		}
		else
		{
			*buf = *report_history++;
			journal(*buf,1);
			if((evolving = defined_key(*buf,1)))	/* report from history buffer */
			{
				next_key(&chrbuf,&repeat);
				*buf = chrbuf;
			}
			return(repeat);
		}
	}
/* no other form of input, get from the terminal */
	while(1)
	{
		ttyget(&chrbuf);	/* get byte from user */
traverse:
		for(b = 0;b < cur->branches;b++)	/* see if it is known at current node */
			if(cur->chars[b] == chrbuf)
				break;
		if(b < cur->branches)	/* it exists at this node */
		{
			*next_history++ = chrbuf;	/* store in history buffer, in case we get a miss later */
			cur = cur->nodes[b];	/* advance to subnode for that byte */
			if(cur->leaf && !cur->branches)	/* it is a leaf, report it */
			{
				*buf = cur->value;	/* return value */
				next_history = history;	/* clear the history buffer */
				cur = &base;	/* initialize the tree search */
				journal(*buf,repeat);
				if((evolving = defined_key(*buf,repeat)))
				{
					next_key(&chrbuf,&repeat);
					*buf = chrbuf;
				}
				return(repeat);
			}
		}
		else	/* a miss */
		{
			if(cur == &base)	/* special, simple, handling for top-level miss (a common occurrence) */
			{
				if((chrbuf & 0x80) && chrbuf >= KEY_SCROLLLEFT)	/* last in edit */
				{
					*buf = KEY_SPECIALINSERT;	/* NCS support for 8 bit chars */
					repeat = chrbuf;
				}
				else
					*buf = chrbuf;
				journal(*buf,repeat);
				if((evolving = defined_key(*buf,repeat)))
				{
					next_key(&chrbuf,&repeat);
					*buf = chrbuf;
					return(repeat);
				}
			}
			else
			{
				if(cur->leaf)	/* previous sequence was allowed to (optionally) terminate, stick its value on stack */
				{
					if(cur->value == KEY_GOLD)
					{
						if(isdigit(chrbuf))	/* special handling for GOLD */
						{
							repeat = 0;
							while(isdigit(chrbuf))  /* acquire digits and build up repeat count */
							{
								repeat = 10 * repeat + chrbuf - '0';
								ttyget(&chrbuf);
							}
							cur = &base;
							next_history = history;
							goto traverse;
						}
						else	/* flag gold-<key> with negative repeat count */
						{
							i = toupper(chrbuf);
							if(i >= 'A' && i <= 'Z')
								chrbuf = i + 0x80 - 'A';
							cur = &base;
							next_history = history;
							goto traverse;
						}
					}
					history[0] = cur->value;
					next_history = history+1;
				}
				*next_history++ = chrbuf;
				report_history = history + 1;
				*buf = history[0];
				journal(*buf,repeat);
				if((evolving = defined_key(*buf,repeat)))
				{
					next_key(&chrbuf,&repeat);
					*buf = chrbuf;
				}
			}
			return(repeat);
		}
	}
}

/******************************************************************************\
|Routine: set_recover
|Callby: journal
|Purpose: Sets a flag that tells get_next_key to get characters from the
|         journal file instead of the terminal.
|Arguments:
|    none
\******************************************************************************/
void set_recover()
{
	recovering = 1;
}

/******************************************************************************\
|Routine: get_eob_char
|Callby: select
|Purpose: Returns the first character of the user's <eob> string.
|Arguments:
|    none
\******************************************************************************/
Int get_eob_char()
{
	register Int i;
	
	if(!(i = coms[EOB][0]))
		i = ' ';
	return(i);
}

