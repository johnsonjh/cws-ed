/*
 * Copyright (C) 1992 by Rush Record
 * Copyright (C) 1993 by Charles Sandmann (sandmann@clio.rice.edu)
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
|Routine: unselect
|Callby: command edit inquire set_window sort_recs trim wincom word_fill
|Purpose: Turns off the select marker.
|Arguments:
|    none
\******************************************************************************/
void unselect()
{
	register rec_ptr r;
	register Int selrow,selcol;
	Char b[2];

	if(!SELREC)
		return;
	for(r = TOPREC,selrow = TOPROW;r != BASE && r != BOTREC;r = r->next,selrow++)
		if(r == SELREC)
			break;
	if(r == SELREC)
	{
		if(SELREC == BASE)
		{
			move(selrow,1);
			eob();
			cr();
		}
		else
		{
			selcol = get_column(SELREC,SELBYT) - FIRSTCOL + 1;
			if(selcol > 0 && selcol <= NCOL)
			{
				move(selrow,selcol + FIRSTCOL - 1);
				putz(" ");
				move(selrow,selcol + FIRSTCOL - 1);
				if(SELBYT || SELREC->length)
				{
					b[0] = SELREC->data[SELBYT];
					if(b[0] != '\t' && SELBYT != SELREC->length)
					{
						if(NONPRINTNCS(b[0]) || HEXMODE)
							b[0] = '<';
						b[1] = '\0';
						putz(b);
						move(selrow,selcol + FIRSTCOL - 1);
					}
				}
			}
		}
	}
	SELREC = 0;
}

