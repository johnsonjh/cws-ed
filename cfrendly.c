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

#include "memory.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"	/* global data */
#include "buffer.h"
#include "ctyp_dec.h"

/******************************************************************************\
|Routine: cfrendly
|Callby: edit
|Purpose: Handles closure of curly braces in C programs.
|Arguments:
|    charbuf is the buffer holding the } they just typed.
|    multibuf,multirec are a buffer and record for insertion of multibyte data.
\******************************************************************************/
void cfrendly(charbuf,multibuf,multirec)
buf_ptr charbuf,multibuf;
rec_ptr multirec;
{
	register Int level,incomment,inquote,indoublequote,byt,save_auto,tbyt;
	register rec_ptr rec;
	register Char c,*p;

	rec = CURREC;
	level = 1;
	byt = CURBYT;
	incomment = 0;
	indoublequote = inquote = 0;
	while(level)	/* find the closing { */
	{
		if(--byt < 0)	/* we hit the beginning of the record */
		{
			if((rec = rec->prev) == BASE)
				break;
			byt = rec->length - 1;
		}
		if((c = rec->data[byt]) == '/')	/* check for comment termination */
		{
			if(byt)
				if(rec->data[byt - 1] == '*')
					incomment = 1;
		}
		else if(c == '*')	/* check for start-of-comment */
		{
			if(byt)
				if(rec->data[byt - 1] == '/')
					incomment = 0;
		}
		if(!incomment)
		{
			if(c == '"')	/* prevent "'" and '"' from confusing things */
			{
				if(!inquote)
					indoublequote = !indoublequote;
			}
			else if(c == '\'')
			{
				if(!indoublequote)
				{
					if(!byt)
						inquote = !inquote;
					else if(inquote)
					{
						if(rec->data[byt - 1] != '\\')
							inquote = !inquote;
					}
					else
						inquote = 1;
				}
			}
			else if(!inquote && !indoublequote)
			{
				if(c == '}')
					level++;
				else if(c == '{')
					level--;
			}
		}
	}
	if(level)	/* no closer found, just insert the } */
		insert(charbuf);
	else
	{
		for(byt = 0;byt < rec->length;byt++)	/* count initial whitespace in closure record */
			if(!isspace(rec->data[byt]))
				break;	/* this break will eventually be taken because rec contains a { */
		if(--byt < 0 && rec->length > 1)	/* if matching record has no initial whitespace, and isn't just a {, just insert the } */
			insert(charbuf);
		else	/* back up to last initial whitespace character */
		{
			for(tbyt = 0;tbyt < CURREC->length;tbyt++)	/* check whether the current record contains only whitespace */
				if(!isspace(CURREC->data[tbyt]))
				{
					if(tbyt < CURREC->length)	/* the current record contains nonwhitespace data, insert RETURN */
					{
						multirec->data = (Char *)imalloc(++byt + 2);	/* create buffer big enough to hold byt bytes plus curly */
						multirec->length = ++byt;
						multirec->data[byt] = '\0';	/* terminate buffer */
						multirec->data[--byt] = '}';	/* add curly */
						while(--byt >= 0)	/* copy byt bytes from record containing matching { */
							multirec->data[byt] = rec->data[byt];
						save_auto = TAB_AUTO;	/* disable auto tabs temporarily */
						TAB_AUTO = 0;
						carriage_return(1);	/* insert a carriage return */
						TAB_AUTO = save_auto;
						insert(multibuf);	/* insert whitespace and } */
						ifree(multirec->data);	/* toss buffer */
						return;
					}
				}
/* the current record is all whitespace, excise it all and then insert */
			if(CURREC == BASE)		/* CWS 11/93 eob bug */
			{
				carriage_return(1);
				left_arrow(1);
			}
			p = (Char *)imalloc(++byt + 2);	/* get new buffer for CURREC */
			memcpy(p,rec->data,byt);	/* copy the first byt bytes to it */
			CURREC->length = ++byt;	/* set the length as byt + 1 for curly brace */
			p[byt - 1] = '}';	/* put in the curly */
			p[byt] = '\0';	/* terminate the buffer */
			if(CURREC->recflags & 1)
				ifree(CURREC->data);	/* free the old buffer */
			CURREC->data = p;	/* replace it with the new one */
			CURREC->recflags = 1;	/* mark it as freeable */
			if(CURREC == SELREC && SELBYT > byt)	/* move SELBYT to fit within new record */
				SELBYT = byt;
			paint(CURROW,CURROW,FIRSTCOL);	/* display the record */
			CURBYT = byt;	/* put cursor right after curly */
			fix_scroll();
		}
	}
}

