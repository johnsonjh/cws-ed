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
#include "ed_dec.h"

/******************************************************************************\
|Routine: rec_chgcas
|Callby: change_case
|Purpose: Changes the case of a portion of a record.
|Arguments:
|    rec is the record affected.
|    byt1 is the first byte to change.
|    byt2 is the last byte to change.
|    table is the case-changing table, or NULL if they want capitalization.
\******************************************************************************/
void rec_chgcas(rec,byt1,byt2,table)
register rec_ptr rec;
register Int byt1,byt2;
register Uchar *table;
{
	register Uchar *p,*q;

	if(table)
		for(p = (Uchar *)(rec->data + byt1),q = (Uchar *)(rec->data + byt2);p != q;p++)
			*p = table[*p];
	else
	{
		rec->data[byt1] = toupper(rec->data[byt1]);
		for(p = (Uchar *)(rec->data + byt1 + 1),q = (Uchar *)(rec->data + byt2);p != q;p++)
			*p = (isspace(*(p - 1)) || ispunct(*(p - 1)))? toupper(*p) : tolower(*p);
	}
}

