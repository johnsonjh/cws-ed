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
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "ctyp_def.h"
#include "rec.h"
#include "window.h"
#include "ed_def.h"
#include "buffer.h"
#include "buf_def.h"
#include "key_def.h"
#include "mxch_def.h"

static Char *ARGV;	/* save argv[0] so we can locate the help file */
static Char multiple;	/* flags that -m is in effect */

extern Char *version();
extern Char *filename();

/******************************************************************************\
|Routine: main
|Callby: not called
|Purpose: Implements command line interface.
|Arguments:
|    standard unix args:
|       argv[1] is the name of the terminal configuration file.
|       argv[2] is the name of the user's setup file.
|       argv[3] is the number of lines on the terminal screen.
|       argv[4] is the number of columns on the terminal screen.
|       argv[5] is either the first file name or one or more of:
|          -u, which indicates that the position reports should be updated continuously, or
|          -k, which indicates that a backup file should be created when the user EXITs, or
|          -b, which indicates that the files are to be read in as binary files, or
|          -h, which indicates that the files are to be read in as binary files, and only hex displayed, or
|          -z, which indicates that ED is to run a configuration and then exit, or
|          -r[<file>], which indicates that we are recovering edits interactively, or
|          -t[<file>], which indicates that we are text processing, or
|          -f<file>, which means that <file> is a list of file names to edit, or
|          -i<line>, which means position the cursor to line number <line>, or
|          -1, which means to not prompt for additional files (1 pass), or
|          -m, which means to use multiple-file mode, or
|          -n<server>, which calls up the news using <server> as server, or
|          -s<string>, where what follows -s is the search buffer, or
|          -p<string>, where what follows -p is the paste buffer, or
|          -l<string>, where what follows -l is the line buffer, or
|          -w<string>, where what follows -w is the word buffer, or
|          -c<string>, where what follows -c is the char buffer, or
|          -v, reports ED version number and licensing information.
\******************************************************************************/
Int main(argc,argv)
Int argc;
Char **argv;
{
	Char recover,process,diredit,binary,news;
	Char *buffer,editfile[512],recfile[512],bookmark[512];
	Char **p,*q,*r,*s,*filebuf;
	Int i,bookpos,bookbyt,bufferlen;

	for(p = argv + 1;(q = *p);p++)
		if(!strcmp(q,"-v"))
		{
			printf("\n\
This is ED version ");
			printf(version());
			printf(" Copyright (C) 1993 Charles Sandmann.\n\
ED comes with ABSOLUTELY NO WARRANTY and is free software distributed\n\
in accordance with the terms of the GNU General Public License Version 2.\n\
\n\
Please use and distribute this program freely, but the source code is covered\n\
by a license that restricts what you may do with it.\n\
\n\
You should have received a file called COPYING with this copy of ED,\n\
which explains the terms of this license in detail. If not, write to:\n\
\n\
      Free Software Foundation\n\
      675 Mass Ave\n\
      Cambridge, MA 02139   USA\n\
\n\
ED was written by Rush Record and is maintained by Charles Sandmann.\n\
Please report all bugs (save the traceback!), enhancement requests, and\n\
requests for the source code to:\n\
\n\
      Charles Sandmann (email: sandmann@clio.rice.edu)\n\
      527 Asheboro\n\
      Katy, TX 77450   USA\n");
			exit(0);
		}
	if(argc < 5)
	{
#ifndef GNUDOS
		printf("\n\
Usage: ED terminal-file startup-file term-rows term-cols [file...]\n\
\n\
If you've never used ED before, you should do the following:\n\
\n\
1) Determine the name of the directory that holds the ED program. For\n\
   the purposes of this description, we will assume '/usr/local/bin'.\n\
2) Select one of the terminal files from the directory where the ED\n\
   program resides. If your terminal acts like a VT100, for example,\n\
   use the /usr/local/bin/vt100.ed file for your terminal-file.\n\
3) Copy the ed.setup file from /usr/local/bin to your home directory.\n\
   Use the name of the new file as startup-file in the command line.\n\
4) Determine how many rows and columns your terminal (or emulator) has. Use\n\
   the number of rows where term-rows appears and similarly for the columns.\n\
\n\
To start the editor enter:\n\
\n\
/usr/local/bin/ED /usr/local/bin/vt100.ed ~/ed.setup 24 80 file.name\n\
\n\
To get license and version information enter:     /usr/local/bin/ED -v\n\
Good luck!\n\
");
#else
		printf("\n\
Usage: ED terminal-file startup-file term-rows term-cols [file...]\n\
\n\
If you've never used ED before, you should do the following:\n\
\n\
1) Determine the name of the directory that holds the ED program. For\n\
   the purposes of this description, we will assume 'C:\\EDC'.\n\
2) Set up a DOSKEY macro or .BAT file to implement the command using\n\
   PC.ED as the terminal-file and PC.SET as the startup file. The\n\
   term-rows and term-cols can be specified as zero (use current screen\n\
   size) or as values to try and set the size. If you run on a network,\n\
   only the file PC.SET needs to local and writeable.\n\
\n\
DOSKEY example:\n\
   DOSKEY ED=C:\\EDC\\EDC C:\\EDC\\PC.ED C:\\EDC\\PC.SET 50 80 -k -u $*\n\
\n\
BAT file example (contents of ED.BAT):\n\
   C:\\EDC\\EDC C:\\EDC\\PC.ED C:\\EDC\\PC.SET 0 0 -k -u %%1 %%2 %%3 %%4 %%5 %%6 %%7 %%8 %%9\n\
\n\
To get license and version information enter:     C:\\EDC\\EDC -v\n\
Good luck!\n\
");
#endif
		exit(0);
	}
	find_infinity();	/* find the largest Char value, as far as strcmp is concerned */
	recover = process = 0;
	ARGV = argv[0];
	init_terminal(argv[1],argv[2],argv[3],argv[4]);	/* initialize the interface to the terminal */
	if(argc >= 6)
		for(p = argv + 5;(q = *p);p++)
			if(!strcmp(q,"-z"))
				cfg(argv[1]);	/* never returns */
/* count the args that will need to be quoted */
	for(i = 0,p = argv + 5;(q = *p++);)	/* get total size required */
	{
		if(q[0] != '"' && q[strlen(q) - 1] != '"' && strchr(q,' '))	/* this arg will require quoting, go through the motions to figure the size */
		{
			bookmark[0] = '"';
			for(r = bookmark + 1,i = strlen(q);i--;)
			{
				if(*q == '"')
					*r++ = '\\';
				*r++ = *q++;
			}
			*r++ = '"';
			*r++ = '\0';
			i += strlen(bookmark);
		}
		else
			i += strlen(q) + 1;
	}
/* package the args into a string */
	bufferlen = i + 2;
	if(bufferlen < 1024)
		bufferlen = 1024;
	buffer = (Char *)imalloc(bufferlen);
	for(r = buffer,p = argv + 5;(q = *p++);)
	{
		if(strchr(q,' '))	/* this arg requires quoting */
		{
			bookmark[0] = '"';
			for(s = bookmark + 1,i = strlen(q);i--;)
			{
				if(*q == '"')
					*s++ = '\\';
				*s++ = *q++;
			}
			*s++ = '"';
			*s++ = '\0';
			memcpy(r,bookmark,(i = strlen(bookmark)));
		}
		else
			memcpy(r,q,i = strlen(q));
		r += i;
		*r++ = ' ';
	}
	if(r != buffer)
		r--;
	*r = '\0';
	load_inquire("File",buffer);
/* point to first non-essential argument */
	binary = 0;
	while(parse_filename(buffer,bufferlen,editfile,&recover,recfile,&process,&multiple,&binary,&news))
	{
		CURWINDOW = -1;	/* this is looked at in load_file() to see whether file size may be available */
		                /* edit() sets it to nonnegative */
		set_processmode(process);
		strip_quotes(editfile);
		envir_subs(editfile);
		if(!recover)
			journal_init(editfile);
		BASE = (rec_ptr)imalloc(sizeof(rec_node));
		BASE->length = 0;
		BASE->data = NULL;
#ifndef NO_NEWS
		if(news)
		{
			set_wait_main(0);	/* prevent "Press any key to continue" messages */
			BASE->next = BASE->prev = BASE;
			NETRC = (rec_ptr)imalloc(sizeof(rec_node));
			NETRC->length = 0;
			NETRC->data = NULL;
			NETRC->next = NETRC->prev = NETRC;
			if(!load_news(editfile,BASE,NETRC,bookmark,sizeof(bookmark),news))	/* editfile has the server name, bookmark gets window title */
			{
				toss_data(NETRC);
				continue;
			}
			strcpy(editfile,bookmark);
			bookmark[0] = '\0';
			WINDOW[0].filebuf = NULL;
			WINDOW[0].diredit = 0;
			WINDOW[0].binary = 0;
			WINDOW[0].news = 1;	/* this signifies a top-level news window */
		}
		else
#endif
		{
			set_wait_main(0);	/* prevent "Press any key to continue" messages */
			if(load_file(editfile,BASE,bookmark,&bookpos,&bookbyt,&diredit,binary,&filebuf) == -2)
				continue;
			WINDOW[0].filebuf = filebuf;
			WINDOW[0].diredit = diredit;
			WINDOW[0].binary = binary;
			WINDOW[0].news = 0;
		}
		if(host_in_name(editfile) && diredit)
			dir_store(0,editfile,BASE);
		if((i = strlen(bookmark)))
		{
			WINDOW[0].bookmark = (Char *)imalloc(++i);
			strcpy(WINDOW[0].bookmark,bookmark);
			edit_init_pos(bookpos);
			edit_init_byt(bookbyt);
		}
		set_wait_main(1);	/* enable "Press any key to continue" messages */
		edit(editfile,&multiple,buffer,bufferlen);
		if(WINDOW[0].news)
			toss_data(NETRC);
#ifndef GNUDOS
		for(i = 0;i < NWINDOWS;i++)	/* command() has already done window zero */
			if(WINDOW[i].bookmark)
				update_bookmark(i);
#endif
		for(i = 0;i < NWINDOWS;i++)	/* toss all memory */
		{
			toss_data(WINDOW[i].base);
			ifree(WINDOW[i].filename);
			if(WINDOW[i].filebuf)
				unmap_file(WINDOW[i].filebuf);
			if(WINDOW[i].bookmark)
			{
				ifree(WINDOW[i].bookmark);
				WINDOW[i].bookmark = NULL;
			}
		}
		if(!recover)
			journal_fini();
		winsub_clean();	/* clean up detached subprocess (VMS only) */
		if(!process)
			recover = 0;
		binary = 0;
/* clean up any remote FTP window data */
		for(i = 0;i < NWINDOWS;i++)
			dir_destroy(i);
		NWINDOWS = 0;
		marge(1,NROW);
#ifndef NO_FTP
		ftp_close();
#endif
#ifndef NO_NEWS
		if(news)
		{
			news_close();
			next();
		}
		news = 0;
#endif
	}
	vtend();
	cleanup(0);
	return(0);
}

