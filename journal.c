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
#include <stdlib.h>
#include <string.h>

#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "buffer.h"
#include "buf_dec.h"

static FILE *fp = NULL;
static Int pos = 0;
static Char buf[128];	/* room for ten keys */
static Char jfilename[512];	/* saved journal file name, used to reopen the journal after a recovery */
struct inhibit_flags
{
	unsigned int search:1;
	unsigned int paste:1;
	unsigned int line:1;
	unsigned int word:1;
	unsigned int chr:1;
};

/******************************************************************************\
|Routine: journal
|Callby: init_term
|Purpose: Stores characters in the journal file.
|Arguments:
|    c is the character to store.
|    repeat is the repeat count to associate with the character.
\******************************************************************************/
void journal(c,repeat)
Char c;
Int repeat;
{
	if(!fp)
		return;
	sprintf(buf + pos,"%x %x ",(Uchar)c,repeat);
	pos += strlen(buf + pos);
	if(pos > sizeof(buf) - 14)
	{
		buf[pos] = '\0';
		fprintf(fp,"%s\n",buf);
		fflush(fp);
		pos = 0;
	}
}

/******************************************************************************\
|Routine: journal_fini
|Callby: main
|Purpose: Closes the journal file, and (on VMS systems) purges it to one version.
|Arguments:
|    none
\******************************************************************************/
void journal_fini()
{
#ifdef VMS
	Char journal_file[512];
#endif

	if(!fp)
		return;
	buf[pos] = '\0';
	fprintf(fp,"%s\n",buf);
	fflush(fp);
	fclose(fp);
	fp = NULL;
#ifdef VMS
	strcpy(journal_file,JOURNAL_FILE);
	strcat(journal_file,";-1");
	while(!remove(journal_file));
#endif
}

/******************************************************************************\
|Routine: journal_buffer
|Callby: journal
|Purpose: Stores a buffer in the journal file.
|Arguments:
|    prefix is the prefix for the string.
|    buffer is the buffer pointer.
\******************************************************************************/
void journal_buffer(prefix,buffer)
Char *prefix;
buf_ptr buffer;
{
	static Char *zip = (Char *)"\0";
	
	if(!fp)
		return;
	fprintf(fp,"%s=%s\n",prefix,(
		buffer->first == (rec_ptr)&buffer->first ||
		!buffer->first->length ||
		!buffer->first->data)? zip : buffer->first->data);
}

/******************************************************************************\
|Routine: journal_init
|Callby: main
|Purpose: Creates a journal file.
|Arguments:
|    file is the name the journal file should have.
\******************************************************************************/
void journal_init(file)
Char *file;
{
	Char buf[512];
	
	strcpy(buf,JOURNAL_FILE);
	strip_quotes(buf);
	envir_subs(buf);
	if(!(fp = fopen(buf,"w")))
		return;
	fprintf(fp,"%s\n",file);
	journal_buffer("search",&SEARCHBUF);
	journal_buffer("paste",&KILLBUF);
	journal_buffer("line",&LINEBUF);
	journal_buffer("word",&WORDBUF);
	journal_buffer("char",&CHARBUF);
	pos = 0;
}

/******************************************************************************\
|Routine: unjournal
|Callby: init_term
|Purpose: Retrieves a character and repeat count from the journal. Closes the
|         journal file when it runs out.
|Arguments:
|    c is the returned character.
|    repeat is the associated repeat count.
\******************************************************************************/
void unjournal(c,repeat)
Char *c;
Int *repeat;
{
	Int ic,ir,n;
	Char buf[512];

	n = my_fscanf(fp,"%x%x",&ic,&ir);
	if(n < 0)
	{
		pos = 0;
		fclose(fp);
		strcpy(buf,jfilename);
		strip_quotes(buf);
		envir_subs(buf);
		if(!(fp = fopen(buf,"a")))
		{
			printf("Unable to append to the journal file %s.\r\n",buf);
			cleanup(-1);
		}
		*repeat = 0;
		return;
	}
	else if(n != 2)
	{
		printf("There is an error in the journal file %s.\r\n",JOURNAL_FILE);
		cleanup(-1);
	}
	*((Schar *)c) = ic;
	*repeat = ir;
}

/******************************************************************************\
|Routine: unjournal_buffer
|Callby: journal
|Purpose: Recalls a buffer from the journal file.
|Arguments:
|    prefix is the prefix for the string.
|    buffer is the buffer pointer.
|    inhibit is the set of buffer-loading inhibition flags.
\******************************************************************************/
void unjournal_buffer(prefix,buffer,inhibit)
Char *prefix;
buf_ptr buffer;
Int inhibit;
{
	Int l;
	Char string[512];

	if(!fgets(string,sizeof(string),fp))
	{
		printf("Unexpected end-of-file or error while reading the %s buffer from the journal file.\r\n",prefix);
		cleanup(-1);
	}
	l = strlen(prefix);
	if(strncmp(string,prefix,l))
	{
		printf("Was expecting %s=<string> in journal file, found %s.\r\n",prefix,string);
		cleanup(-1);
	}
	if(!inhibit)
	{
		*strchr(string,'\n') = '\0';
		string_to_buf(string + l + 1,buffer);
	}
}

/******************************************************************************\
|Routine: unjournal_init
|Callby: parse_fnm
|Purpose: Opens an existing journal file and retrieves the name of the file
|         that was being edited.
|Arguments:
|    file is the returned name of the file being edited.
\******************************************************************************/
void unjournal_init(jfile,file,inhibit)
Char *jfile,*file;
struct inhibit_flags *inhibit;
{
	Char buf[512],lfile[512];
	
	if(fp)
		fclose(fp);	/* if text processing on multiple files, close journal before reopening it */
	strcpy(lfile,jfile);
	strip_quotes(lfile);
	envir_subs(lfile);
	if(!(fp = fopen(lfile,"r")))
	{
		printf("Unable to open the journal file %s.\r\n",lfile);
		cleanup(-1);
	}
	if(!fgets(buf,sizeof(buf),fp))
	{
		printf("Unable to read the edited file name from the journal file %s.\r\n",lfile);
		cleanup(-1);
	}
	*strchr(buf,'\n') = '\0';
	strcpy(file,buf);
	unjournal_buffer("search",&SEARCHBUF,inhibit->search);
	unjournal_buffer("paste",&KILLBUF,inhibit->paste);
	unjournal_buffer("line",&LINEBUF,inhibit->line);
	unjournal_buffer("word",&WORDBUF,inhibit->word);
	unjournal_buffer("char",&CHARBUF,inhibit->chr);
	pos = 0;
	set_recover();	/* tell get_next_key to get from journal file */
	strcpy(jfilename,lfile);
}

