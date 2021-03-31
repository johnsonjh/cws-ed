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

#include "memory.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "buffer.h"
#include "buf_dec.h"
#include "keyvals.h"
#include "handy.h"

typedef struct pr_str *pr_ptr;
typedef struct pr_str {pr_ptr next,prev;Char *prompt;rec_ptr base,cur;Char hideit;} pr_node;

static pr_node ibase = {&ibase,&ibase,0,0};
static Schar terminator;
static Int hidden = 0;

/******************************************************************************\
|Routine: inquire_hidden
|Callby: read_in_diredit
|Purpose: Sets whether the characters typed in during an inquire() are hidden.
|         This is used when they are entering passwords.
|Arguments:
|    state is 0 for no hiding, 1 for hidden.
\******************************************************************************/
void inquire_hidden(state)
Int state;
{
	hidden = state;
}

/******************************************************************************\
|Routine: get_terminator
|Callby: edit
|Purpose: Returns the last terminating character used in response to a prompt.
|         This is used to determine which direction to search in.
|Arguments:
|    none
\******************************************************************************/
Schar get_terminator()
{
	return(terminator);
}

/******************************************************************************\
|Routine: inquire_insert
|Callby: inquire
|Purpose: Inserts data from a buffer and puts it in the user string buffer.
|         If it doesn't fit, it truncates at the end of the string buffer.
|Arguments:
|    buffer is the user string buffer.
|    bufl is the length of the string buffer.
|    bufc is the number of characters currently in the string buffer.
|    cursor is the position of the cursor in the string buffer.
|    buf is the buffer that contains the string to be inserted.
\******************************************************************************/
void inquire_insert(buffer,bufl,bufc,cursor,buf)
Char *buffer;
Int bufl,*bufc,*cursor;
buf_ptr buf;
{
	Int recl,hangover;
	Char *from,*to;

	if(buf->nrecs > 0)	/* if the buffer is nonempty... */
		if((recl = buf->first->length) > 0)	/* and the first record has length... */
		{
			if(*bufc + recl >= bufl)
				recl = bufl - *bufc - 1;
			if((hangover = *bufc - *cursor) > 0)	/* cursor is embedded */
			{
				*(to = buffer + *bufc + recl) = '\0';	/* new string terminator */
				for(from = buffer + *bufc;hangover--;)	/* make room for insertion */
					*--to = *--from;
			}
			memcpy(buffer + *cursor,buf->first->data,recl);	/* insert text */
			if(buf->direction == 1)
				*cursor += recl;
			*bufc += recl;
		}
}

/******************************************************************************\
|Routine: inquire_delete
|Callby: inquire
|Purpose: Deletes data from the user buffer.
|Arguments:
|    buffer is the user string buffer.
|    bufc is the number of characters currently in the string buffer.
|    cursor is the position of the cursor in the string buffer.
|    repeat is the number of characters to delete.
\******************************************************************************/
void inquire_delete(buffer,bufc,cursor,repeat,buf)
Char *buffer;
Int *bufc,*cursor,repeat;
buf_ptr buf;
{
	register Char *from,*to,c;
	rec_ptr r;

	buffer_empty(buf);
	buf->nrecs = 1;
	buf->direction = 1;
	r = buf->first = buf->last = (rec_ptr)imalloc(sizeof(rec_node));
	r->prev = r->next = (rec_ptr)&buf->first;
	r->data = (Char *)imalloc(repeat + 1);
	r->length = repeat;
	r->recflags = 1;	/* it is a freeable buffer */
	memcpy(r->data,buffer + *cursor - repeat,repeat);	/* this is dangerous if repeat > bufc */
	r->data[repeat] = '\0';
	if(*bufc == *cursor)
	{
		if((*cursor -= repeat) < 0)
			*cursor = 0;
		buffer[*cursor] = '\0';
		*bufc = *cursor;
	}
	else	/* cursor is embedded in buffer */
	{
		from = buffer + *cursor;
		to = from - repeat;
		while((c = *from++))
			*to++ = c;
		*to = '\0';
		*cursor -= repeat;
		*bufc -= repeat;
	}
}

