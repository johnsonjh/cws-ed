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

#include <string.h>

#include "memory.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"

static Char word_table[256];

/******************************************************************************\
|Routine: move_word
|Callby: edit word_fill
|Purpose: Moves the cursor a number of words forward or backward.
|Arguments:
|    repeat is the number of words to move.
\******************************************************************************/
void move_word(repeat)
register Int repeat;
{
	register Uchar *c,*e;

	c = (Uchar *)&CURREC->data[CURBYT];
	if(DIRECTION > 0)
		while(repeat-- > 0)
		{
			if(CURREC == BASE)
			{
				fix_display();
				WANTCOL = CURCOL = 1;
				abort_key();
				return;
			}
			if(CURBYT == CURREC->length)
			{
				CURREC = CURREC->next;
				CURBYT = 0;
				c = (Uchar *)&CURREC->data[CURBYT];
				CURROW++;
			}
			else
			{
				e = (Uchar *)&CURREC->data[CURREC->length];
				while(!word_table[*c] && c != e)	/* get into some delimiters */
					c++;
				if(c == e)	/* hit end of record */
					CURBYT = CURREC->length;
				else
				{
					c++;
					while(word_table[*c] && c != e)
						c++;
					e = (Uchar *)CURREC->data;
					CURBYT = c - e;
				}
			}
		}
	else	/* direction is backwards */
		while(repeat-- > 0)
		{
			if(!CURBYT)
			{
				if(CURREC->prev == BASE)
				{
					fix_display();
					WANTCOL = CURCOL = 1;
					abort_key();
					return;
				}
				CURREC = CURREC->prev;
				CURBYT = CURREC->length;
				c = (Uchar *)&CURREC->data[CURBYT];
				CURROW--;
			}
			else
			{
				e = (Uchar *)CURREC->data;
				c--;
				while(word_table[*c] && c != e)	/* get into some non-delimiters */
					c--;
				if(c == e)	/* hit end of record */
					CURBYT = 0;
				else
				{
					c--;
					while(!word_table[*c] && c != e)      /* find a delimiter */
						c--;
					if(c == e)
					{
						if(word_table[*c])
							c++;
					}
					else
						c++;
					CURBYT = c - e;
				}
			}
		}
	CURCOL = get_column(CURREC,CURBYT);
	WANTCOL = CURCOL;
	fix_display();
}

/******************************************************************************\
|Routine: find_word
|Callby: edit word_fill
|Purpose: Does what move_word does, without actually moving the cursor. Instead,
|         it returns the new position of the cursor (if it had actually moved)
|         as an offset from the current position.
|Arguments:
|    prepeat is the number of words to (figuratively) move.
\******************************************************************************/
Int find_word(prepeat)
register Int prepeat;
{
	register Uchar *c,*e;
	rec_ptr currec;
	register Int repeat,curbyt;
	Int offset;

	currec = CURREC;
	curbyt = CURBYT;
	c = (Uchar *)&currec->data[curbyt];
	offset = 0;
	if(prepeat > 0)
	{
		repeat = prepeat;
		while(repeat-- > 0)
		{
			if(currec == BASE)
				return(0);
			if(curbyt == currec->length)
			{
				currec = currec->next;
				curbyt = 0;
				c = (Uchar *)&currec->data[curbyt];
				offset++;
			}
			else
			{
				e = (Uchar *)&currec->data[currec->length];
				while(!word_table[*c] && c != e)	/* get into some delimiters */
				{
					c++;
					offset++;
				}
				if(c == e)	/* hit end of record */
					curbyt = currec->length;
				else
				{
					c++;
					offset++;
					while(word_table[*c] && c != e)
					{
						c++;
						offset++;
					}
					e = (Uchar *)currec->data;
					curbyt = c - e;
				}
			}
		}
	}
	else	/* direction is backwards */
	{
		repeat = -prepeat;
		while(repeat-- > 0)
		{
			if(!curbyt)
			{
				if(currec->prev == BASE)
					return(0);
				currec = currec->prev;
				curbyt = currec->length;
				c = (Uchar *)&currec->data[curbyt];
				offset--;
			}
			else
			{
				e = (Uchar *)currec->data;
				c--;
				offset--;
				while(word_table[*c] && c != e)	/* get into some non-delimiters */
				{
					c--;
					offset--;
				}
				if(c == e)	/* hit end of record */
					curbyt = 0;
				else
				{
					c--;
					offset--;
					while(!word_table[*c] && c != e)      /* find a delimiter */
					{
						c--;
						offset--;
					}
					if(c != e)
					{
						c++;
						offset++;
					}
					curbyt = c - e;
				}
			}
		}
	}
	return(offset);
}

