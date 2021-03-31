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

static buf_node charbuf = {1,1,0,0};	/* single-char buffer for insertion of single chars */
static rec_node charrec = {0,0,1,0};
static Char character;
static buf_node multibuf = {1,1,0,0};	/* buffer for insertion of repeated chars */
static rec_node multirec = {0,0,0,0};
static buf_node pagebuf = {0,1,0,0};	/* buffer to hold page delimiter string during page moves */
static Int specify_position = 0;	/* flags a user- or bookmark-specified position in file (-innn) */
static Int specify_byt = 0;	/* flags a bookmark-specified byte offset in record */
static Int dirpos[128];	/* record numbers in directory listings */
static Char *greptitle[128];	/* titles of grep windows (used when popping back into them) */
static rec_ptr grepdata[128];	/* contents of grep windows */

extern rec_ptr dir_find();
#ifndef NO_FTP
extern Char *host_filename();
#endif

/******************************************************************************\
|Routine: dir_depth
|Callby: edit
|Purpose: Calculates the depth of a directory on a filesystem.
|Arguments:
|    dir is the directory specification.
\******************************************************************************/
Int dir_depth(dir)
Char *dir;
{
	Char *p,c;
	Int curdirpos,ftpsystem;

	ftpsystem = ftp_system(dir);
	if(ftpsystem == 'V' || ftpsystem == 'I')
	{	
		for(p = dir + 1;(c = *p++);)
			if(c == '[' || c == '<')
				break;
		if(!strncmp(p,"000000]",strlen("000000]")))
			curdirpos = 0;
		else
		{
			for(curdirpos = 1;(c = *p++);)
				if(c == '.')
					curdirpos++;
				else if(c == ']' || c == '>')
					break;
		}
	}
	else
	{
		for(p = dir + 1,curdirpos = 0;(c = *p++);curdirpos += (c == '/'));	/* count the '/' characters (past the first one) */
	}
	return(curdirpos);
}

/******************************************************************************\
|Routine: position_buffer
|Callby: edit
|Purpose: Sets up a buffer to put the cursor at a specified position.
|Arguments:
|    rec is the line number to start the cursor out on. Line 1 is the first line
|        in the file.
|    byt is the byte offset in that record.
\******************************************************************************/
void position_buffer(rec,byt)
Int rec,byt;
{
	Int i;
	
	if((CURREC = BASE->next) != BASE)	/* if it isn't an empty buffer */
	{
		while(--rec)	/* advance in buffer as directed */
		{
			if(CURREC == BASE)
				break;
			CURREC = CURREC->next;
		}
		for(TOPREC = CURREC,i = TOPLIM - TOPROW;i--;TOPREC = TOPREC->prev)	/* try to back up to leave margin at top */
			if(TOPREC == BASE->next)
				break;
		for(i = BOTROW - TOPROW,BOTREC = TOPREC;i--;BOTREC = BOTREC->next)	/* establish BOTREC */
			if(BOTREC == BASE)
				break;
		while(i-- >= 0)	/* deal with being too close to the <eob> */
		{
			if(TOPREC == BASE->next)
				break;
			TOPREC = TOPREC->prev;
		}
		for(BOTREC = TOPREC;BOTREC != CURREC;BOTREC = BOTREC->next,CURROW++);
		BOTREC = TOPREC;
		CURBYT = byt;
		if(CURBYT > CURREC->length)
			CURBYT = CURREC->length;
		CURCOL = get_column(CURREC,CURBYT);
	}
	else	/* empty buffer */
		TOPREC = BOTREC = CURREC = BASE;
}

/******************************************************************************\
|Routine: edit_init_pos
|Callby: main parse_fnm
|Purpose: Sets the initial position in a file.
|Arguments:
|    pos is the line number to start the cursor out on. Line 1 is the first line in the file.
\******************************************************************************/
void edit_init_pos(pos)
Int pos;
{
	specify_position = pos;
}

/******************************************************************************\
|Routine: edit_init_byt
|Callby: main
|Purpose: Sets the byte offset when initial positioning occurs through a
|         bookmark.
|Arguments:
|    pos is the byte offset to start out with.
\******************************************************************************/
void edit_init_byt(pos)
Int pos;
{
	specify_byt = pos;
}

