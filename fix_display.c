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
|Routine: fix_display
|Callby: bigger_win down_arrow fix_scroll left_arrow move_eol move_word right_arrow up_arrow
|Purpose: Tries to put the cursor in the free-movement region.
|Arguments:
|    none
\******************************************************************************/
void fix_display()
{
	if(CURROW > BOTLIM)
		CURROW -= scroll_up(CURROW - BOTLIM);
	else if(CURROW<TOPLIM)
		CURROW += scroll_down(TOPLIM - CURROW);
}

