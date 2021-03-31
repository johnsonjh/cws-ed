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

#include "rec.h"
#include "window.h"
#include "ed_dec.h"

/******************************************************************************\
|Routine: get_seldir
|Callby: edit output_file sort_recs trim word_fill
|Purpose: Figures out in which direction the select marker lies.
|Arguments:
|    none
\******************************************************************************/
Int get_seldir()
{
	register rec_ptr forw,back;

	if(SELREC == CURREC)
	{
		if(SELBYT < CURBYT)
			return(-1);
		else if(SELBYT > CURBYT)
			return(1);
		else
			return(0);
	}
	if(SELREC == BASE)
		return(1);
	if(CURREC == BASE)
		return(-1);
	forw = back = CURREC;
	while(forw != SELREC && back != SELREC)
	{
		if((forw = forw->next) == BASE)
			return(-1);
		if((back = back->prev) == BASE)
			return(1);
	}
	if(forw == SELREC)
		return(1);
	else
		return(-1);
}

