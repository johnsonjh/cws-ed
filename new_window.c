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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rec.h"
#include "window.h"
#include "ed_dec.h"

/******************************************************************************\
|Routine: new_window
|Callby: edit
|Purpose: Allocates a new window in the window database. Returns 1 for success,
|         0 if no more window slots are available.
|Arguments:
|    fname is the name of the file associated with the window.
\******************************************************************************/
Int new_window(fname)
Char *fname;
{
	if(NWINDOWS >= MAX_WINDOWS - 1)
		return(-1);	/* no window available */
	WINDOW[NWINDOWS].base = BASE;
	WINDOW[NWINDOWS].modified = 0;
	save_window();
	WINDOW[NWINDOWS].filename = (Char *)imalloc(strlen(fname) + 1);
	strcpy(WINDOW[NWINDOWS].filename,fname);
	return(NWINDOWS++);
}

