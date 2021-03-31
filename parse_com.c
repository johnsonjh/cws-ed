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

#include <string.h>

#include "memory.h"
#include "ctyp_dec.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"

/******************************************************************************\
|Routine: parse_command
|Callby: command set_param
|Purpose: Parses user input against a list of valid keywords. Returns:
|             -1 if no token was found,
|             -2 if ambiguity existed,
|             -3 if a bad token was found, or
|             n if keyword[n] was matched (zero-based).
|Arguments:
|    prompt is the user prompt, or NULL if we are parsing additional tokens
|            entered on a previous call.
|    buf is the buffer containing the user's input.
|    bufl is the length of buf.
|    length is the (returned) length of the user's buffer.
|    indx is the (returned) current position in the user buffer.
|    commands is the array of valid keywords.
|    num_commands is the number of valid commands.
|    errpart is a string used in reporting of errors.
\******************************************************************************/
Int parse_command(prompt,buf,bufl,length,indx,commands,num_commands,errpart)
Char *prompt,*buf,*commands[],*errpart;
Int bufl,*length,*indx,num_commands;
{
	register Int i,j,k,l,hits;
	Char hit[256];	/* no more than 256 commands or subcommands are allowed */

	if(!prompt)	/* if no prompt, we are parsing forward from where we left off */
	{
		l = *length;
		i = *indx;
	}
	else	/* get new response from user */
	{
		if(!(l = inquire(prompt,buf,bufl,1)))
		{
			paint(BOTROW,BOTROW,FIRSTCOL);
			return(-1);
		}
		*length = l;
		i = 0;
		/* Ugly, context specific code for S/ and S / being substitute */
		if(prompt[0] == 'C' && tolower(buf[0]) == 's' && !isalpha(buf[1]) && l >= 5)
		{
			buf[0] = ' ';
			*indx = 0;
			return 27;
		}
	}
	for(;i < l;i++)
		if(!isspace(buf[i]))
		{
			memset(hit,1,num_commands);
			for(j = 0;j < 4;j++)	/* examine four columns in command array */
			{
				for(k = 0;k < num_commands;k++)
					if(commands[k][j] != (Char)tolower(buf[i]))
						hit[k] = 0;
				for(hits = k = 0;k < num_commands;k++)
					hits += hit[k];
				if(!hits)
					goto bad_command;
				if(hits == 1)
				{
					while((i < l) && !isspace(buf[i]))
						i++;
					*indx = i;
					for(k = 0;k < num_commands;k++)
						if(hit[k])
							return(k);
				}
				i++;
				if(!buf[i] || isspace(buf[i]))
					goto ambiguous_command;
			}
		}
	paint(BOTROW,BOTROW,FIRSTCOL);
	return(-1);	/* no nonblank characters in command, ignore */
ambiguous_command:
	strcpy(buf,"Ambiguous ");
	strcat(buf,errpart);
	strcat(buf,". It matches ");
	for(i = 0;i < num_commands;i++)
		if(hit[i])
		{
			if(--hits > 1)
			{
				strcat(buf,"'");
				strcat(buf,commands[i]);
				strcat(buf,"', ");
			}
			else if(hits == 1)
			{
				strcat(buf,"'");
				strcat(buf,commands[i]);
				strcat(buf,"' ");
			}
			else
			{
				strcat(buf,"and '");
				strcat(buf,commands[i]);
				strcat(buf,"'.");
			}
		}
	slip_message(buf);
	wait_message();
	paint(BOTROW,BOTROW,FIRSTCOL);
	return(-2);
bad_command:
	strcpy(buf,"Unrecognized ");
	strcat(buf,errpart);
	strcat(buf,". Try 'help.'");
	slip_message(buf);
	wait_message();
	paint(BOTROW,BOTROW,FIRSTCOL);
	return(-3);
}

