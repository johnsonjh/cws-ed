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

#include "memory.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "cmd_enum.h"
#include "cmd_dec.h"

/******************************************************************************\
|Routine: user_file_err
|Callby: restore_par
|Purpose: Reports errors in the terminal configuration file.
|Arguments:
|    none
\******************************************************************************/
void user_file_err()
{
	Char buf[512];
	
	sprintf(buf,"There is an error in the user configuration file '%s'.",KEYFILE);
	slip_message(buf);
	wait_message();
	vtend();
	cleanup(-1);
}

/******************************************************************************\
|Routine: restore_par
|Callby: command init_term
|Purpose: Restore editor settings from the user's startup file.
|Arguments:
|    choice is the indication of which parameter to restore. If -1, restore
|            all parameters.
\******************************************************************************/
void restore_par(choice)
Int choice;
{
	static Char *defsearch = "gbnnnc";
	
	FILE *fp;
	Int i,j,l,nkeys,keyval,keylength,missing;
	Int *nextrpt,nkey,nrpt;
	Schar *nextkey;
	Char *p;
	static Char buf[256];
	Schar defbuf[4096];
	Int defrpt[4096];

	if(!(fp = fopen(KEYFILE,"r")))
	{
		printf("Unable to open the setup file.\r\n");
		cleanup(-1);
	}
	if(choice < 0)	/* if restoring everything, set all defaults */
	{
		strcpy(USERSHELL,"/bin/csh");
		TAB_SETUPS = 1;
		CUR_TAB_SETUP = 0;
		TAB_NAME[0] = (Char *)imalloc(strlen("NORMAL") + 1);
		strcpy(TAB_NAME[0],"NORMAL");
		TAB_STRING[0] = (Char *)imalloc(162);
		strcpy(TAB_STRING[0],"t       t       t       t       t       t       t       t       t       t       ");
		strcat(TAB_STRING[0],"t       t       t       t       t       t       t       t       t       t       t");
		WRAP_MARGIN = 80;
		SECTION_LINES = 18;
		NWORD_DELIMITERS = 19;
		strcpy(WORD_DELIMITERS," \t,.+=-*/[]{}()\n\v\f\r");
		PAGE_BREAK_LENGTH = 1;
		strcpy(PAGE_BREAK,"\f");
		PARAGRAPH_BREAK_LENGTH = 2;
		strcpy(PARAGRAPH_BREAK,"\r\r");
		strcpy(SEARCH_FLAGS,defsearch);
		strcpy(SEARCH_TABLE,"~/categories.st");
		CLOSE_PARENS = 1;
		PAREN_STRING_LENGTH = strlen("(){}[]");
		strcpy(PAREN_STRING,"(){}[]");
		DEFAULT_EXT_LENGTH = strlen("c h f");
		strcpy(DEFAULT_EXT,"c h f");
		TAB_AUTO = 0;
		CFRIENDLY = 0;
		BOXCUT = 0;
		AUTOWRAP = 0;
		CASECHANGE = 2;	/* default is OPPOSITE */
		OVERSTRIKE = 0;
		GREPMODE = 0;
		WILDCARD = '?';
	}
/* scan the journal file name */
	if(!fgets(JOURNAL_FILE,sizeof(JOURNAL_FILE),fp))
		user_file_err();
	JOURNAL_FILE[strlen(JOURNAL_FILE) - 1] = '\0';
	while(fgets(buf,sizeof(buf),fp))
	{
		if((p = (Char *)strchr(buf,'\n')))
			*p = '\0';
		if(buf[0] == '#' || buf[0] == '!' || buf[0] == '\0')
			continue;
		if((p = (Char *)strstr(buf,"shell=")))	/* user shell */
		{
			p += strlen("shell=");
			if(choice == (int)PAR_SHELL || choice < 0)
				strcpy(USERSHELL,p);
		}
		else if((p = (Char *)strstr(buf,"tabs=")))	/* tab setups */
		{
			p += strlen("tabs=");
			if(my_sscanf(p,"%d",&i) != 1)
				user_file_err();
			if(choice == (int)PAR_TABS || choice < 0)
			{
				TAB_SETUPS = i;
				if(my_fscanf(fp,"curtab=%d\n",&CUR_TAB_SETUP) != 1)
					user_file_err();
				for(i = 0;i < TAB_SETUPS;i++)
				{
					if(!fgets(buf,256,fp))
						user_file_err();
					l = strlen(buf) - 1;
					buf[l] = '\0';
					TAB_NAME[i] = (Char *)imalloc(l + 1);
					strcpy(TAB_NAME[i],buf);
					if(!fgets(buf,256,fp))
						user_file_err();
					l = strlen(buf) - 1;
					buf[l] = '\0';
					TAB_STRING[i] = (Char *)imalloc(l);
					memcpy(TAB_STRING[i],buf,l);
					TAB_LENGTH[i] = l;
				}
			}
			else
				for(j = 0;j <= i << 1;j++)
					fgets(buf,256,fp);
		}
		else if((p = (Char *)strstr(buf,"wrap=")))	/* word wrap margin */
		{
			p += strlen("wrap=");
			if(choice == (int)PAR_WRAP || choice < 0)
				if(my_sscanf(p,"%d",&WRAP_MARGIN) != 1)
					user_file_err();
		}
		else if((p = (Char *)strstr(buf,"section=")))	/* section lines */
		{
			p += strlen("section=");
			if(choice == (int)PAR_SECTION || choice < 0)
				if(my_sscanf(p,"%d",&SECTION_LINES) != 1)
					user_file_err();
		}
		else if((p = (Char *)strstr(buf,"word=")))	/* word delimiters */
		{
			p += strlen("word=");
			if(choice == (int)PAR_WORD || choice < 0)
			{
				NWORD_DELIMITERS = strlen(p);
				memcpy(WORD_DELIMITERS,p,strlen(p));
				trans_term_string(WORD_DELIMITERS,&NWORD_DELIMITERS);
				init_word_table(NWORD_DELIMITERS,WORD_DELIMITERS);
			}
		}
		else if((p = (Char *)strstr(buf,"page=")))	/* page break */
		{
			p += strlen("page=");
			if(choice == (int)PAR_PAGE || choice < 0)
			{
				PAGE_BREAK_LENGTH = strlen(p);
				memcpy(PAGE_BREAK,p,PAGE_BREAK_LENGTH);
				trans_term_string(PAGE_BREAK,&PAGE_BREAK_LENGTH);
			}
		}
		else if((p = (Char *)strstr(buf,"paragraph=")))	/* word paragraph delimiter */
		{
			p += strlen("paragraph=");
			if(choice == (int)PAR_PARAGRAPH || choice < 0)
			{
				PARAGRAPH_BREAK_LENGTH = strlen(p);
				memcpy(PARAGRAPH_BREAK,p,PARAGRAPH_BREAK_LENGTH);
				trans_term_string(PARAGRAPH_BREAK,&PARAGRAPH_BREAK_LENGTH);
			}
		}
		else if((p = (Char *)strstr(buf,"search=")))	/* search mode */
		{
			p += strlen("search=");
			if((missing = (i = (NUM_SMODES >> 1)) - strlen(p)) > 0)
				memcpy(p + strlen(p),defsearch + i - missing,missing);	/* handle older search modes that have fewer categories */
			if(choice == (int)PAR_SEARCH || choice < 0)
				memcpy(SEARCH_FLAGS,p,((int)NUM_SMODES) >> 1);
		}
		else if((p = (Char *)strstr(buf,"stable=")))	/* search table */
		{
			p += strlen("stable=");
			if(choice == (int)PAR_STABLE || choice < 0)
				strcpy(SEARCH_TABLE,p);
			if(choice == (int)PAR_SEARCH || choice < 0)	/* check the search table if necessary */
				if(SEARCH_FLAGS[((int)SMODE_TABLE_DRIVEN) >> 1] == 'y')
					if(!set_search_table(SEARCH_TABLE))
						SEARCH_FLAGS[((int)SMODE_TABLE_DRIVEN) >> 1] = 'n';
		}
		else if((p = (Char *)strstr(buf,"close=")))	/* paren matching status */
		{
			p += strlen("close=");
			if(choice == (int)PAR_CLOSE_PARENS || choice < 0)
				CLOSE_PARENS = (*p == 'y' || *p == 'Y');
		}
		else if((p = (Char *)strstr(buf,"parens=")))	/* paren pairs */
		{
			p += strlen("parens=");
			if(choice == (int)PAR_PARENS || choice < 0)
			{
				PAREN_STRING_LENGTH = strlen(p);
				memcpy(PAREN_STRING,p,PAREN_STRING_LENGTH);
				trans_term_string(PAREN_STRING,&PAREN_STRING_LENGTH);
			}
		}
		else if((p = (Char *)strstr(buf,"defext=")))	/* the defext string */
		{
			p += strlen("defext=");
			if(choice == (int)PAR_DEFAULT || choice < 0)
			{
				DEFAULT_EXT_LENGTH = strlen(p);
				strcpy(DEFAULT_EXT,p);
			}
		}
		else if((p = (Char *)strstr(buf,"auto=")))	/* auto tabs status */
		{
			p += strlen("auto=");
			if(choice == (int)PAR_AUTO_TABS || choice < 0)
				TAB_AUTO = (*p == 'y' || *p == 'Y');
		}
		else if((p = (Char *)strstr(buf,"cfriendly=")))	/* cfriendly status */
		{
			p += strlen("cfriendly=");
			if(choice == (int)PAR_CFRIENDLY || choice < 0)
				CFRIENDLY = (*p == 'y' || *p == 'Y');
		}
		else if((p = (Char *)strstr(buf,"boxmode=")))	/* box mode */
		{
			p += strlen("boxmode=");
			if(choice == (int)PAR_BOXCUT || choice < 0)
				BOXCUT = (*p == 'y' || *p == 'Y');
		}
		else if((p = (Char *)strstr(buf,"wraparound=")))	/* auto-wrap */
		{
			p += strlen("wraparound=");
			if(choice == (int)PAR_AUTOWRAP || choice < 0)
				AUTOWRAP = (*p == 'y' || *p == 'Y');
		}
		else if((p = (Char *)strstr(buf,"casechange=")))	/* case change */
		{
			p += strlen("casechange=");
			if(choice == (int)PAR_CASE || choice < 0)
			{
				for(CASECHANGE = 0;CASECHANGE < NUM_CASES;CASECHANGE++)
					if(!strncmp(p,caseprompts[CASECHANGE],strlen(caseprompts[CASECHANGE])))
						break;
				if(CASECHANGE < 0 || CASECHANGE >= NUM_CASES)
					user_file_err();
			}
		}
		else if((p = (Char *)strstr(buf,"overstrike=")))	/* auto-wrap */
		{
			p += strlen("overstrike=");
			if(choice == (int)PAR_OVERSTRIKE || choice < 0)
				OVERSTRIKE = (*p == 'y' || *p == 'Y');
		}
		else if((p = (Char *)strstr(buf,"grepmode=")))	/* GREP mode */
		{
			p += strlen("grepmode=");
			if(choice == (int)PAR_GREPMODE || choice < 0)
				GREPMODE = (*p == 's' || *p == 'S');
		}
		else if((p = (Char *)strstr(buf,"wildcard=")))	/* search wildcard */
		{
			p += strlen("wildcard=");
			if(choice == (int)PAR_WILDCARD || choice < 0)
				WILDCARD = *p;
		}
		else if((p = (Char *)strstr(buf,"ndefined=")))	/* the key definitions */
		{
			p += strlen("ndefined=");
			if(choice == (int)PAR_KEYS || choice < 0)
			{
				if(my_sscanf(p,"%d",&nkeys) != 1)
					user_file_err();
				for(i = 0;i < nkeys;i++)
				{
					if(my_fscanf(fp,"%x",&keyval) != 1)
						user_file_err();
					if(keyval > 127)
						keyval -= 256;
					if(my_fscanf(fp,"%d",&keylength) != 1)
						user_file_err();
					nextkey = defbuf;
					nextrpt = defrpt;
					for(j = 0;j < keylength;j++)
					{
						if(my_fscanf(fp,"%x%Lx",&nkey,&nrpt) != 2)
							user_file_err();
						*nextkey++ = nkey;
						*nextrpt++ = nrpt;
					}
					load_key(keyval,keylength,defbuf,defrpt);
				}
			}
			break;	/* keys must be the last thing in the file */
		}
	}
/* clean up */
	fclose(fp);
}

