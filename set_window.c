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
|Routine: set_window
|Callby: bigger_win edit insert_win remove_win smaller_win wincom
|Purpose: Loads current editing parameters from the window database. Used when
|         switching windows.
|Arguments:
|    n is the number of the window to load from (zero-based).
\******************************************************************************/
void set_window(n)
Int n;
{
	if(n != CURWINDOW)
		unselect();
	BASE = WINDOW[n].base;
	CURREC = WINDOW[n].currec;
	TOPREC = WINDOW[n].toprec;
	BOTREC = WINDOW[n].botrec;
	CURBYT = WINDOW[n].curbyt;
	TOPROW = WINDOW[n].toprow;
	BOTROW = WINDOW[n].botrow;
	TOPLIM = WINDOW[n].toplim;
	BOTLIM = WINDOW[n].botlim;
	CURROW = WINDOW[n].currow;
	CURCOL = WINDOW[n].curcol;
	FIRSTCOL = WINDOW[n].firstcol;
	WANTCOL = WINDOW[n].wantcol;
	HEXMODE = (WINDOW[n].binary == 2);
	CURWINDOW = n;
	marge(TOPROW,BOTROW);
}

