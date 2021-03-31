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

#include <string.h>

#include "rec.h"
#include "window.h"
#include "ed_dec.h"

/******************************************************************************\
|Routine: window_title
|Callby: define_key ref_window
|Purpose: Returns the window title string.
|Arguments:
|    w is the window number.
\******************************************************************************/

Char *window_title(w)
Int w;
{
	static Char buf[512];
	
	if(definition_inprog() == w)
		strcpy(buf,"Defining key...");
	else
	{
		if(host_in_name(WINDOW[w].filename))
			switch(ftp_system(WINDOW[w].filename))
			{
				case 'U':
					strcpy(buf,"(Unix)");
					break;
				case 'M':
					strcpy(buf,"(MS-DOS)");
					break;
				case 'W':
					strcpy(buf,"(WNT)");
					break;
				case 'O':
					strcpy(buf,"(OS/2)");
					break;
				case 'V':
					strcpy(buf,"(VMS)");
					break;
				case 'I':
					strcpy(buf,"(VM)");
					break;
				default:
					strcpy(buf,"(unknown system)");
			}
		else
			buf[0] = '\0';
		strcat(buf,WINDOW[w].filename);
	}
	return(buf);
}

/******************************************************************************\
|Routine: ref_window
|Callby: bigger_win edit insert_win ref_display scroll_down scroll_up slip_message smaller_win sort_recs
|Purpose: Refreshes a particular window.
|Arguments:
|    w is the window to refresh.
\******************************************************************************/
void ref_window(w)
Int w;
{
	Char *p;	/* to hold, possibly, system type, and, definitely, either "Defining key..." or filename */
	
	p = window_title(w);
	if(w == CURWINDOW)
	{
		move(TOPROW - 1,1);
		ers_end();
		reverse();
		putz(p);
		normal();
		paint(TOPROW,BOTROW,FIRSTCOL);
	}
	else
	{
		move(WINDOW[w].toprow - 1,1);
		ers_end();
		reverse();
		putz(p);
		normal();
		paint_window(w,WINDOW[w].toprow,WINDOW[w].botrow,WINDOW[w].firstcol);
	}
}

