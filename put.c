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

#define BUFFERSIZE 64

#include "emul_def.h"
#include "handy.h"

static Char enabled = 1;
static Char buf[BUFFERSIZE+1];
static Char *cur = buf,*end = &buf[BUFFERSIZE];

/******************************************************************************\
|Routine: put
|Callby: init_term put
|Purpose: Basic output routine. Puts a string to the terminal. Should not be
|         used by software sharing this library, use putz instead.
|Arguments:
|    string is the string to put to the terminal.
\******************************************************************************/
void put(string)
Char *string;
{
	register Char *b;

	if(!enabled)
		return;
	if(BRAINDEAD)
		return;
	b = cur;
	while((*b++ = *string++))
		if(b == end)
		{
			ttyput(buf);
			b = buf;
		}
	cur = --b;
}

/******************************************************************************\
|Routine: putout
|Callby: cfg command help imalloc init_term inquire put wincom word_fill
|Purpose: Forces dumping of buffered output to the terminal.
|Arguments:
|    none
\******************************************************************************/
void putout()
{
	short *screen;
	register Char *s,*a;
	Int i;

	if(!BRAINDEAD)
	{
		if(cur != buf)
		{
			*cur = 0;
			if(enabled)
				ttyput(buf);
			cur = buf;
		}
	}
	else if(enabled && EMU_CURROW >= 0)	/* braindead terminal */
	{
		ttygetsb(&screen);	/* get screen base from ttyput routines */
		s = EMU_SCREEN;
		a = EMU_ATTRIB;
		for(i = 1; i <= EMU_NCOL * EMU_NROW; i++)
			if(*a++)
				*screen++ = 0x7000 | (Uchar)*s++;
			else
				*screen++ = 0x700 | (Uchar)*s++;
		ttyscreen(EMU_CURROW,EMU_CURCOL);
	}
}

/******************************************************************************\
|Routine: putz
|Callby: cfg command edit help init_term inquire load_key match_paren paint paint_window ref_window remove_win select slip_message unselect
|Purpose: High-level output routine; puts a string to the terminal.
|Arguments:
|    string is the string to put out.
\******************************************************************************/
void putz(string)
register Char *string;
{
	register Char *s,*a;
	register Int nskip,column;
	Char buf[2],skipbuf[5];	/* skipbuf is used to store characters that we may later have to spit out to reposition the cursor */
	Int off;

	buf[1] = '\0';
	off = EMU_NCOL * EMU_CURROW + EMU_CURCOL;
	s = EMU_SCREEN + off;
	a = EMU_ATTRIB + off;
	for(column = EMU_CURCOL,nskip = 0;*string && column < EMU_NCOL;string++,s++,a++,column++)
		if(*s == *string && *a == EMU_CURATT)
		{
			if(nskip <= 3)
				skipbuf[nskip] = *s;
			nskip++;
		}
		else
		{
			if(nskip > 4)
				right(nskip);
			else if(nskip > 0)
			{
				skipbuf[nskip] = '\0';
				put(skipbuf);
			}
			nskip = 0;
			buf[0] = *s = *string;
			*a = EMU_CURATT;
			put(buf);
		}
	if(nskip > 0)
		right(nskip);
	EMU_CURCOL = min(column,EMU_NCOL - 1);
}

/******************************************************************************\
|Routine: putoff
|Callby: edit init_term parse_fnm word_fill
|Purpose: Turns off all terminal i/o.
|Arguments:
|    none
\******************************************************************************/
void putoff()
{
	putout();
	enabled = 0;
}

/******************************************************************************\
|Routine: puton
|Callby: edit init_term word_fill
|Purpose: Resumes terminal i/o.
|Arguments:
|    none
\******************************************************************************/
void puton()
{
	putout();
	enabled = 1;
}

/******************************************************************************\
|Routine: puttest
|Callby: edit init_term insert paint
|Purpose: Test whether terminal i/o is enabled.
|Arguments:
|    none
\******************************************************************************/
Int puttest()
{
	return(enabled);
}

/******************************************************************************\
|Routine: putpurge
|Callby: parse_fnm
|Purpose: Make it as though nothing has been passed to put() yet.
|Arguments:
|    none
\******************************************************************************/
void putpurge()
{
	cur = buf;
}