/******************************************************************************\
|Routine: edit
|Callby: main
|Purpose: Implements the user interface.
|Arguments:
|    fname is the name of the file being edited.
|    multiplefiles is a flag that -m appeared on the command line.
|    comm is the remainder of the command line.
|    commlen is the length of the command buffer.
\******************************************************************************/
void edit(fname,multiplefiles,comm,commlen)
Char *fname;	/* the name of the file we're editing */
Char *multiplefiles;	/* flags -m on command line */
Char *comm;	/* buffer containing remainder of command line (examined if -m specified) */
Int commlen;	/* length of command buffer */
{
/* this table is a list of flags that tell whether a given key is allowed
   when pressed in a diredit buffer. the 128th entry corresponds to NULL,
   with entries going negative and positive from there.
*/
	static Schar diredit_keymodes[256] =
	{
/*      . a b c d e f g h i j k l m n o p q r s t u v w x y z [ \ ] ^ _ */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,0,0,0,1,0,1,1,
		1,1,0,1,0,0,1,1,1,1,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,0,1,1,0,1,0,1,1,1,1,1,1,1,1,1,0,1,1,0,1,1,1,1,1,
		0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* the zero is SPACE */
		1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* the zeroes are A and B */
		1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* the zeroes are a and b */
	};
	static Schar *diredit_keymode = diredit_keymodes + 128;
	static Schar group_keymodes[256] =
	{
/*      . a b c d e f g h i j k l m n o p q r s t u v w x y z [ \ ] ^ _ */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,0,0,0,1,0,1,1,
		1,1,0,1,0,0,1,1,1,1,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,0,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,	/* the zeroes are R and V */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,	/* the zeroes are r and v */
	};
	static Schar *group_keymode = group_keymodes + 128;
	static Schar article_keymodes[256] =
	{
/*      . a b c d e f g h i j k l m n o p q r s t u v w x y z [ \ ] ^ _ */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,0,0,0,1,0,1,1,
		1,1,0,1,0,0,1,1,1,1,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,0,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,	/* the zeroes are P, R and V */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,	/* the zeroes are p, r and v */
	};
	static Schar *article_keymode = article_keymodes + 128;
	
	Int i,j,k,n,deliberate,was_displaying,artnum,read,close,bodycount;
	Int byte,offset,record,searchlength,repeat;
	Int laststatcol,newstatcol,excess,total,bookpos,bookbyt,curdirpos,upmove,binary;
	Char buf[512],editfile[512],recfile[512],bookmark[512],ftpfile[512],artcom[512];
	Char recover,process,*p,bdummy,*filebuf,diredit;
	Schar term,keybuf;
	Uchar *casetable;
	rec_ptr base,rec,rec1,rec2,new,save_rec;
	Char *q;

	laststatcol = NCOL;
	charbuf.first = &charrec;	/* initialize single char buffer */
	charbuf.last = &charrec;
	charrec.next = (rec_ptr)&charbuf;
	charrec.prev = (rec_ptr)&charbuf;
	charrec.data = &character;

	multibuf.first = &multirec;	/* initialize repeated-char buffer */
	multibuf.last = &multirec;
	multirec.next = (rec_ptr)&multibuf;
	multirec.prev = (rec_ptr)&multibuf;

	pagebuf.first = (rec_ptr)&pagebuf.first;	/* initialize page delimiter buffer */
	pagebuf.last = (rec_ptr)&pagebuf.first;

	TOPROW = 2;	/* set up the screen-oriented context */
	BOTROW = NROW;
	calc_limits(TOPROW,BOTROW,&TOPLIM,&BOTLIM);
	CURROW = 2;
	CURCOL = 1;
	FIRSTCOL = 1;
	WANTCOL = 1;
	NWINDOWS = 0;
	CURWINDOW = new_window(fname);	/* this increments NWINDOWS */
	DIRECTION = 1;
	SELREC = 0;
	deliberate = 0;	/* no deliberate sideways scrolling in effect */
	memset(dirpos,0,sizeof(dirpos));	/* dirpos[i] = 0 means we haven't visited it yet */
	HEXMODE = (WINDOW[CURWINDOW].binary == 2);

	CURBYT = 0;	/* initialize the first buffer */
	if(specify_position)	/* deal with -i option if present */
	{
		position_buffer(specify_position,specify_byt);
		specify_position = 0;	/* it's a one-shot */
		specify_byt = 0;
	}
	else	/* position not specified */
		TOPREC = BOTREC = CURREC = BASE->next;
	for(i = 2;i < NROW;i++)	/* establish BOTREC */
	{
		if(BOTREC == BASE)
			break;
		BOTREC = BOTREC->next;
	}
	if(i < NROW)
		BOTREC = 0;

	ref_display();	/* display the file */
	marge(TOPROW,BOTROW);	/* set up scrolling region */
	save_window();	/* load WINDOW[0] in case it's needed */
/* handle -m on command line */
	if(*multiplefiles)
	{
		while(1)
		{
			save_window();
			if(NWINDOWS >= MAX_WINDOWS)
				break;
			for(total = i = 0;i < NWINDOWS;i++)
				if((excess = WINDOW[i].botrow - WINDOW[i].toprow - 2) > 0)	/* there is space available here */
					total += excess;
			if(total < 4)
				break;
			recover = process = 0;
			parse_skip(1);	/* flag no prompting in parse_fnm */
			if(!parse_filename(comm,commlen,editfile,&recover,recfile,&process,multiplefiles,&bdummy,&bdummy))
				break;
			base = (rec_ptr)imalloc(sizeof(rec_node));
			base->length = 0;
			base->data = NULL;
			if(load_file(editfile,base,bookmark,&bookpos,&bookbyt,&diredit,binary = WINDOW[CURWINDOW].binary,&filebuf) != -2)
			{
				insert_window(editfile,base,binary,(int)diredit);	/* note, this routine may call set_window in order to scroll reduced windows */
				if((i = strlen(bookmark)))
				{
					WINDOW[CURWINDOW].bookmark = (Char *)imalloc(++i);
					strcpy(WINDOW[CURWINDOW].bookmark,bookmark);
					if(--bookpos > 0)
						down_arrow(bookpos);
					if(bookbyt)
						right_arrow(bookbyt);
				}
				WINDOW[CURWINDOW].filebuf = filebuf;
				WINDOW[CURWINDOW].binary = binary;
				WINDOW[CURWINDOW].news = 0;
			}
		}
	}
	while(1)	/* main loop */
	{
		if(!deliberate)	/* deliberate != 0 means they shifted left or right deliberately */
		{
			if((i = CURCOL - NCOL - FIRSTCOL) > -2)
			{
				FIRSTCOL += i + 10;
				ref_window(CURWINDOW);
			}
			else if(CURCOL < FIRSTCOL)
			{
				FIRSTCOL = max(1,CURCOL - 10);
				ref_window(CURWINDOW);
			}
		}
		else
			deliberate = 0;
		if(REPORTSTATUS && puttest())	/* handle cursor position report if necessary */
		{
			get_position(&record,&byte);
			sprintf(buf,"Rec %d,Byt %d,Col %d",record,CURBYT + 1,CURCOL - FIRSTCOL + 1);
			if(OVERSTRIKE)
				strcat(buf,",Ovr");
			if(BOXCUT)
				strcat(buf,",Box");
			newstatcol = NCOL - strlen(buf) + FIRSTCOL;
			if(laststatcol < newstatcol)
			{
				move(TOPROW - 1,laststatcol);
				ers_end();
			}
			move(TOPROW - 1,laststatcol = newstatcol);
			reverse();
			putz(buf);
			normal();
		}
		if(WINDOW[CURWINDOW].diredit)
			if(CURREC != BASE)
				if(CURCOL < 23)
				{
					CURCOL = 23;
					CURBYT = 22;
				}
		move(CURROW,CURCOL);	/* move to where we are */
		repeat = get_next_key(&keybuf);
		unmark_paren();	/* remove parenthesis marking, if any */
/* verify the keystroke is allowed for a diredit window */
		if((WINDOW[CURWINDOW].diredit && diredit_keymode[keybuf]))
		{
			bell();
			continue;
		}
/* verify the keystroke is allowed for a group window */
		if(WINDOW[CURWINDOW].news == 1 && group_keymode[keybuf])
		{
			bell();
			continue;
		}
/* verify the keystroke is allowed for an article window */
		if(WINDOW[CURWINDOW].news == 2 && article_keymode[keybuf])
		{
			bell();
			continue;
		}
		switch(keybuf)
		{
/*******************************************************************************/
/*                              control characters                             */
/*******************************************************************************/

/*case KEY_CTRLC:*/ 			/* control-c, just redisplay */
/*	emulate_key((Schar)KEY_REFRESH,1);
	break;*/

case KEY_BACKSPACE: 			/* backspace (move to beginning of line, or previous if at beginning) */
	backspace(repeat);
	break;

case KEY_DELPREVWORD: 			/* delete previous word */
	if(!find_word(-repeat))
	{
		abort_key();
		break;
	}
	if(OVERSTRIKE)
	{
		offset = find_word(-repeat);
		killer(offset,&WORDBUF,0);
	}
	else
	{
		if(repeat > 1)
		{
			offset = find_word(-(repeat - 1));
			killer(offset,0,0);
			WORDBUF.direction = 1;
		}
		offset = find_word(-1);
		killer(offset,&WORDBUF,0);
	}
	WORDBUF.direction = 1;
	WINDOW[CURWINDOW].modified = 1;
	break;

case KEY_DEFINEKEY: 			/* define key */
	define_key();
	break;

case KEY_CARRIAGERETURN: 			/* carriage return */
selective:
#ifndef NO_NEWS
	if(WINDOW[CURWINDOW].news == 1)	/* a group selection */
	{
		if(CURREC == BASE)
			bell();
		else
		{
			for(i = 0;i < NWINDOWS;i++)	/* find out if an article is displayed */
				if(WINDOW[i].news == 3)
					break;
			if(i < NWINDOWS)	/* destroy the article window */
			{
				save_window();
				remove_window(i);
				for(j = 0;j < NMARK;j++)	/* adjust marks */
				{
					if(MARKWINDOW[j] == i)
					{
						for(k = j + 1;k < NMARK;k++)
						{
							MARKWINDOW[k - 1] = MARKWINDOW[k];
							MARKREC[k - 1] = MARKREC[k];
							MARKBYT[k - 1] = MARKBYT[k];
						}
						NMARK--;
						j--;
					}
				}
			}
			for(i = 0;i < NWINDOWS;i++)
				if(WINDOW[i].news == 2)
					break;
			newsrc_current(CURREC->data + groupname_offset());	/* figures out which entry in NETRC corresponds to current group, save internal pointer */
			news_group(CURREC->data + groupname_offset(),i,keybuf);
		}
	}
	else if(WINDOW[CURWINDOW].news == 2)	/* an article selection */
	{
		if(keybuf == 'P')	/* there is no distinction between p and P */
			keybuf = tolower(keybuf);
		if(keybuf == 'p' && !posting_allowed())
		{
			bell();
			slip_message("Posting is not allowed by this news server.");
			wait_message();
			break;
		}
/* if retrieving an article, don't allow them off the end of the list */
		if(CURREC == BASE && keybuf != 'p')
			bell();
		else
		{
/* retrieve the article */
			if(CURREC != BASE)
			{
				my_sscanf(CURREC->data,"%d",&artnum);	/* get article number */
				sprintf(artcom,"STAT %d",artnum);
				news_command(artcom);
				news_response(buf,sizeof(buf));
				if(buf[0] != '2')
				{
					slip_message("An error occurred while STATing that article:");
					slip_message(buf);
					wait_message();
					break;
				}
			}
/* generate a new buffer */
			base = (rec_ptr)imalloc(sizeof(rec_node));
			base->length = 0;
			base->data = NULL;
			base->next = base->prev = base;
/* retrieve the article as appropriate */
			if(keybuf == 'R' || keybuf == 'V')
				news_command("ARTICLE");
			else if(keybuf == 'p')
			{
				if(CURREC == BASE)
				{
					if(!inquire("Subject",buf,sizeof(buf),1))
					{
						toss_data(base);
						break;
					}
					load_post(base,buf);
				}
				else
				{
					p = CURREC->data + 42;
					while(*p == '=')
						p++;
					load_post(base,p);
					news_command(artcom);
					news_response(buf,sizeof(buf));
					if(buf[0] != '2')
					{
						toss_data(base);
						slip_message("An error occurred while STATing that article:");
						slip_message(buf);
						wait_message();
						break;
					}
					news_command("BODY");
				}
			}
			else
				news_command("BODY");
			if(CURREC != BASE)
			{
				news_response(buf,sizeof(buf));
				if(buf[0] != '2')
				{
					slip_message("An error occurred while retrieving that article:");
					slip_message(buf);
					wait_message();
					toss_data(base);
					break;
				}
			}
			if(keybuf != 'p')	/* posting doesn't change article-read state */
			{
				read = CURREC->recflags & 2;	/* save current state of this entry */
				CURREC->recflags ^= 2;	/* toggle its state */
				paint(CURROW,CURROW,FIRSTCOL);
				if(read)	/* was already marked read, mark it unread */
					newsrc_mark_unread(artnum);
				else	/* was not marked as read, mark it read */
					newsrc_mark_read(artnum);
			}
/* read the article body */
			if(CURREC != BASE)
				while(1)
				{
					news_response(buf,sizeof(buf));
					if((i = strlen(buf)) == 1 && buf[0] == '.')
						break;
					new = (rec_ptr)imalloc(sizeof(rec_node));
					insq(new,base->prev);
/* if posting buffer, precede each line with "> " */
					if(keybuf == 'p')
					{
						new->data = (Char *)imalloc(i + 3);
						strcpy(new->data,"> ");
						strcat(new->data,buf);
						new->length = i + 2;
					}
					else
					{
						new->data = (Char *)imalloc(i + 1);
						strcpy(new->data,buf);
						new->length = i;
					}
					new->recflags = 1;	/* this is a freeable buffer */
				}
			if(keybuf == 'p')
			{
				new = (rec_ptr)imalloc(sizeof(rec_node));
				new->data = (Char *)imalloc(1);
				*new->data = '\0';
				new->length = 0;
				new->recflags = 1;
				insq(new,base->prev);
				if(CURREC == BASE)
					strcpy(artcom,"New Posting");
				else
					sprintf(artcom,"Followup to article %d",artnum);
			}
			else
			{
				down_arrow(1);
				sprintf(artcom,"Article %d",artnum);
			}
/* if no news=3 buffer exists, create one */
			for(n = 0;n < NWINDOWS;n++)
				if(WINDOW[n].news == 3)
					break;
			if(n == NWINDOWS)
			{
				save_window();
				insert_window_percent(80);
				insert_window(artcom,base,0,0);
				WINDOW[CURWINDOW].filebuf = NULL;
				WINDOW[CURWINDOW].binary = 0;
				WINDOW[CURWINDOW].diredit = 0;
				WINDOW[CURWINDOW].news = 3;
				if(keybuf == 'p')
					WINDOW[CURWINDOW].news = 4;
				else if(keybuf == 'r' || keybuf == 'R')
					set_window(1);	/* move back to the article list window */
			}
			else	/* the article window already appears */
			{
				if(WINDOW[n].filename)
					ifree(WINDOW[n].filename);
				WINDOW[n].filename = (Char *)imalloc(strlen(artcom) + 1);
				strcpy(WINDOW[n].filename,artcom);
				toss_data(WINDOW[n].base);
				WINDOW[n].base = base;
				WINDOW[n].currec = WINDOW[n].toprec = base->next;
				new_botrec(n);
				WINDOW[n].curbyt = 0;
				WINDOW[n].currow = WINDOW[n].toprow;
				WINDOW[n].curcol = WINDOW[n].firstcol = WINDOW[n].wantcol = 1;
				WINDOW[n].modified = WINDOW[n].diredit = WINDOW[n].binary = 0;
				WINDOW[n].news = 3;
				ref_window(n);
				if(keybuf == 'v' || keybuf == 'V')
					set_window(n);
				if(keybuf == 'p')
					WINDOW[n].news = 4;
				save_window();
			}
		}
	}
	else
#endif
    if(WINDOW[CURWINDOW].diredit)
	{
		if(CURREC == BASE)
		{
			abort_key();
			bell();
			break;
		}
		else
		{
			unselect();
			curdirpos = dir_depth(WINDOW[CURWINDOW].filename);
/* generate the complete selected file name */
			strcpy(buf,WINDOW[CURWINDOW].filename);
			if((p = strstr(buf," - GREP window")))
				*p = '\0';
			else if((p = strstr(buf," - PERG window")))
				*p = '\0';
			strcat(buf,CURREC->data + filename_offset());
/* handle symbolic link format for ftp diredits */
			if((p = strstr(buf," -> ")))
				*p = '\0';
/* store the position in the current listing, unless they're going up */
			if(strcmp(CURREC->data + filename_offset(),"[-]") && strcmp(CURREC->data + filename_offset(),".."))
			{
				get_position(&record,&byte);
				dirpos[curdirpos] = record;
				upmove = 0;
			}
			else
				upmove = 1;
/* parse VMS-y dirnames */
			if(!ftp_unixy(ftp_system(buf)))
			{
				while((p = strstr(buf,"][")))
					for(q = p + 2;(*p++ = *q++););
				if((p = strstr(buf,"-]")))
				{
					while(*--p != '.')
						if(*p == '[')
							break;
					if(*p == '[')
						strcpy(++p,"000000]");
					else
						strcpy(p,"]");
				}
/* this code used to turn [000000.X] to [X]. some servers are picky.
				if((p = strstr(buf,"[000000.")))
				{
					p++;
					for(q = p + 7;(*p++ = *q++););
				}*/
			}
/* if they selected a directory */
			if(CURREC->data[0] == 'd' || CURREC->data[0] == 'l')
			{
				if(ftp_unixy(ftp_system(buf)))
					strcat(buf,"/");
#ifndef NO_FTP
				if((i = host_in_name(buf)))	/* this is true if /xxx.yyy.zzz: is in the file name */
				{
					p = host_filename(i,buf,ftpfile);	/* this does all the parsing */
					if((base = dir_find(CURWINDOW,ftpfile)))	/* check whether we've visited here yet */
					{
						strcpy(fname,ftpfile);
						goto dispwin;
					}
				}
#endif
/* create new buffer */
				base = (rec_ptr)imalloc(sizeof(rec_node));
				base->length = 0;
				base->data = NULL;
				base->prev = base->next = base;
/* check for return to a GREP window */
				curdirpos = dir_depth(fname);
				if(upmove && grepdata[curdirpos - 1])
				{
					copy_records(grepdata[curdirpos - 1],base,0,0);
					strcpy(fname,greptitle[curdirpos - 1]);
					goto dispwin;
				}
/* read in the directory */
				if(read_in_diredit(fname,buf,base) > 0)
				{
dispwin:
					if(host_in_name(ftpfile))
						dir_store(CURWINDOW,ftpfile,base);
					else
						toss_data(BASE);
					BASE = base;
					CURROW = TOPROW;
					CURCOL = 1;
					FIRSTCOL = 1;
					WANTCOL = 1;
					DIRECTION = 1;
					SELREC = 0;
					TOPREC = BOTREC = CURREC = BASE->next;
/* do magical parsing ad nauseam */
					if(upmove)
					{
						curdirpos = dir_depth(fname);
						if(dirpos[curdirpos])
							position_buffer(dirpos[curdirpos],0);
						else	/* we haven't been here yet, find olddir in the list */
						{
							strcpy(buf,WINDOW[CURWINDOW].filename);
							if(ftp_unixy(ftp_system(buf)))
							{
								buf[strlen(buf) - 1] = '\0';	/* strip the final '/' */
								if(strlen(buf))
								{
									p = strrchr(buf,'/') + 1;	/* point to dirname */
									for(i = 1,rec = BASE->next;rec != BASE;rec = rec->next,i++)
										if(!strcmp(rec->data + filename_offset(),p))
											break;
									position_buffer(i,0);
								}
								else
									position_buffer(1,0);
							}
							else
							{
								for(p = buf + strlen(buf);p != buf;)
								{
									if(*--p == ':')
										break;
									if(*p == '[')
										break;
									if(*p == '.')
										break;
								}
								if(*p == ':')
									position_buffer((dirpos[curdirpos] = 1),0);
								else
								{
									if(*p == '.')
										*--p = '[';
									else
									{
										*p-- = '.';
										*p = '[';
									}
									for(i = 1,rec = BASE->next;rec != BASE;rec = rec->next,i++)
										if(!inscmp(rec->data + filename_offset(),p))
											break;
									position_buffer(i,0);
								}
							}
						}
					}
					for(i = TOPROW;i < BOTROW;i++)	/* establish BOTREC */
					{
						if(BOTREC == BASE)
							break;
						BOTREC = BOTREC->next;
					}
					if(i < BOTROW)
						BOTREC = 0;
					ifree(WINDOW[CURWINDOW].filename);
					WINDOW[CURWINDOW].filename = (Char *)imalloc(strlen(fname) + 1);	/* this line cannot be merged with the following one... */
					strcpy(WINDOW[CURWINDOW].filename,fname);	/* ...without generating a pointer mismatch error under VMS cc */
					WINDOW[CURWINDOW].base = BASE;
					save_window();
					ref_window(CURWINDOW);
				}
				else	/* read_in_diredit failed, if it's a link, try doing it more directly */
				{
					if(CURREC->data[0] == 'l')
						goto open_window;
					ifree(base);
					bell();
				}
				break;
			}
open_window:
			wincom_special();
			emulate_key((Schar)KEY_WINDOW,1);
			emulate_key((Schar)'o',1);
			emulate_key((Schar)KEY_CARRIAGERETURN,1);
			strcpy(buf,WINDOW[CURWINDOW].filename);
			if((p = strstr(buf," - GREP window")))
				*p = '\0';
			else if((p = strstr(buf," - PERG window")))
				*p = '\0';
			for(i = strlen((p = buf));i--;emulate_key((Schar)*p++,1));
			strcpy(buf,CURREC->data + filename_offset());
/* set grepdata to NULL, tossing anything that was there (a quasi-memory-leak) */
			curdirpos = dir_depth(buf);
			if(grepdata[curdirpos])
			{
				toss_data(grepdata[curdirpos]);
				grepdata[curdirpos] = NULL;
			}
			if(greptitle[curdirpos])
			{
				ifree(greptitle[curdirpos]);
				greptitle[curdirpos] = NULL;
			}
/* handle symbolic link format for ftp diredits */
			if((p = strstr(buf," -> ")))
				*p = '\0';
			for(i = 0;i < strlen(buf);i++)
				emulate_key((Schar)buf[i],1);
			emulate_key((Schar)KEY_CARRIAGERETURN,1);
			if(host_in_name(WINDOW[CURWINDOW].filename))
				ftp_fileonly();
		}
	}
	else
	{
		carriage_return(repeat);
		WINDOW[CURWINDOW].modified = 1;
	}
	break;

case KEY_DELTOBEGLINE: 			/* delete to beginning of line */
	emulate_key((Schar)KEY_DELTOBEGLINE2,repeat);
	WINDOW[CURWINDOW].modified = 1;
	break;

case KEY_REFRESH: 			/* refresh screen */
	ref_display();
	break;

case KEY_DELTOBEGLINE2: 			/* delete to beginning of line */
	if(!find_line(-repeat))
	{
		abort_key();
		break;
	}
	if(OVERSTRIKE)
	{
		offset = find_line(-repeat);
		killer(offset,&LINEBUF,0);
	}
	else
	{
		if(repeat > 1)
		{
			offset = find_line(-(repeat - 1));
			killer(offset,0,0);
			LINEBUF.direction = 1;
		}
		offset = find_line(-1);
		killer(offset,&LINEBUF,0);
	}
	LINEBUF.direction = 1;
	WINDOW[CURWINDOW].modified = 1;
	break;

/*case KEY_CTRLY:*/ 			/* control-y */
/*	emulate_key((Schar)KEY_REFRESH,repeat);
	break;*/

case KEY_COMMAND: 			/* command */
	emulate_key((Schar)KEY_COMMAND2,repeat);
	break;

case KEY_DELETE: 			/* delete */
	if(!find_char(-repeat))
	{
		abort_key();
		break;
	}
	if(OVERSTRIKE)
		killer(-repeat,&CHARBUF,0);
	else
	{
		if(repeat > 1)
			killer(1L - repeat,0,0);
		killer(-1L,&CHARBUF,0);
	}
	CHARBUF.direction = 1;
	WINDOW[CURWINDOW].modified = 1;
	break;

/*******************************************************************************/
/*                                 keypad keys                                 */
/*******************************************************************************/

case KEY_UPARROW: 			/* up arrow */
	copy_only(0);
	up_arrow(repeat);
	break;

case KEY_DOWNARROW: 			/* down arrow */
	copy_only(0);
	down_arrow(repeat);
	break;

case KEY_RIGHTARROW: 			/* right arrow */
	copy_only(0);
	right_arrow(repeat);
	break;

case KEY_LEFTARROW: 			/* left arrow */
	copy_only(0);
	left_arrow(repeat);
	break;

case KEY_MOVEBYLINE: 			/* move by line */
	if(WINDOW[CURWINDOW].diredit && DIRECTION == -1)
		emulate_key((Schar)KEY_UPARROW,repeat);
	else
	{
		copy_only(0);
		move_line(repeat);
	}
	break;

case KEY_MOVEBYWORD: 			/* move by word */
	copy_only(0);
	move_word(repeat);
	break;

case KEY_MOVEBYEOL: 			/* move to end of line */
	copy_only(0);
	move_eol(repeat);
	break;

case KEY_MOVEBYCHAR: 			/* move by character */
	copy_only(0);
	if(DIRECTION == 1)
		right_arrow(repeat);
	else
		left_arrow(repeat);
	break;

case KEY_FORWARD: 			/* set forward */
	copy_only(0);
	DIRECTION = 1;
	break;

case KEY_BACKWARD: 			/* set backward */
	copy_only(0);
	DIRECTION = -1;
	break;

case KEY_KILL: 			/* kill */
	match_init(SEARCH_FLAGS);
	if(SELREC != 0)
	{
		offset = get_offset(SELREC,SELBYT,get_seldir());
		if(WINDOW[CURWINDOW].diredit || WINDOW[CURWINDOW].news == 1 || WINDOW[CURWINDOW].news == 2)
		{
			if(offset < 0 && SELREC != CURREC)
			{
				rec1 = SELREC;
				rec2 = CURREC->prev;
			}
			else if(offset > 0 && SELREC != CURREC)
			{
				rec1 = CURREC;
				rec2 = SELREC->prev;
			}
			else
				break;
			while(1)
			{
#ifndef NO_NEWS
				if(WINDOW[CURWINDOW].news == 2)	/* handle newsrc dbs if it's an article window */
				{
					my_sscanf(rec1->data,"%d",&artnum);
					if(rec1->recflags & 2)	/* the article is marked read */
						newsrc_mark_unread(artnum);
					else
						newsrc_mark_read(artnum);
				}
#endif
				rec1->recflags ^= 2;
				if(rec1 == rec2)
					break;
				rec1 = rec1->next;
			}
			unselect();
			ref_window(CURWINDOW);
		}
		else
		{
			unselect();
			if(BOXCUT)
			{
				if(WINDOW[CURWINDOW].binary)
				{
					slip_message("You can't use box mode in binary buffers.");
					wait_message();
					abort_key();
					break;
				}
				else
					killer_box();
			}
			killer(offset,&KILLBUF,0);
			if(offset < 0)
				KILLBUF.direction = 1;
			else
				KILLBUF.direction = -1;
		}
	}
	else if((offset = match_search(CURREC,CURBYT,&SEARCHBUF)) != 0)
	{
		killer(offset,&KILLBUF,0);
		KILLBUF.direction = 1;
	}
	WINDOW[CURWINDOW].modified = 1;
	break;

case KEY_MOVEBYPAGE: 			/* move by page */
	copy_only(0);
	if(!PAGE_BREAK_LENGTH)
		break;
	load_buffer(&pagebuf,PAGE_BREAK,PAGE_BREAK_LENGTH);
	match_init("ebnnn");
	if(DIRECTION < 0 && !match_search(CURREC,CURBYT,&pagebuf))
		repeat++;
	offset = find_string(CURREC,CURBYT,DIRECTION,&pagebuf,"ebnnn",repeat);
	if(DIRECTION == 1)
	{
		if(offset != 0)
			right_arrow(offset);
		else
		{
			abort_key();
			emulate_key((Schar)KEY_ENDOFFILE,1);	/* treat as move-to-end-of-file */
		}
	}
	else
	{
		if(offset != 0)
			left_arrow(-offset);
		else
		{
			abort_key();
			emulate_key((Schar)KEY_TOPOFFILE,1);	/* treat as move-to-top-of-file */
		}
	}
	if(offset != 0 && CURROW <= BOTLIM)
	{
		down_arrow(BOTLIM - TOPROW);
		up_arrow(BOTLIM - TOPLIM);
	}
	break;

case KEY_MOVEBYSECTION: 			/* move by section */
	copy_only(0);
	if(SECTION_LINES <= 0)
	{
		if((i = repeat * ((BOTROW - TOPROW) + SECTION_LINES)) < 1)
			i = 1;
	}
	else
		i = repeat * SECTION_LINES;
	if(DIRECTION == 1)
		down_arrow(i);
	else
		up_arrow(i);
	break;

case KEY_APPEND: 			/* append */
	match_init(SEARCH_FLAGS);
	if(SELREC != 0)
	{
		offset = get_offset(SELREC,SELBYT,get_seldir());
		unselect();
		if(BOXCUT)
			killer_box();
		killer(offset,&KILLBUF,1);
	}
	else if((offset = match_search(CURREC,CURBYT,&SEARCHBUF)) != 0)
		killer(offset,&KILLBUF,1);
	KILLBUF.direction = 1;
	WINDOW[CURWINDOW].modified = 1;
	break;

case KEY_DELWORD: 			/* delete word */
	if(!find_word(repeat))
	{
		abort_key();
		copy_only(0);
		break;
	}
	if(OVERSTRIKE)
	{
		offset = find_word(repeat);
		killer(offset,&WORDBUF,0);
	}
	else
	{
		if(repeat > 1)
		{
			offset = find_word(repeat - 1);
			killer(offset,0,0);
		}
		offset = find_word(1);
		killer(offset,&WORDBUF,0);
	}
	WORDBUF.direction = -1;
	WINDOW[CURWINDOW].modified = 1;
	break;

case KEY_DELCHAR: 			/* delete character */
	if(!find_char(repeat))
	{
		abort_key();
		copy_only(0);
		break;
	}
	if(OVERSTRIKE)
		killer(repeat,&CHARBUF,0);
	else
	{
		if(repeat > 1)
			killer(repeat - 1L,0,0);
		killer(1L,&CHARBUF,0);
	}
	CHARBUF.direction = -1;
	WINDOW[CURWINDOW].modified = 1;
	break;

case KEY_SELECT: 			/* select */
	copy_only(0);
	unselect();	/* remove selection if it is already active */
	SELREC = CURREC;
	SELBYT = CURBYT;
	SELDIR = 0;
	move(CURROW,CURCOL);
	doselect(CURREC,CURBYT);
	move(CURROW,CURCOL);
	break;

case KEY_ENTER: 			/* enter */
	copy_only(0);
	break;

case KEY_GOLD: 			/* gold */
	copy_only(0);
	reverse();
	putz("That was interesting.");
	normal();
	break;

case KEY_SETCOPY: 			/* set copy select */
	copy_only(1);
	break;

case KEY_FINDNEXT: 			/* find next */
	copy_only(0);
	if(WINDOW[CURWINDOW].diredit && DIRECTION == -1)
	{
		i = CURBYT;
		CURBYT = 0;
	}
	if((offset = find_string(CURREC,CURBYT,DIRECTION,&SEARCHBUF,SEARCH_FLAGS,repeat)) != 0)
	{
		if(DIRECTION == 1)
			right_arrow(offset);
		else
			left_arrow(-offset);
	}
	else
	{
		if(WINDOW[CURWINDOW].diredit && DIRECTION == -1)
			CURBYT = i;
		abort_key();
		bell();
	}
	break;

case KEY_DELLINE: 			/* delete line */
	if(WINDOW[CURWINDOW].diredit || WINDOW[CURWINDOW].news == 1 || WINDOW[CURWINDOW].news == 2)
	{
#ifndef NO_NEWS
		if(WINDOW[CURWINDOW].news == 2)
		{
			my_sscanf(CURREC->data,"%d",&artnum);
			if(CURREC->recflags & 2)	/* the article is marked read */
				newsrc_mark_unread(artnum);
			else
				newsrc_mark_read(artnum);
		}
#endif
		CURREC->recflags ^= 2;
		paint(CURROW,CURROW,FIRSTCOL);
	}
	else
	{
		if(!find_line(repeat))
		{
			abort_key();
			copy_only(0);
			break;
		}
		if(OVERSTRIKE)
		{
			offset = find_line(repeat);
			killer(offset,&LINEBUF,0);
		}
		else
		{
			if(repeat > 1)
			{
				offset = find_line(repeat - 1);
				killer(offset,0,0);
			}
			offset = find_line(1);
			killer(offset,&LINEBUF,0);
		}
		LINEBUF.direction = -1;
		WINDOW[CURWINDOW].modified = 1;
	}
	break;

case KEY_OPENLINE: 			/* open line */
	copy_only(0);
	openline(repeat);
	WINDOW[CURWINDOW].modified = 1;
	break;

case KEY_CHANGECASE: 			/* change case */
	switch(CASECHANGE)
	{
		case CASE_UPPER:
			casetable = _my_toupper_;
			break;
		case CASE_LOWER:
			casetable = _my_tolower_;
			break;
		case CASE_OPPOSITE:
			casetable = _my_tooppos_;
			break;
		case CASE_CAPITALIZE:
			casetable = NULL;
			break;
	}
	copy_only(0);
	match_init(SEARCH_FLAGS);
	if(SELREC != 0)
	{
		offset = get_offset(SELREC,SELBYT,get_seldir());
		unselect();
		change_case(offset,0,casetable);
	}
	else if((offset = match_search(CURREC,CURBYT,&SEARCHBUF)) != 0)
		change_case(offset,(SEARCH_FLAGS[((int)SMODE_EXACT) >> 1] == 'e')? 0 : 1,casetable);
	else
		if(!change_case(repeat * DIRECTION,1,casetable))
			abort_key();
	WINDOW[CURWINDOW].modified = 1;
	break;

case KEY_DELTOEOL: 			/* delete to end of line */
	if(!find_line(repeat))
	{
		abort_key();
		copy_only(0);
		break;
	}
	if(OVERSTRIKE)
	{
		offset = find_line(repeat) - 1;
		killer(offset,&LINEBUF,0);
	}
	else
	{
		if(repeat > 1)
		{
			offset = find_line(repeat - 1) - 1;
			killer(offset,0,0);
		}
		offset = find_line(1) - 1;
		killer(offset,&LINEBUF,0);
	}
	LINEBUF.direction = -1;
	WINDOW[CURWINDOW].modified = 1;
	break;

case KEY_SPECIALINSERT: 			/* special insert */
	copy_only(0);
	keybuf = (repeat < 128)? repeat : (repeat - 256);
	repeat = 1;
	goto insert_char;

case KEY_ENDOFFILE: 			/* bottom of file */
	copy_only(0);
	forgive();
	down_arrow(0x7fffffff);
	begrudge();
	WANTCOL = 0;
	break;

case KEY_TOPOFFILE: 			/* top of file */
	copy_only(0);
	forgive();
	up_arrow(0x7fffffff);
	left_arrow(0x7fffffff);
	begrudge();
	WANTCOL = 0;
	break;

case KEY_UNKILL: 			/* unkill */
	copy_only(0);
	for(i = 0;i < repeat;i++)
	{
		if(BOXCUT)
			insert_box();
		insert(&KILLBUF);
	}
	WINDOW[CURWINDOW].modified = 1;
	break;

case KEY_COMMAND2: 			/* command */
	copy_only(0);
	if((i = command(grepdata,greptitle)))
	{
		if(i == 1)	/* normal EXIT, QUIT etc. */
		{
			for(i = 0;i < sizeof(grepdata) / sizeof(rec_ptr);i++)
			{
				if(grepdata[i])
				{
					toss_data(grepdata[i]);
					grepdata[i] = NULL;
				}
				if(greptitle[i])
				{
					ifree(greptitle[i]);
					greptitle[i] = NULL;
				}
			}
			return;
		}
#ifndef NO_NEWS
/* find the posting window */
		for(close = 0;close < NWINDOWS;close++)
			if(WINDOW[close].news == 4)
				break;
/* insert the Lines header */
rescan:
		if(WINDOW[close].base->next == WINDOW[close].base)
		{
bad_posting:
			slip_message("That posting doesn't seem to contain anything, ignoring it.");
			wait_message();
			goto clean_posting;
		}
		if(i == -2)
		{
			slip_message("Posting aborted.");
			wait_message();
			goto clean_posting;
		}
		for(j = k = 0,rec = WINDOW[close].base->next;rec != WINDOW[close].base;rec = rec->next,j++)
			if(!k && !rec->length)	/* found a blank line */
				if(!j)	/* it is at top of file, remove it and try again */
				{
					ifree(rec->data);
					remq(rec);
					ifree(rec);
					goto rescan;
				}
				else
				{
					save_rec = rec;	/* save blank line position, Lines: will go preceeding it */
					k = j;
				}
		if(!(bodycount = j - k - 1))	/* total lines in body. if zero, find the Reply-To: line and insert count following that */
		{
			for(j = k = 0,rec = WINDOW[close].base->next;rec != WINDOW[close].base;rec = rec->next,j++)
				if(!k && !strncmp(rec->data,"Reply-To: ",strlen("Reply-To: ")))
				{
					save_rec = rec;
					k = j;
					break;
				}
			if(!(bodycount = j - k - 1))
				goto bad_posting;
			new = (rec_ptr)imalloc(sizeof(rec_node));
			*(new->data = (Char *)imalloc(1)) = '\0';
			new->length = 0;
			new->recflags = 1;
			insq(new,save_rec);	/* insert a blank record following Reply-To: */
			save_rec = save_rec->next;
		}
/* insert the Lines: header */
		new = (rec_ptr)imalloc(sizeof(rec_node));
		sprintf(buf,"Lines: %d",bodycount);
		new->data = (Char *)imalloc((new->length = strlen(buf)) + 1);
		strcpy(new->data,buf);
		new->recflags = 1;
		insq(new,save_rec->prev);
/* post the article */
		news_command("POST");
		news_response(buf,sizeof(buf));
		if(strncmp(buf,"340 ",4))
		{
			bell();
			slip_message(buf + 4);
			wait_message();
			goto clean_posting;
		}
		for(rec = WINDOW[close].base->next;rec != WINDOW[close].base;rec = rec->next)
			news_command(rec->data);
		news_command(".");
		news_response(buf,sizeof(buf));
		slip_message(buf + 4);
		wait_message();
clean_posting:
		remove_window(close);
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
#endif	/* NO_NEWS */
	}
	break;

case KEY_FILL: 			/* fill */
	copy_only(0);
	if(!SELREC)
	{
		abort_key();
		break;
	}
	word_fill();
	WINDOW[CURWINDOW].modified = 1;
	break;

case KEY_REPLACE: 			/* replace */
	copy_only(0);
	match_init(SEARCH_FLAGS);
	if((offset = match_search(CURREC,CURBYT,&SEARCHBUF)) != 0)
	{
		killer(offset,&pagebuf,0);
		insert(&KILLBUF);
		WINDOW[CURWINDOW].modified = 1;
	}
	else
		abort_key();
	break;

case KEY_UNDELWORD: 			/* undelete word */
	copy_only(0);
	for(i = 0;i < repeat;i++)
		insert(&WORDBUF);
	WINDOW[CURWINDOW].modified = 1;
	break;

case KEY_UNDELCHAR: 			/* undelete character */
	copy_only(0);
	for(i = 0;i < repeat;i++)
		insert(&CHARBUF);
	WINDOW[CURWINDOW].modified = 1;
	break;

case KEY_UNSELECT: 			/* unselect */
	copy_only(0);
	unselect();
	break;

case KEY_SUBSTITUTE: 			/* substitute */
	copy_only(0);
	match_init(SEARCH_FLAGS);
	if((searchlength = match_search(CURREC,CURBYT,&SEARCHBUF)) != 0)
	{
		KILLBUF.direction = DIRECTION;
		if(repeat > 1)
		{
			was_displaying = puttest();
			putoff();
		}
		else
			toggle_paren();
		for(i = 0;i < repeat;i++)
		{
			killer(searchlength,&pagebuf,0);
			WINDOW[CURWINDOW].modified = 1;
			if(KILLBUF.nrecs > 0)
				insert(&KILLBUF);
			if(DIRECTION == 1)
				left_arrow(1);	/* otherwise, we might already be sitting on the next string to replace */
			if((offset = find_string(CURREC,CURBYT,DIRECTION,&SEARCHBUF,SEARCH_FLAGS,1)) != 0)
			{
				if(DIRECTION == 1)
					right_arrow(offset);
				else
					left_arrow(-offset);
			}
			else
			{
				bell();
				emulate_abort();
				abort_key();
				break;
			}
		}
		KILLBUF.direction = 1;
		if(repeat <= 1)		/* CWS 11-93 non-display toggle bug */
			toggle_paren();
		else if(was_displaying)
		{
			puton();
			ref_display();
			move(CURROW,CURCOL);
		}
	}
	else
		abort_key();
	break;

case KEY_GOLDGOLD: 			/* gold-gold */
	copy_only(0);
	break;

case KEY_WINDOW: 			/* window */
	copy_only(0);
	wincom();
	break;

case KEY_FIND: 			/* find */
	copy_only(0);
	i = inquire("Search for",buf,sizeof(buf),0);
	paint(BOTROW,BOTROW,FIRSTCOL);
	if(!i)
		break;
	term = get_terminator();
	if(term == KEY_FORWARD)
		DIRECTION = 1;
	else if(term == KEY_BACKWARD)
		DIRECTION = -1;
	load_buffer(&SEARCHBUF,buf,i);
	if(SEARCH_FLAGS[((int)SMODE_CHAR) >> 1] == 'w' && SEARCHBUF.nrecs > 1)
	{
		buffer_empty(&SEARCHBUF);
		slip_message("Embedded <cr>s don't work when SET SEARCH WORD is in effect.");
		wait_message();
		break;
	}
	if((offset = find_string(CURREC,CURBYT,DIRECTION,&SEARCHBUF,SEARCH_FLAGS,repeat)) != 0)
	{
		if(DIRECTION == 1)
			right_arrow(offset);
		else
			left_arrow(-offset);
	}
	else
	{
		emulate_abort();
		abort_key();
		bell();
	}
	break;

case KEY_UNDELLINE: 			/* undelete line */
	copy_only(0);
	for(i = 0;i < repeat;i++)
		insert(&LINEBUF);
	WINDOW[CURWINDOW].modified = 1;
	break;

case KEY_RETURN: 			/* gold up (return to spot) */
	copy_only(0);
	for(i = NMARK - 1;i >= 0;i--)
		if(MARKWINDOW[i] != CURWINDOW || MARKREC[i] != CURREC || MARKBYT[i] != CURBYT)
		{
			save_window();
			set_window(MARKWINDOW[i]);
			offset = get_offset(MARKREC[i],MARKBYT[i],get_markdir(MARKREC[i],MARKBYT[i]));
			if(offset < 0)
				left_arrow(-offset);
			else if(j > 0)
				right_arrow(offset);
			break;
		}
	break;

case KEY_MARK: 			/* gold down (mark spot) */
	copy_only(0);
	for(i = 0;i < NMARK;i++)
		if(MARKWINDOW[i] == CURWINDOW && MARKREC[i] == CURREC && MARKBYT[i] == CURBYT)
		{
			for(j = i + 1;j < NMARK;j++)
			{
				MARKWINDOW[j - 1] = MARKWINDOW[j];
				MARKREC[j - 1] = MARKREC[j];
				MARKBYT[j - 1] = MARKBYT[j];
			}
			NMARK--;
			goto label41;
		}
	if(NMARK == MAX_MARKS)	/* mark a new spot */
		break;
	MARKWINDOW[NMARK] = CURWINDOW;
	MARKREC[NMARK] = CURREC;
	MARKBYT[NMARK++] = CURBYT;
label41: 
	break;

case KEY_SCROLLRIGHT: 			/* gold right (scroll right) */
	copy_only(0);
	if(FIRSTCOL > 1)
	{
		FIRSTCOL = max(1,FIRSTCOL - NCOL / 4);
		ref_window(CURWINDOW);
	}
	deliberate = 1;
	break;

case KEY_SCROLLLEFT: 			/* gold left (scroll left) */
	copy_only(0);
	FIRSTCOL += NCOL / 4;
	ref_window(CURWINDOW);
	deliberate = 1;
	break;

default:
insert_char: /* user inserts a character */
	if(WINDOW[CURWINDOW].news == 1 || WINDOW[CURWINDOW].news == 2)
		goto selective;
	if(WINDOW[CURWINDOW].diredit)
	{
		if(CURREC->data[0] == 'd')
			goto selective;
		switch(keybuf)
		{
			case 'A':
			case 'a':
			case ' ':
				goto selective;
			case 'B':
			case 'b':
				force_binary();
				goto selective;
		}
	}
	copy_only(0);
	if(AUTOWRAP && (keybuf != ' ') && (CURCOL > WRAP_MARGIN))	/* handle autowrap */
		if(autowrp(keybuf))	/* returns 1 if autowrapping was possible (and was done) */
			continue;
	if(repeat == 1)
	{
		if((character = keybuf) == '}' && CFRIENDLY)
			cfrendly(&charbuf,&multibuf,&multirec);	/* deal with closure of C-block */
		else
			insert(&charbuf);	/* just insert character */
	}
	else	/* multiple-character insertion */
	{
		multirec.data = (Char *)imalloc(repeat + 1);
		memset(multirec.data,(Char)keybuf,repeat);
		multirec.length = repeat;
		insert(&multibuf);
		ifree(multirec.data);
	}
	WINDOW[CURWINDOW].modified = 1;
	break;
	
/*******************************************************************************/
		}	/* end of switch */
	}	/* end of forever loop */
}