/******************************************************************************\
|Routine: find_word_string
|Callby: inquire
|Purpose: Finds the beginning of a word in the user buffer used in
|         the inquire routine. Returns the index in the buffer of the word's
|         first character.
|Arguments:
|    repeat is the number of words to (figuratively) back up.
|    string is the user buffer.
|    length is the length of the data in string.
|    cursor is the cursor position in the buffer.
|    direct is the search direction (-1 = backward, 1 = forward).
\******************************************************************************/
Int find_word_string(repeat,string,length,cursor,direct)
register Int repeat;
Char *string;
Int length,cursor,direct;
{
	register Uchar *c,*e;

	if(direct == -1)
	{
		if(!cursor)
			return(0);
		e = (Uchar *)string;
		c = e + cursor - 1;
		while(repeat--)
		{
			while(word_table[*c])	/* get into some non-delimiters */
				if(c-- == e)
					return(0);
			while(!word_table[*c])      /* find a delimiter */
				if(c-- == e)
					return(0);
		}
		return(((Char *)c) - string + 1);
	}
	else
	{
		c = (Uchar *)string + cursor;
		e = (Uchar *)string + length;
		while(repeat--)
		{
			while(!word_table[*c] && c != e)	/* get into some delimiters */
				c++;
			if(c == e)	/* hit end of record */
				break;
			c++;
			while(word_table[*c] && c != e)
				c++;
			if(c == e)	/* hit end of record */
				break;
		}
		return(((Char *)c) - string);
	}
}

/******************************************************************************\
|Routine: init_word_table
|Callby: restore_par set_param word_fill
|Purpose: Initializes the word delimiter list.
|Arguments:
|    ndelim is the number of word delimiters.
|    delims is the array of word delimiters.
\******************************************************************************/
void init_word_table(ndelim,delims)
Int ndelim;
Uchar *delims;
{
	memset(word_table,0,256);
	for(;ndelim > 0;ndelim--)
		word_table[*delims++] = 1;
}

/******************************************************************************\
|Routine: wordtable
|Callby: match_search
|Purpose: Returns the address of the current word-delimiter table.
|Arguments:
|    none
\******************************************************************************/
Char *wordtable()
{
	return(word_table);
}

/******************************************************************************\
|Routine: get_word_start
|Callby: match_search
|Purpose: Finds the next word. Returns 0 on failure. Returns 1 if we are sitting
|         on a word, else finds a beginning of a word in the forward direction.
|Arguments:
|    base is the record queue base.
|    pr is the current record pointer.
|    cur is the starting position for the word search.
|    length is the returned length of the word.
|    poffset is the search offset, which can end up getting modified here.
\******************************************************************************/
Int get_word_start(base,pr,pcur,length,poffset)
rec_ptr base,*pr;
Int *pcur,*length;
Int *poffset;
{
	rec_ptr r;
	Int cur,end;
	Int offset;
	
	r = *pr;
	cur = *pcur;
	if(poffset)
		offset = *poffset;
	else
		offset = 0;
	if(cur >= r->length)
		goto advance;
	if(r->length && cur && !word_table[(Uchar)r->data[cur]])
		if(word_table[(Uchar)r->data[cur - 1]])
			goto report;	/* we are already at the beginning of a word */
	if(cur)	/* if we are not at the beginning of a line... */
	{
		if(!word_table[(Uchar)r->data[cur]])	/* if we are on a non-delimiter... */
		{
			if(!word_table[(Uchar)r->data[cur - 1]])	/* and the previous character is a non-delimiter... */
			{	/* we need to advance to the end of the current word before advancing */
				while(cur < r->length && !word_table[(Uchar)r->data[cur]])	/* find a delimiter */
				{
					cur++;
					offset++;
				}
advance:
				while(cur < r->length && word_table[(Uchar)r->data[cur]])	/* find a non-delimiter */
				{
					cur++;
					offset++;
				}
				if(cur >= r->length)
				{
					if(r == base || (r = r->next) == base)
						return(0);
					cur = 0;
					offset++;
					goto advance;
				}
			}
		}
		else
			goto advance;
	}
	else
		if(!r->length || word_table[(Uchar)r->data[0]])	/* we are at the beginning of a line, and the first char is a delimiter */
			goto advance;
/* cur is now at the beginning of a word */
report:
	end = cur + 1;
	offset++;
	while(end < r->length && !word_table[(Uchar)r->data[end]])
	{
		end++;
		offset++;
	}
	*pr = r;
	*pcur = cur;
	*length = end - cur;
	if(poffset)
		*poffset = offset;
	return(1);
}