/******************************************************************************\
|Routine: inquire
|Callby: cfg command edit help parse_com parse_fnm set_param wincom
|Purpose: Handles prompting of user for input. Returns the number of characters
|         the user typed.
|Arguments:
|    prompt is the prompt string.
|    buffer is the returned string of characters the user typed.
|    bufl is the length of the user buffer.
|    crterm is a flag that indicates that <cr> should terminate entry. If
|            crterm == 0, user must type some keypad key (like ENTER) to
|            terminate entry.
\******************************************************************************/
Int inquire(prompt,buffer,bufl,crterm)
Char *prompt,*buffer;
Int bufl,crterm;
{
	Int i,cursor,column,oldcolumn,count,start,savefirstcol,wordstart,ch,selectpos,repeat;
	Char *expbuffer,oldexpbuffer[512],save_hexmode,*s,*t;
	pr_ptr p;
	register rec_ptr r;
	Schar c;
	buf_node charbuffer,tmpbuf;
	rec_node charrec,tmprec;
	Schar charbuf;
	Int l1,l2,mn;

	selectpos = -1;	/* no selection yet */
	charbuffer.first = &charrec;	/* initialize repeated-char buffer */
	charbuffer.last = &charrec;
	charbuffer.nrecs = charbuffer.direction = 1;
	charrec.next = (rec_ptr)&charrec.next;
	charrec.prev = (rec_ptr)&charrec.next;
	charrec.length = 1;
	charrec.data = (Char *)&charbuf;
	charbuf = '\0';
	tmpbuf.first = (rec_ptr)&tmpbuf.first;
	tmpbuf.last = (rec_ptr)&tmpbuf.first;
	if(crterm)
	{
		save_hexmode = HEXMODE;
		HEXMODE = 0;
	}
/* identify the prompt queue for this prompt */
	for(p = ibase.next;p != &ibase;p = p->next)
		if(!strcmp(p->prompt,prompt))
			break;
/* create a new one if necessary */
	if(p == &ibase)
		if((p = (pr_ptr)imalloc(sizeof(pr_node))))
		{
			insq((rec_ptr)p,(rec_ptr)ibase.prev);
			i = strlen(prompt);
			if((p->prompt = (Char *)imalloc(i + 1)))
			{
				strcpy(p->prompt,prompt);
				p->base = 0;
			}
			else
			{
				ifree(p);
				p = 0;
			}
			p->hideit = hidden;
		}
/* load a blank record into the queue and make it the current position in the queue */
	if(p != 0)
		if((r = (rec_ptr)imalloc(sizeof(rec_node))))
		{
			r->length = 0;
			if((r->data = (Char *)imalloc(1)))
			{
				r->data[0] = '\0';
				if(p->base != 0)
					insq(r,p->base);
				else
					r->next = r->prev = r;
				p->base = p->cur = r;
				r->recflags = 1;	/* it is a freeable buffer */
			}
			else
			{
				ifree(r);
				r = p->base = p->cur = 0;
			}
		}
/* display the prompt */
	savefirstcol = FIRSTCOL;
	FIRSTCOL = 1;
	move(BOTROW,1);
	ers_end();
	reverse();
	putz(prompt);
	putz(">");
	normal();
	start = strlen(prompt) + 2;
/* prevent alpha evolution */
	ignore_alpha();
/* get the user input */
	cursor = 0;	/* cursor position */
	count = 0;	/* count of chars in buffer */
	oldexpbuffer[0] = '\0';	/* the contents of this buffer are compared to what express() returns */
	while(1)
	{
		for(s = buffer,t = s + cursor,column = 0;s != t;s++)
		{
				if(HEXMODE)
					column += 4;
				else if((ch = *s) == '\t')
					column = tabstop(column);
				else if(NONPRINTNCS(ch))
					column += 4;
				else
					column++;
		}
		column += start;
		move(BOTROW,oldcolumn = column);
		if((repeat = get_next_key(&c)) >= 0)
		{
			if(c == KEY_UPARROW || c == KEY_DOWNARROW)
			{
				if(p != 0 && p->cur != 0)
				{
					p->cur = ((c == KEY_UPARROW)? p->cur->prev : p->cur->next);
					cursor = count = min(bufl - 1,p->cur->length);
					memcpy(buffer,p->cur->data,count);
					buffer[count] = '\0';
					hidden = p->hideit;
				}
			}
			else if(c == KEY_RIGHTARROW || (c == KEY_MOVEBYCHAR && DIRECTION == 1))
			{
				if(cursor < count)
					cursor++;
			}
			else if(c == KEY_LEFTARROW || (c == KEY_MOVEBYCHAR && DIRECTION == -1))
			{	
				if(cursor)
					cursor--;
			}
			else if(c == KEY_MOVEBYLINE)
				cursor = 0;
			else if(c == KEY_MOVEBYWORD)
				cursor = find_word_string(repeat,buffer,count,cursor,DIRECTION);
			else if(c == KEY_MOVEBYEOL)
				cursor = count;
			else if(c == KEY_FORWARD && crterm)
				DIRECTION = 1;
			else if(c == KEY_BACKWARD && crterm)
				DIRECTION = -1;
			else if(c == KEY_SPECIALINSERT)
			{
				charbuf = repeat & 0xff;
				inquire_insert(buffer,bufl,&count,&cursor,&charbuffer);
			}
			else if(c == KEY_DELETE)
			{
				repeat = min(repeat,cursor);
				inquire_delete(buffer,&count,&cursor,repeat,&CHARBUF);
				CHARBUF.first->length = 1;
			}
			else if(c == KEY_DEFINEKEY && definition_inprog() >= 0)
				define_key();
			else if(c == KEY_UNKILL)
				inquire_insert(buffer,bufl,&count,&cursor,&KILLBUF);
			else if(c == KEY_UNDELWORD)
				inquire_insert(buffer,bufl,&count,&cursor,&WORDBUF);
			else if(c == KEY_UNDELCHAR)
				inquire_insert(buffer,bufl,&count,&cursor,&CHARBUF);
			else if(c == KEY_UNDELLINE)
				inquire_insert(buffer,bufl,&count,&cursor,&LINEBUF);
			else if(c == KEY_DELWORD)
			{
				wordstart = find_word_string(repeat,buffer,count,cursor,1);
				repeat = wordstart - cursor;
				cursor = wordstart;
				inquire_delete(buffer,&count,&cursor,repeat,&WORDBUF);
				WORDBUF.direction = -1;
			}
			else if(c == KEY_DELCHAR)
			{
				wordstart = min(count,cursor + repeat);
				repeat = wordstart - cursor;
				cursor = wordstart;
				inquire_delete(buffer,&count,&cursor,repeat,&CHARBUF);
				CHARBUF.direction = -1;
				CHARBUF.first->data[0] = CHARBUF.first->data[CHARBUF.first->length - 1];
				CHARBUF.first->length = 1;
			}
			else if(c == KEY_DELLINE || c == KEY_DELTOEOL)
			{
				if((repeat = count - cursor) > 0)
				{
					cursor += repeat;
					inquire_delete(buffer,&count,&cursor,repeat,&LINEBUF);
					LINEBUF.direction = -1;
				}
			}
			else if(c == KEY_SELECT)
			{
				unselect();
				selectpos = cursor;
			}
			else if(c == KEY_KILL)
			{
				if(selectpos >= 0)
				{
					if(selectpos < cursor)
						inquire_delete(buffer,&count,&cursor,cursor - selectpos,&KILLBUF);
					else if(selectpos > cursor)
					{
						repeat = selectpos - cursor;
						cursor = selectpos;
						inquire_delete(buffer,&count,&cursor,repeat,&KILLBUF);
						KILLBUF.direction = -1;
					}
					else
						buffer_empty(&KILLBUF);
					selectpos = -1;
				}
				else
				{
					tmprec.next = (rec_ptr)&tmprec.next;
					tmprec.prev = (rec_ptr)&tmprec.next;
					tmprec.length = strlen((char *)(tmprec.data = buffer));
					if((i = match_search(&tmprec,cursor,&SEARCHBUF)) < 0)
						inquire_delete(buffer,&count,&cursor,-i,&KILLBUF);
					else if(i > 0)
					{
						repeat = i;
						cursor += i;
						inquire_delete(buffer,&count,&cursor,repeat,&KILLBUF);
						KILLBUF.direction = -1;
					}
				}
			}
			else if(c == KEY_APPEND)
			{
				if(selectpos >= 0)
				{
					if(selectpos < cursor)
						repeat = cursor - selectpos;
					else if(selectpos > cursor)
					{
						repeat = selectpos - cursor;
						cursor = selectpos;
					}
				}
				else
				{
					tmprec.next = (rec_ptr)&tmprec.next;
					tmprec.prev = (rec_ptr)&tmprec.next;
					tmprec.length = strlen((char *)(tmprec.data = buffer));
					if(!(i = match_search(&tmprec,cursor,&SEARCHBUF)))
						continue;
					if(i < 0)
						repeat = -i;
					else if(i > 0)
					{
						repeat = i;
						cursor += i;
					}
				}
				inquire_delete(buffer,&count,&cursor,repeat,&tmpbuf);
				s = (Char *)imalloc((i = repeat + KILLBUF.first->length) + 1);
				if(KILLBUF.first->data)
					memcpy(s,KILLBUF.first->data,KILLBUF.first->length);
				memcpy(s + KILLBUF.first->length,tmpbuf.first->data,repeat);
				s[i] = '\0';
				buffer_empty(&tmpbuf);
				ifree(KILLBUF.first->data);
				KILLBUF.first->data = s;
				KILLBUF.first->length = i;
				selectpos = -1;
			}
			else if(c == KEY_DELPREVWORD)
			{
				wordstart = find_word_string(repeat,buffer,count,cursor,-1);
				repeat = cursor - wordstart;
				inquire_delete(buffer,&count,&cursor,repeat,&WORDBUF);
			}
			else if(c == KEY_DELTOBEGLINE)
			{
				if(cursor > 0)
				{
					cursor = count;
					repeat = count;
					inquire_delete(buffer,&count,&cursor,repeat,&LINEBUF);
				}
			}
			else if(c < 0 || (c == KEY_CARRIAGERETURN && crterm == 1))
			{
				terminator = c;
				buffer[count] = '\0';
				FIRSTCOL = savefirstcol;
				if(p != 0 && p->base != 0)	/* if we are at a valid prompt queue entry */
				{
					ifree(r->data);	/* dump the single null char */
					if((r->data = (Char *)imalloc(count + 1)))	/* record the user's input */
					{
						memcpy(r->data,buffer,count);
						r->length = count;
						p->base = r;	/* make this prompt queue point at the most recent one */
					}
					else
					{
						remq(r);
						ifree(r);
					}
				}
				see_alpha();
				cr();
				putout();
				if(crterm)
					HEXMODE = save_hexmode;
				return(count);
			}
			else
			{
				charbuf = (Char)c;
				for(i = repeat;i--;)
					inquire_insert(buffer,bufl,&count,&cursor,&charbuffer);
			}
			if(hidden)
			{
				move(BOTROW,start);
				ers_end();
				for(i = count;i--;)
					putz(".");
			}
			else
			{
				express(count,buffer,TAB_STRING[CUR_TAB_SETUP],TAB_LENGTH[CUR_TAB_SETUP],&expbuffer,512,0);
				l1 = strlen(oldexpbuffer);
				l2 = strlen(expbuffer);
				mn = (l1 < l2)? l1 : l2;
				for(i = 0;i < mn;i++)
					if(oldexpbuffer[i] != expbuffer[i])
						break;
				if((i += start) > oldcolumn)
				{
					right(i - oldcolumn);
					ers_end();
				}
				else if(i < oldcolumn)
					left(oldcolumn - i);
				i -= start;
				if(l2 < l1)
				{
					if(oldcolumn != l1)
						putz(expbuffer + i);
					ers_end();
				}
				else
					putz(expbuffer + i);
				strcpy(oldexpbuffer,expbuffer);
			}
			if(selectpos >= 0)
			{
				move(BOTROW,start + selectpos);
				reverse();
				expbuffer[selectpos + 1] = '\0';
				if(selectpos == count)
					expbuffer[selectpos] = ' ';
				putz(expbuffer + selectpos);
				normal();
			}
		}
	}
}

