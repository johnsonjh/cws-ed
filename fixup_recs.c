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
|Routine: fixup_recs
|Callby: rec_insert rec_merge rec_trim
|Purpose: Keep TOPREC,BOTREC, etc. in line when it is necessary to dump a record.
|Arguments:
|    old is the dumped record.
|    new is the new record which takes its place.
\******************************************************************************/
void fixup_recs(old,new)
rec_ptr old,new;
{
	if(old == TOPREC)
		TOPREC = new;
	if(old == BOTREC)
		BOTREC = new;
	if(old == CURREC)
		CURREC = new;
	if(old == NEWTOPREC)
		NEWTOPREC = new;
}