/******************************************************************************\
|Routine: get_word_end
|Callby: match_search
|Purpose: Finds the next end of a word. Returns 0 on failure. Returns 1 if we
|         are sitting on a wordend, else finds a wordend in the backward
|         direction.
|Arguments:
|    base is the record queue base.
|    pr is the current record pointer.
|    cur is the starting position for the word search.
|    length is the returned length of the word.
|    poffset is the search offset, which can end up getting modified here.
\******************************************************************************/
Int get_word_end(base,pr,pcur,length,poffset)
rec_ptr base,*pr;
Int *pcur,*length;
Int *poffset;
{
	rec_ptr r;
	Int cur,end;
	Int offset;
	
	r = *pr;
	cur = *pcur;
	if(poffset)
		offset = *poffset;
	else
	{
		if(!cur)	/* search string must be only one record, and we have hit the beginning of it */
			return(0);
		offset = 0;
	}
/* if we are at <end>, just back up */
	if(poffset && r == base)
	{
		if((r = r->prev) == base)
			return(0);	/* empty file */
		cur = r->length;
		offset--;
	}
	else	/* find a delimiter, going backwards */
	{
		if(cur < r->length)	/* if cur is equal to r->length, we are on a virtual delimiter, no need to look for one */
			while(!word_table[(Uchar)r->data[cur]])
			{
				if(--cur <= 0)	/* we hit the start of the record, which can never be a wordend */
				{
					while((r = r->prev) != base)
					{
						if((end = r->length))
						{
							while(end--)
								if(!word_table[(Uchar)r->data[end]])
									break;
							if(end >= 0)
								break;
						}
						offset -= r->length + 1;
					}
					if(r == base)
						return(0);	/* cannot match at beginning of file */
					cur = r->length - 1;
					offset -= 2;
					break;
				}
				offset--;
			}
		else
		{
			cur--;
			offset--;
		}
	}
/* now find a non-delimiter. this is the wordend - 1 */
	while(word_table[(Uchar)r->data[cur]])
	{
		if(--cur < 0)
		{
			if((r = r->prev) == base)
				return(0);
			cur = r->length - 1;
			offset--;
		}
		offset--;
	}
/* find the character before the beginning of this word */
	end = cur + 1;
	while(!word_table[(Uchar)r->data[cur]])
	{
		offset--;
		if(--cur < 0)
			break;
	}
	cur++;
	offset++;
/* cur is now at the beginning of a word */
	*pr = r;
	*pcur = cur;
	*length = end - cur;
	if(poffset)
		*poffset = offset;
	return(1);
}

/******************************************************************************\
|Routine: on_a_word
|Callby: match_search
|Purpose: Returns whether or not the specified position is at the start of a
|         word.
|Arguments:
|    rec is the record.
|    byt is the byte.
\******************************************************************************/
Int on_a_word(rec,byt)
rec_ptr rec;
Int byt;
{
	if(byt >= rec->length)
		return(0);
	if(!byt)
	{
		if(!word_table[(Uchar)rec->data[0]])
			return(1);
		return(0);
	}
	if(word_table[(Uchar)rec->data[byt]])
		return(0);
	if(!word_table[(Uchar)rec->data[byt - 1]])
		return(0);
	return(1);
}

/******************************************************************************\
|Routine: on_a_wordend
|Callby: match_search
|Purpose: Returns whether or not the specified position is past the end of a
|         word.
|Arguments:
|    rec is the record.
|    byt is the byte.
\******************************************************************************/
Int on_a_wordend(rec,byt)
rec_ptr rec;
Int byt;
{
	if(!byt)	/* beginning of record is never a wordend */
		return(0);
	if(byt < rec->length)
		if(!word_table[(Uchar)rec->data[byt]])	/* must be on a delimiter */
			return(0);
	if(!word_table[(Uchar)rec->data[byt - 1]])
		return(1);
	return(0);
}

