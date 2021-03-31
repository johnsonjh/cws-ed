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
#include <string.h>

#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "cmd_enum.h"
#include "cmd_dec.h"

extern Char *version();

/******************************************************************************\
|Routine: show_param
|Callby: command
|Purpose: Handles the display of editor settings.
|Arguments:
|    choice is the number of the setting to display, or -1 to display all settings.
\******************************************************************************/
void show_param(choice)
Int choice;
{
	Int start,l,i;
	Int record,byte;
	Char buf[256],save_hexmode,*ebuf;

	save_hexmode = HEXMODE;
	HEXMODE = 0;
	if(choice < 0)
	{
		start = 0;
		choice = (int)NUM_PARAMS - 2;	/* don't do SHOW KEYS */
	}
	else
		start = choice;
	for(;start <= choice;start++)
	{
		strcpy(buf,prompts[start]);
		strcat(buf,": ");
		l = strlen(buf);
		switch(start)
		{
		case PAR_SHELL:
			strcat(buf,USERSHELL);
			break;
		case PAR_TABS:
			strcat(buf,TAB_NAME[CUR_TAB_SETUP]);
			break;
		case PAR_WRAP:
			sprintf(buf + l,"%d",WRAP_MARGIN);
			break;
		case PAR_SECTION:
			sprintf(buf + l,"%d",SECTION_LINES);
			break;
		case PAR_WORD:
			express(NWORD_DELIMITERS,WORD_DELIMITERS,0,0,&ebuf,256 - l,1);
			strcpy(buf + l,ebuf);
			break;
		case PAR_PAGE:
			express(PAGE_BREAK_LENGTH,PAGE_BREAK,0,0,&ebuf,256 - l,1);
			strcpy(buf + l,ebuf);
			break;
		case PAR_PARAGRAPH:
			express(PARAGRAPH_BREAK_LENGTH,PARAGRAPH_BREAK,0,0,&ebuf,256 - l,1);
			strcpy(buf + l,ebuf);
			break;
		case PAR_SEARCH:
			for(i = 0;i < NUM_SMODES;i += 2)
			{
				strcat(buf,"<");
				strcat(buf,sprompts[(SEARCH_FLAGS[i >> 1] == *smodes[i])? i : i+1]);
				strcat(buf,"> ");
			}
			break;
		case PAR_STABLE:
			strcat(buf,SEARCH_TABLE);
			break;
		case PAR_CLOSE_PARENS:
			strcat(buf,offon[(CLOSE_PARENS)? 1 : 0]);
			break;
		case PAR_PARENS:
			strcat(buf,PAREN_STRING);
			break;
		case PAR_DEFAULT:
			strcat(buf,DEFAULT_EXT);
			break;
		case PAR_AUTO_TABS:
			strcat(buf,offon[(TAB_AUTO)? 1 : 0]);
			break;
		case PAR_CFRIENDLY:
			strcat(buf,offon[(CFRIENDLY)? 1 : 0]);
			break;
		case PAR_BOXCUT:
			strcat(buf,offon[(BOXCUT)? 1 : 0]);
			break;
		case PAR_AUTOWRAP:
			strcat(buf,offon[(AUTOWRAP)? 1 : 0]);
			break;
		case PAR_CASE:
			strcat(buf,caseprompts[CASECHANGE]);
			break;
		case PAR_OVERSTRIKE:
			strcat(buf,offon[(OVERSTRIKE)? 1 : 0]);
			break;
		case PAR_GREPMODE:
			strcat(buf,grepmodes[(int)GREPMODE]);
			break;
		case PAR_WILDCARD:
			buf[strlen(buf) + 1] = '\0';
			buf[strlen(buf)] = WILDCARD;
			break;
		case PAR_POSITION:
			get_position(&record,&byte);
			sprintf(buf + l,"Record: %d, Byte: %d, Row: %d, Column: %d",record,byte,CURROW,CURCOL);
			break;
		case PAR_MODIFIED:
			strcat(buf,(WINDOW[CURWINDOW].modified)? "Yes" : "No");
			break;
		case PAR_VERSION:
			sprintf(buf + l,"%s",version());
			break;
		}
		slip_message(buf);
	}
	wait_message();
	paint(BOTROW,BOTROW,FIRSTCOL);
	HEXMODE = save_hexmode;
}

