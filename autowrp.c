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

#include "ctyp_dec.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"	/* global data */
#include "keyvals.h"

/******************************************************************************\
|Routine: autowrp
|Callby: edit
|Purpose: Handles autowrapping when cursor moves past wrap margin. Returns 1 if
|         autowrapping was possible, otherwise 0.
|Arguments:
|    keybuf is the key they just pressed.
\******************************************************************************/
Int autowrp(keybuf)
Schar keybuf;
{
	register Int j,k,nspaces,nleft;
	
	for(j = CURBYT;j > 0;j--)	/* find a blank char backwards from current position */
		if(isspace(CURREC->data[j - 1]) && (j <= WRAP_MARGIN + 1))
			break;
	if(j)	/* if no blank chars, skip it */
	{
		nleft = CURBYT - j;	/* number of times to move left before starting to delete blanks */
		for(k = j - 1;k > 0;k--)	/* find a nonblank char backwards from blank char */
			if(!isspace(CURREC->data[k - 1]))
				break;
		nspaces = j - k;	/* number of blank chars to delete */
		emulate_key((Schar)KEY_LEFTARROW,nleft);	/* load nleft left moves */
		emulate_key((Schar)KEY_DELETE,nspaces);	/* delete leading spaces if any */
		emulate_key((Schar)KEY_CARRIAGERETURN,1);	/* load cr */
		emulate_key((Schar)KEY_RIGHTARROW,nleft);	/* load nleft right moves */
		emulate_key(keybuf,1);	/* load the character they typed */
		return(1);
	}
	else
		return(0);
}

