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
|Routine: fix_botrec
|Callby: fix_scroll remove_win
|Purpose: Tries to scroll the screen down to move [eob] to the bottom.
|Arguments:
|    none
\******************************************************************************/
void fix_botrec()
{
	register rec_ptr r;
	register Int i;

	if(!BOTREC)
	{
		for(r = TOPREC,i = TOPROW;r != BASE;r = r->next,i++);
		CURROW += scroll_down(BOTROW - i);
	}
}

