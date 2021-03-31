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
|Routine: save_window
|Callby: bigger_win command edit insert_win new_window remove_win smaller_win wincom
|Purpose: Saves the current window parameters in the window database.
|Arguments:
|    none
\******************************************************************************/
void save_window()
{
	if(CURWINDOW < 0)
		return;
	WINDOW[CURWINDOW].currec = CURREC;
	WINDOW[CURWINDOW].toprec = TOPREC;
	WINDOW[CURWINDOW].botrec = BOTREC;
	WINDOW[CURWINDOW].curbyt = CURBYT;
	WINDOW[CURWINDOW].toprow = TOPROW;
	WINDOW[CURWINDOW].botrow = BOTROW;
	WINDOW[CURWINDOW].toplim = TOPLIM;
	WINDOW[CURWINDOW].botlim = BOTLIM;
	WINDOW[CURWINDOW].currow = CURROW;
	WINDOW[CURWINDOW].curcol = CURCOL;
	WINDOW[CURWINDOW].firstcol = FIRSTCOL;
	WINDOW[CURWINDOW].wantcol = WANTCOL;
}

