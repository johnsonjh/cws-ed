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

#include "memory.h"
#include "ctyp_dec.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "cmd_enum.h"
#include "cmd_dec.h"

/******************************************************************************\
|Routine: inscmp
|Callby: set_param
|Purpose: Does what strcmp does, without case-sensitivity.
|Arguments:
|    s1,s2 are the strings being compared.
\******************************************************************************/
Int inscmp(s1,s2)
register Char *s1,*s2;
{
	Char l1[128],l2[128];
	register Char *p,*q,c;

	for(p = (Char *)s1,q = l1;(c = *p++);)
		*q++ = tolower(c);
	*q = '\0';
	for(p = (Char *)s2,q = l2;(c = *p++);)
		*q++ = tolower(c);
	*q = '\0';
	return(strcmp(l1,l2));
}

/******************************************************************************\
|Routine: decode_error
|Callby: set_param
|Purpose: Handles bad user input to numeric questions.
|Arguments:
|    p is the description of what the user was asked for.
\******************************************************************************/
void decode_error(p)
Char *p;
{
	Char buf[256];

	strcpy(buf,"Unable to understand the ");
	strcat(buf,p);
	strcat(buf,".");
	slip_message(buf);
	wait_message();
}

/******************************************************************************\
|Routine: set_param
|Callby: command
|Purpose: Lets user set editing parameters.
|Arguments:
|    choice is the number of the paramter user wants to set.
|    buf is the buffer containing the value the user wants for the parameter.
|    bufl is the length of the user buffer.
|    l is the length of the data in buf.
|    i is the current position in buf.
\******************************************************************************/
void set_param(choice,buf,bufl,l,i)
Int choice,bufl,l,i;
Char *buf;
{
	Int j,crterm;

	for(;i < l;i++)	/* skip leading whitespace before value */
		if(!isspace(buf[i]))
			break;
	if(i == l || (choice >= (int)PAR_WORD && choice <= (int)PAR_PARAGRAPH))	/* WORD and PARAGRAPH require explicit input */
	{
		if(choice >= (int)PAR_WORD && choice <= (int)PAR_PARAGRAPH)
			crterm = 0;
		else
			crterm = 1;
		if(!(l = inquire(prompts[choice],buf,bufl,crterm)))
			return;
		buf[l] = '\0';
		i = 0;
	}
	switch(choice)
	{
		case PAR_SHELL:
			if((j = strlen(buf + i)) > sizeof(USERSHELL) - 1)
				j = sizeof(USERSHELL) - 1;
			memcpy(USERSHELL,buf + i,j);
			USERSHELL[j] = '\0';
			break;
		case PAR_TABS:
			for(j = 0;j < TAB_SETUPS;j++)
				if(!inscmp(buf + i,TAB_NAME[j]))
					break;
			if(j == TAB_SETUPS)
			{
				strcpy(buf,"Unknown tab setup. Use one of (");
				for(j = 0;j < TAB_SETUPS;j++)
				{
					strcat(buf,TAB_NAME[j]);
					if(j < TAB_SETUPS - 2)
						strcat(buf,", ");
					else if(j < TAB_SETUPS - 1)
						strcat(buf," or ");
				}
				strcat(buf,").");
				slip_message(buf);
				wait_message();
			}
			else
			{
				CUR_TAB_SETUP = j;
				CURCOL = get_column(CURREC,CURBYT);
				ref_display();
			}
			break;
		case PAR_WRAP:
			if((my_sscanf(buf + i,"%d",&j)) != 1)
				decode_error(prompts[choice]);
			else
				WRAP_MARGIN = j;
			break;
		case PAR_SECTION:
			if((my_sscanf(buf + i,"%d",&j)) != 1)
				decode_error(prompts[choice]);
			else
				SECTION_LINES = j;
			break;
		case PAR_WORD:
			memcpy(WORD_DELIMITERS,buf,l);
			NWORD_DELIMITERS = l;
			init_word_table(NWORD_DELIMITERS,WORD_DELIMITERS);
			break;
		case PAR_PAGE:
			memcpy(PAGE_BREAK,buf,l);
			PAGE_BREAK_LENGTH = l;
			break;
		case PAR_PARAGRAPH:
			memcpy(PARAGRAPH_BREAK,buf,l);
			PARAGRAPH_BREAK_LENGTH = l;
			break;
		case PAR_SEARCH:
			if((choice = parse_command(0,buf,bufl,&l,&i,smodes,NUM_SMODES,"search mode")) >= 0)
			{
				if(choice == (int)SMODE_TABLE_DRIVEN)	/* SET SEARCH TABLE */
				{
					if(!set_search_table(SEARCH_TABLE))
					{
						SEARCH_FLAGS[((int)SMODE_TABLE_DRIVEN) >> 1] = 'n';
						slip_message("Unable to read the search table.");
						wait_message();
						break;
					}
				}
				SEARCH_FLAGS[choice >> 1] = *smodes[choice];
			}
			break;
		case PAR_STABLE:
			if((j = strlen(buf + i)) > sizeof(SEARCH_TABLE) - 1)
				j = sizeof(SEARCH_TABLE) - 1;
			memcpy(SEARCH_TABLE,buf + i,j);
			SEARCH_TABLE[j] = '\0';
			break;
		case PAR_CLOSE_PARENS:
			if((choice = parse_command(0,buf,bufl,&l,&i,offon,3,"paren matching status")) >= 0)
				CLOSE_PARENS = ((choice == 2)? !CLOSE_PARENS : choice);
			break;
		case PAR_PARENS:
			if((j = strlen(buf + i)) > sizeof(PAREN_STRING) - 1)
				j = sizeof(PAREN_STRING) - 1;
			memcpy(PAREN_STRING,buf + i,j);
			PAREN_STRING[j] = '\0';
			PAREN_STRING_LENGTH = j;
			break;
		case PAR_DEFAULT:
			if((j = strlen(buf + i)) > sizeof(DEFAULT_EXT) - 1)
				j = sizeof(DEFAULT_EXT) - 1;
			memcpy(DEFAULT_EXT,buf + i,j);
			DEFAULT_EXT[j] = '\0';
			DEFAULT_EXT_LENGTH = j;
			break;
		case PAR_AUTO_TABS:
			if((choice = parse_command(0,buf,bufl,&l,&i,offon,3,"auto tabs status")) >= 0)
				TAB_AUTO = ((choice == 2)? !TAB_AUTO : choice);
			break;
		case PAR_CFRIENDLY:
			if((choice = parse_command(0,buf,bufl,&l,&i,offon,3,"C-friendly status")) >= 0)
				CFRIENDLY = ((choice == 2)? !CFRIENDLY : choice);
			break;
		case PAR_BOXCUT:
			if((choice = parse_command(0,buf,bufl,&l,&i,offon,3,"box cut status")) >= 0)
				BOXCUT = ((choice == 2)? !BOXCUT : choice);
			break;
		case PAR_AUTOWRAP:
			if((choice = parse_command(0,buf,bufl,&l,&i,offon,3,"auto wrap")) >= 0)
				AUTOWRAP = ((choice == 2)? !AUTOWRAP : choice);
			break;
		case PAR_CASE:
			if((choice = parse_command(0,buf,bufl,&l,&i,cases,NUM_CASES,"case change")) >= 0)
				CASECHANGE = choice;
			break;
		case PAR_OVERSTRIKE:
			if((choice = parse_command(0,buf,bufl,&l,&i,offon,3,"overstrike status")) >= 0)
				OVERSTRIKE = ((choice == 2)? !OVERSTRIKE : choice);
			break;
		case PAR_GREPMODE:
			if((choice = parse_command(0,buf,bufl,&l,&i,grepmodes,3,"GREP mode")) >= 0)
				GREPMODE = ((choice == 2)? !GREPMODE : choice);
			break;
		case PAR_WILDCARD:
			WILDCARD = buf[i];
			break;
	}
}

