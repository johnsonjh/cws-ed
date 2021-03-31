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
|Routine: backspace
|Callby: edit move_line
|Purpose: Implements the EDT-like behavior of the backspace key (control-H).
|         If at the beginning of a line, move up, else, move to the beginning
|         of the line.
|Arguments:
|    repeat is the repeat count applied to the action.
\******************************************************************************/
void backspace(repeat)
register Int repeat;
{
	if(CURCOL == 1)
		up_arrow(repeat);
	else
	{
		CURBYT = 0;
		CURCOL = 1;
		if(repeat > 1)
		{
			WANTCOL = 1;
			up_arrow(repeat - 1);
		}
	}
	WANTCOL = CURCOL;
}

