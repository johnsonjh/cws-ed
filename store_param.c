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

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "cmd_enum.h"
#include "cmd_dec.h"

extern Char *filename();

#define NUM_FILEENTRIES 19	/* number of a=b lines (other than ndefined=) */

/******************************************************************************\
|Routine: fixup_delim
|Callby: store_param
|Purpose: Converts control characters to printable form for the setup file.
|Arguments:
|    buf is the buffer that gets the data in printable form.
|    l is the length of the original data.
|    d is the buffer that has the original data, including control characters.
\******************************************************************************/
void fixup_delim(buf,l,d)
register Char *buf,*d;
register Int l;
{
	register Int i,j;

	for(i = j = 0;i < l;i++)
	{
		if(d[i] < 33)
		{
			buf[j++] = 0x1d;
			sprintf(buf + j,"%03d",((int)d[i]) & 0xff);
			j += strlen(buf + j);
		}
		else
			buf[j++] = d[i];
	}
	buf[j] = '\0';
}

/******************************************************************************\
|Routine: store_param
|Callby: command
|Purpose: Stores editor settings in the user's startup file.
|Arguments:
|    choice is the number of the setting to store, or -1 to store all settings.
\******************************************************************************/
void store_param(choice)
Int choice;
{
	FILE *ifp,*ofp;
	Int i,ntab;
	Char buf[512],tmpfile[512],*p;
	Char found[NUM_FILEENTRIES];
	static Char *keyword[NUM_FILEENTRIES] =
	{
		"shell=",
		"wrap=",
		"section=",
		"word=",
		"page=",
		"paragraph=",
		"search=",
		"stable=",
		"close=",
		"parens=",
		"defext=",
		"auto=",
		"cfriendly=",
		"boxmode=",
		"wraparound=",
		"casechange=",
		"overstrike=",
		"grepmode=",
		"wildcard="
	};
	static Char *deflt[NUM_FILEENTRIES] = 
	{
		"/bin/csh",
		"80",
		"18",
		" \t,.+=-*/[]{}()\n\v\f\r",
		"\f",
		"\r\r",
		"gbnnn",
		"~/categories.st",
		"y",
		"(){}[]",
		"c h f",
		"n",
		"n",
		"n",
		"n",
		"Opposite",
		"n",
		"verbose",
		"?"
	};

	if(!(ifp = fopen(KEYFILE,"r")))
	{
		slip_message("Unable to open the original setup file.");
		wait_message();
		return;
	}
	strcpy(tmpfile,KEYFILE);
	*(filename(tmpfile)) = '\0';
	strcat(tmpfile,"_key_.tmp");
#ifdef VMS
	if(!(ofp = fopen(tmpfile,"w","ctx=rec","rfm=var","rat=cr","shr=nil")))
#else
	if(!(ofp = fopen(tmpfile,"w")))
#endif
	{
		fclose(ifp);
		slip_message("Unable to write to the setup file.");
		wait_message();
		return;
	}
/* transfer the journal file name */
	fgets(buf,sizeof(buf),ifp);
	fputs(buf,ofp);
/* clear all found fields */
	memset(found,0,sizeof(found));
/* read the setup file and process changes */
	while(fgets(buf,sizeof(buf),ifp))
	{
		if((p = strchr(buf,'\n')))
			*p = '\0';
		if((p = strstr(buf,keyword[0])))	/* the shell name */
		{
			found[0] = 1;
			p += strlen(keyword[0]);
			if(choice == (int)PAR_SHELL || choice < 0)
				strcpy(p,USERSHELL);
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,"tabs=")))	/* tabs */
		{
			p += strlen("tabs=");
			my_sscanf(p,"%d",&ntab);
			fprintf(ofp,"%s\n",buf);
			my_fscanf(ifp,"curtab=%d\n",&i);
			if(choice == (int)PAR_TABS || choice < 0)
				i = CUR_TAB_SETUP;
			fprintf(ofp,"curtab=%d\n",i);
			for(i = 0;i < ntab << 1;i++)
			{
				fgets(buf,sizeof(buf),ifp);
				fputs(buf,ofp);
			}
		}
		else if((p = strstr(buf,keyword[1])))	/* word wrap margin */
		{
			found[1] = 1;
			p += strlen(keyword[1]);
			my_sscanf(p,"%d",&i);
			if(choice == (int)PAR_WRAP || choice < 0)
				i = WRAP_MARGIN;
			fprintf(ofp,"wrap=%d\n",i);
		}
		else if((p = strstr(buf,keyword[2])))	/* section lines */
		{
			found[2] = 1;
			p += strlen(keyword[2]);
			my_sscanf(p,"%d",&i);
			if(choice == (int)PAR_SECTION || choice < 0)
				i = SECTION_LINES;
			fprintf(ofp,"section=%d\n",i);
		}
		else if((p = strstr(buf,keyword[3])))	/* word delimiters */
		{
			found[3] = 1;
			p += strlen(keyword[3]);
			if(choice == (int)PAR_WORD || choice < 0)
				fixup_delim(p,NWORD_DELIMITERS,WORD_DELIMITERS);
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,keyword[4])))	/* page break */
		{
			found[4] = 1;
			p += strlen(keyword[4]);
			if(choice == (int)PAR_PAGE || choice < 0)
				fixup_delim(p,PAGE_BREAK_LENGTH,PAGE_BREAK);
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,keyword[5])))	/* word paragraph delimiter */
		{
			found[5] = 1;
			p += strlen(keyword[5]);
			if(choice == (int)PAR_PARAGRAPH || choice < 0)
				fixup_delim(p,PARAGRAPH_BREAK_LENGTH,PARAGRAPH_BREAK);
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,keyword[6])))	/* search mode */
		{
			found[6] = 1;
			p += strlen(keyword[6]);
			if(choice == (int)PAR_SEARCH || choice < 0)
				memcpy(p,SEARCH_FLAGS,NUM_SMODES >> 1);
			*(p + (NUM_SMODES >> 1)) = '\0';
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,keyword[7])))	/* search table */
		{
			found[7] = 1;
			p += strlen(keyword[7]);
			if(choice == (int)PAR_STABLE || choice < 0)
				strcpy(p,SEARCH_TABLE);
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,keyword[8])))	/* paren matching status */
		{
			found[8] = 1;
			p += strlen(keyword[8]);
			if(choice == (int)PAR_CLOSE_PARENS || choice < 0)
				*p = (CLOSE_PARENS)? 'y' : 'n';
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,keyword[9])))	/* paren pairs */
		{
			found[9] = 1;
			p += strlen(keyword[9]);
			if(choice == (int)PAR_PARENS || choice < 0)
				strcpy(p,PAREN_STRING);
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,keyword[10])))	/* the defext string */
		{
			found[10] = 1;
			p += strlen(keyword[10]);
			if(choice == (int)PAR_DEFAULT || choice < 0)
				strcpy(p,DEFAULT_EXT);
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,keyword[11])))	/* auto tab status */
		{
			found[11] = 1;
			p += strlen(keyword[11]);
			if(choice == (int)PAR_AUTO_TABS || choice < 0)
				*p = (TAB_AUTO)? 'y' : 'n';
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,keyword[12])))	/* cfriendly status */
		{
			found[12] = 1;
			p += strlen(keyword[12]);
			if(choice == (int)PAR_CFRIENDLY || choice < 0)
				*p = (CFRIENDLY)? 'y' : 'n';
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,keyword[13])))	/* box mode */
		{
			found[13] = 1;
			p += strlen(keyword[13]);
			if(choice == (int)PAR_BOXCUT || choice < 0)
				*p = (BOXCUT)? 'y' : 'n';
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,keyword[14])))	/* wraparound flag */
		{
			found[14] = 1;
			p += strlen(keyword[14]);
			if(choice == (int)PAR_AUTOWRAP || choice < 0)
				*p = (AUTOWRAP)? 'y' : 'n';
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,keyword[15])))	/* case change */
		{
			found[15] = 1;
			p += strlen(keyword[15]);
			if(choice == (int)PAR_CASE || choice < 0)
				strcpy(p,caseprompts[CASECHANGE]);
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,keyword[16])))	/* overstrike flag */
		{
			found[16] = 1;
			p += strlen(keyword[16]);
			if(choice == (int)PAR_OVERSTRIKE || choice < 0)
				*p = (OVERSTRIKE)? 'y' : 'n';
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,keyword[17])))	/* grep mode */
		{
			found[17] = 1;
			p += strlen(keyword[17]);
			if(choice == (int)PAR_GREPMODE || choice < 0)
				strcpy(p,(GREPMODE)? "silent" : "verbose");
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,keyword[18])))	/* grep mode */
		{
			found[18] = 1;
			p += strlen(keyword[18]);
			if(choice == (int)PAR_WILDCARD || choice < 0)
			{
				*p++ = WILDCARD;
				*p = '\0';
			}
			fprintf(ofp,"%s\n",buf);
		}
		else if((p = strstr(buf,"ndefined=")))	/* key definitions, must be last thing in file */
		{
/* check for unmentioned things and make setup file up-to-date */
			for(i = 0;i < NUM_FILEENTRIES;i++)
				if(!found[i])
					fprintf(ofp,"%s%s\n",keyword[i],deflt[i]);
			if(choice != (int)PAR_KEYS && choice >= 0)
			{
				fprintf(ofp,"%s\n",buf);
				while(fgets(buf,sizeof(buf),ifp))
					fputs(buf,ofp);
			}
			else
				store_keys(ofp);
			break;	/* keys must be last thing in file */
		}
	}
/* clean up */
	fclose(ofp);
	fclose(ifp);
	if(rename(tmpfile,KEYFILE))
	{
		slip_message("Unable to replace the original setup file.");
		wait_message();
		return;
	}
}