/******************************************************************************\
|Routine: exefile
|Callby: help
|Purpose: Returns the name of the ed executable, which yields the directory
|         that the help file ought to be in.
|Arguments:
|    none
\******************************************************************************/
Char *exefile()
{
	return(ARGV);
}

/******************************************************************************\
|Routine: cancel_multiple
|Callby: command
|Purpose: Ends multiple mode when they issue ABORT command.
|Arguments:
|    none
\******************************************************************************/
void cancel_multiple()
{
	multiple = 0;
}

#ifndef GNUDOS
/******************************************************************************\
|Routine: update_bookmark
|Callby: command wincom
|Purpose: Updates a bookmark file for a window.
|Arguments:
|    none
\******************************************************************************/
Int update_bookmark(i)
Int i;
{
	FILE *fp;
	Char buf[512];
	Int record;
	rec_ptr r;
	
	if(WINDOW[i].bookmark)
	{
#ifdef VMS
		if(!(fp = fopen(WINDOW[i].bookmark,"w","ctx=rec","rfm=var","rat=cr","shr=nil","fop=mxv")))
#else
		if(!(fp = fopen(WINDOW[i].bookmark,"w")))
#endif
		{
			sprintf(buf,"Unable to update bookmark file '%s'.",WINDOW[i].bookmark);
			slip_message(buf);
			wait_message();
			return(0);
		}
		for(record = 1,r = WINDOW[i].base->next;r != WINDOW[i].currec;record++,r = r->next);
		fprintf(fp,"%d,%d\n",record,WINDOW[i].curbyt);
		fclose(fp);
	}
	return(1);
}
#endif