/******************************************************************************\
|Routine: load_inquire
|Callby: parse_fnm
|Purpose: Loads the file names passed on the command line as if they had been
|         typed in response to a prompt.
|Arguments:
|    prompt is the prompt that would have been used.
|    buffer is the file name to be stored.
\******************************************************************************/
void load_inquire(prompt,buffer)
Char *prompt,*buffer;
{
	pr_ptr p;
	rec_ptr r;

/* identify the prompt queue for this prompt */
	for(p = ibase.next;p != &ibase;p = p->next)
		if(!strcmp(p->prompt,prompt))
			break;
/* create a new one if necessary */
	if(p == &ibase)
		if((p = (pr_ptr)imalloc(sizeof(pr_node))))
		{
			insq((rec_ptr)p,(rec_ptr)ibase.prev);
			if((p->prompt = (Char *)imalloc(strlen(prompt) + 1)))
			{
				strcpy(p->prompt,prompt);
				p->base = 0;
			}
			else
			{
				ifree(p);
				p = 0;
			}
			p->hideit = 0;
		}
/* load a record into the queue and make it the current position in the queue */
	if(p != 0)
		if((r = (rec_ptr)imalloc(sizeof(rec_node))))
		{
			r->length = strlen(buffer);
			if((r->data = (Char *)imalloc(r->length + 1)))
			{
				strcpy(r->data,buffer);
				if(p->base != 0)
					insq(r,p->base);
				else
					r->next = r->prev = r;
				p->base = p->cur = r;
				r->recflags = 1;	/* it is a freeable buffer */
			}
			else
			{
				ifree(r);
				r = p->base = p->cur = 0;
			}
		}
}

