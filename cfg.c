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
#include "ed_dec.h"	/* global data */
#include "buffer.h"
#include "buf_dec.h"	/* kill buffers */
#include "cmd_enum.h"
#include "key_dec.h"
#include "handy.h"	/* for definition of max() */

extern Char *filename();

/******************************************************************************\
|Routine: cfg
|Callby: main
|Purpose: Handles creation of new terminal setup files.
|Arguments:
|    termfile is the name of the ED executable, from which the proper directory
|             for .ed files is parsed.
\******************************************************************************/
void cfg(termfile)
Char *termfile;
{
	Int nname,nextra,i,j,k,ln[128];
	Long context;
	Schar c;
	Char *p,*q;
	Schar buf[128][16];
	Char sbuf[128],qbuf[128],dbuf[128],tbuf[128],oldfile[128],newfile[128],curterm[128],finistring[128];
	FILE *fp,*ifp;
	
	BOTROW = NROW;
	marge(1,NROW);
	move(BOTROW,1);
	ers_screen();
	next();
/* parse out the ED executable directory name */
	strcpy(dbuf,termfile);
/* parse out the current terminal name */
	p = filename(dbuf);
	strcpy(curterm,p);
	*p = '\0';
	if((q = (Char *)strchr(curterm,'.')))
		*q = '\0';
	else
		strcpy(curterm,"vt100");
/* list all the existing .ed files */
	strcpy(sbuf,dbuf);
	strcat(sbuf,"*.ed");
	if(!(context = find_files(sbuf)))
	{
		putz("I can't find any .ed files to copy commands from.");
		next();
		putout();
		cleanup(0);
	}
	putz("The following configuration files already exist:");
	next();
	next();
	while(next_file(context,sbuf))
	{
		p = filename(sbuf);
		strcpy(qbuf,p);
		*(strstr(qbuf,".ed")) = '\0';
		putz(qbuf);
		next();
	}
	next();
/* get the user's choice */
	sprintf(qbuf,"Which one do you want to copy terminal control strings from?[%s] ",curterm);
	if(!inquire(qbuf,sbuf,sizeof(sbuf),1))
		strcpy(sbuf,curterm);
	strcpy(oldfile,dbuf);
	strcat(oldfile,sbuf);
	strcat(oldfile,".ed");
	if(!(ifp = fopen(oldfile,"r")))
	{
		next();
		putz("I can't open that .ed file for some reason.");
		next();
		putout();
		perror("cfg");
		cr();
		putout();
		cleanup(0);
	}
/* get the new cfg file name */
redo:
	next();
	if(!inquire("What name do you want for the new configuration file?[abort program] ",qbuf,sizeof(qbuf),1))
	{
		next();
		putout();
		cleanup(0);
	}
	next();
	strcpy(newfile,dbuf);
	strcat(newfile,qbuf);
	strcat(newfile,".ed");
	while(next_file(context,tbuf))
		if(!strcmp(tbuf,newfile))
		{
			if(!inquire("That file already exists, do you want to supersede it?[no] ",tbuf,sizeof(tbuf),1))
			{
				while(next_file(context,tbuf));
				goto redo;
			}
			if(!strchr("YyTt1",tbuf[0]))
			{
				while(next_file(context,tbuf));
				goto redo;
			}
			break;
		}
	find_files_end(context);
/* create the new cfg file */
	if(!(fp = fopen(newfile,"w")))
	{
		next();
		sprintf(tbuf,"You seem to be unable to create '%s'.",newfile);
		putz(tbuf);
		next();
		putz("Perhaps you need to be the super-user to do this.");
		cr();
		putout();
		cleanup(0);
	}
/* transfer terminal commands from selected file to new file */
	for(i = 0;i < 19;i++)
	{
		fgets(sbuf,sizeof(sbuf),ifp);
		if(!i)	/* initialize the terminal */
			putz(sbuf);
		else if(i == 1)	/* finalize the terminal */
			strcpy(finistring,sbuf);
		fputs(sbuf,fp);
	}
	fclose(ifp);
/* find out how many additional definable keys they want */
redo2:
	if(!(inquire("How many additional keys (besides the keypad) do you want to define?[0] ",sbuf,sizeof(sbuf),1)))
		strcpy(sbuf,"0");
	next();
	if(my_sscanf(sbuf,"%d",&nextra) != 1)
		goto redo2;
	fprintf(fp,"%d\n",nname = 44 + nextra);
	putz("If you make a mistake after you've started entering a key,");
	next();
	putz("press control-C and hit RETURN.");
	next();
	next();
	putz("If you press control-C and hit RETURN before you've pressed any other keys,");
	next();
	putz("the program will abort.");
	next();
	next();
/* get the key strings */
	for(i = 0;i < nname;i++)
	{
redo3:
		putz("Press the '");
		putz(keyname[127 - i]);
		putz("' key, and then hit RETURN: ");
		putout();
		for(j = 0;j < sizeof(buf[i]);j++)
		{
			ttyget(&c);
			if(c == 13)
				break;
			buf[i][j] = c;
		}
		next();
		if(!(ln[i] = j))
		{
			putz("That key didn't generate any characters, use some other key.");
			next();
			goto redo3;
		}
		else if(ln[i] == 1 && buf[i][0] == 3)
		{
			putz("Program aborted.");
			cr();
			putz(finistring);
			putout();
			cleanup(0);
		}
		else if(buf[i][j - 1] == 3)
			goto redo3;
	}
/* put the strings in the cfg file */
	for(i = 0;i < nname;i++)
	{
		for(k = 0;k < ln[i];k++)
		{
			if(((buf[i][k] >= 0 && buf[i][k] < ' ') || buf[i][k] < 0) && buf[i][k] != 27)
				fprintf(fp,"%c%03d",29,buf[i][k] & 0xff);
			else
				fprintf(fp,"%c",buf[i][k]);
		}
		fprintf(fp,"\n");
	}
	fclose(fp);
	next();
	putz("The new configuration file (");
	putz(newfile);
	putz(") is complete.");
	cr();
	putz(finistring);
	putout();
	cleanup(0);
}

