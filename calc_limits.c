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

#include "handy.h"

/******************************************************************************\
|Routine: calc_limits
|Callby: bigger_win edit insert_win remove_win smaller_win
|Purpose: Calculates the area in which the cursor may move without causing
|         the window to scroll.
|Arguments:
|    top,bot are the top and bottom rows of the window.
|    topl,botl are the top and bottom rows of the free-movement area.
\******************************************************************************/
void calc_limits(top,bot,topl,botl)
Int top,bot,*topl,*botl;
{
	Int wid;

	if((wid = bot - top + 1) < 3)
	{
		*topl = top;
		*botl = bot;
	}
	else
	{
		*topl = max(top + 1,top + wid / 3);
		*botl = min(bot - 1,bot - wid / 3);
	}
}

