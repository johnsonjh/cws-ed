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

#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "buffer.h"
#include "buf_dec.h"
#include "ctyp_dec.h"

extern Char *filename();

static Char edityet = 0;
static Char abortbuffer = 0;
static Char specialskip = 0;	/* flags that no prompting must be done when files run out */
static struct inhibit_flags
{
	unsigned int search:1;
	unsigned int paste:1;
	unsigned int line:1;
	unsigned int word:1;
	unsigned int chr:1;
} inhibit;

/******************************************************************************\
|Routine: insstr
|Callby: parse_long_option parse_option
|Purpose: Does what strstr() does, only case-insensitively.
|Arguments:
|    buf is the command line (which may contain upcase chars).
|    str is the string we're looking for (which must be all lowercase).
\******************************************************************************/
Char *insstr(buf,str)
Char *buf,*str;
{
	Char *p,*q,*r,c,d;
	
	for(p = buf,c = *str;(d = tolower(*p++));)
		if(c == d)
		{
			for(q = p,r = str + 1;(d = *r++);)
				if(((Char)tolower(*q++)) != d)
					break;
			if(!d)
				return(--p);
		}
	return(NULL);
}
	
/******************************************************************************\
|Routine: parse_option
|Callby: parse_options
|Purpose: Parses a simple option of the form -<character>.
|         Returns a cleaned-up buffer, and 1 if -char is present, else 0.
|Arguments:
|    buf is the command line.
|    chr is the <character> we're looking for.
|    exact indicates (if nonzero) that the case is not to be ignored.
\******************************************************************************/
Int parse_option(buf,chr,exact)
Char *buf;
Int chr,exact;
{
	static Char str[5] = {' ','-',' ',' ',0};
	static Char *localbuf = NULL;
	static Int localbufsiz;
	
	Char *from,*to;
	Int i,l;
	
	if((l = strlen(buf) + 3) > localbufsiz)
	{
		if(localbuf)
			ifree(localbuf);
		localbuf = (Char *)imalloc((localbufsiz = l));
	}
	str[2] = chr;
	strcpy(localbuf," ");
	strcat(localbuf,buf);
	strcat(localbuf," ");
	if(exact)
	{
		if(!(to = strstr(localbuf,str)))
			return(0);
	}
	else
		if(!(to = insstr(localbuf,str)))
			return(0);
	for(from = to + 3;(*to++ = *from++););
	for(from = localbuf + 1,to = localbuf;(*to++ = *from++););	/* eliminate the preceeding space */
	if((i = strlen(localbuf)) > 1)
		localbuf[i - 1] = '\0';	/* trim trailing space */
	strcpy(buf,localbuf);
	return(1);
}

/******************************************************************************\
|Routine: parse_long_option
|Callby: parse_options
|Purpose: Parses an option of the form -<character><string>.
|         Returns a cleaned-up buffer, and 1 if option is present, else 0.
|Arguments:
|    buf is the command line.
|    chr is the <character> we're looking for.
|    val is the returned <string>.
\******************************************************************************/
Int parse_long_option(buf,chr,val)
Char *buf;
Int chr;
Char *val;
{
	static Char str[4] = {' ','-',' ',0};
	static Char *localbuf = NULL;
	static Int localbufsiz;
	
	Char *from,*to,*p,c;
	Int i,l;
	
	if((l = strlen(buf) + 3) > localbufsiz)
	{
		if(localbuf)
			ifree(localbuf);
		localbuf = (Char *)imalloc((localbufsiz = l));
	}
	str[2] = chr;
	strcpy(localbuf," ");
	strcat(localbuf,buf);
	strcat(localbuf," ");
	if(!(to = insstr(localbuf,str)))
		return(0);
	for(p = val,from = to + 3;!isspace(c = *from++);*p++ = c);	/* copy the trailing value (possibly NULL) */
	*p = '\0';
	for(from--;(*to++ = *from++););
	for(from = localbuf + 1,to = localbuf;(*to++ = *from++););	/* eliminate the preceeding space */
	if((i = strlen(localbuf)) > 1)
		localbuf[i - 1] = '\0';	/* trim trailing space */
	strcpy(buf,localbuf);
	return(1);
}

/******************************************************************************\
|Routine: parse_options
|Callby: parse_filename
|Purpose: Parses options out of the command line. Sets appropriate flags.
|         Returns a cleaned-up buffer.
|Arguments:
|    file is the command line.
|    recover is the recovery flag, set if -r is present.
|    recfile is the journal file name (defaulted if absent).
|    process is the text-processing flag, set if -t is present.
|    multiple is the multiple-file-mode flag, set if -m is present.
|    binary is the binary-mode flag, set to 1 if -b is present, 2 if -h.
|    news is the flag that -n was present.
\******************************************************************************/
void parse_options(file,recover,recfile,process,multiple,binary,news)
Char *file,*recover,*recfile,*process,*multiple,*binary,*news;
{
	Char *local;
	Char *p;
	Char buf[1024],buf2[1024];
	Int l;
	
/* make a local copy with blank padding on both ends */
	strcpy((local = (Char *)imalloc((l = strlen(file)) + 3)) + 1,file);
	local[0] = ' ';
	local[l + 1] = ' ';
	local[l + 2] = '\0';
/* check for -n, which overrides everything */
#ifndef NO_NEWS
	if(parse_option(local,'n',1))
	{
		*news = 1;
		goto trim_servername;
	}
	else if(parse_option(local,'N',1))
	{
		*news = 2;
trim_servername:
		l = strlen(local);
		local[--l] = '\0';			/* Trim trailing blank */
		p = strrchr(local,' ') + 1;	/* Will always find leading blank */
		while((*file++ = *p++));	/* Copy final parameter, stripping options */
		return;
	}
#endif
	*news = 0;
/* turn -t into -r and set process flag */
	if((p = insstr(local," -t ")))
	{
		*process = 1;
		*(p + 2) = 'r';
	}
	else if((p = insstr(local," -t")))	/* NOTE! " -t" is not the same as " -t " */
	{
		*process = 1;
		*(p + 2) = 'r';
	}
	if(parse_option(local,'u',0))
		REPORTSTATUS = 1;
	if(parse_option(local,'1',0))
		specialskip = 2;
	if(parse_long_option(local,'k',buf))
		set_output_bak((strlen(buf))? buf : NULL);
	if(parse_option(local,'m',0))
		*multiple = 1;
	if(parse_option(local,'b',0))
		*binary = 1;
	if(parse_option(local,'h',0))	/* -h takes precedence if both -b and -h are present */
		*binary = 2;
	if(parse_long_option(local,'f',buf))
	{
		strcpy(buf2,"-f");
		strcat(buf2,buf);
		file_list_init(buf2);
	}
	if(parse_long_option(local,'r',buf))
	{
		*recover = 1;
		strcpy(recfile,(strlen(buf))? buf : (Char *)JOURNAL_FILE);
	}
	if(parse_long_option(local,'t',buf))
	{
		*process = 1;
		strcpy(recfile,(strlen(buf))? buf : (Char *)JOURNAL_FILE);
	}
	if(parse_long_option(local,'i',buf))
	{
		my_sscanf(buf,"%d",&l);
		if(l < 1)
			l = 1;
		edit_init_pos(l);
	}
	if(parse_long_option(local,'s',buf))
	{
		string_to_buf(buf,&SEARCHBUF);
		inhibit.search = 1;
	}
	if(parse_long_option(local,'p',buf))
	{
		string_to_buf(buf,&KILLBUF);
		inhibit.paste = 1;
	}
	if(parse_long_option(local,'l',buf))
	{
		string_to_buf(buf,&LINEBUF);
		inhibit.line = 1;
	}
	if(parse_long_option(local,'w',buf))
	{
		string_to_buf(buf,&WORDBUF);
		inhibit.word = 1;
	}
	if(parse_long_option(local,'c',buf))
	{
		string_to_buf(buf,&CHARBUF);
		inhibit.chr = 1;
	}
	strcpy(file,local + 1);
	if((l = strlen(file)))
		file[l - 1] = '\0';
}

/******************************************************************************\
|Routine: parse_skip
|Callby: command edit
|Purpose: Flags that parse_fnm() must not prompt when files run out.
|Arguments:
|    flag is 0 or 1.
\******************************************************************************/
void parse_skip(flag)
Int flag;
{
	specialskip = flag;
}

/******************************************************************************\
|Routine: strip_quotes
|Callby: command init_term journal load_file main match_search parse_fnm wincom
|Purpose: Strips quotes from quoted file names (in place).
|Arguments:
|    file is the file name.
\******************************************************************************/
void strip_quotes(file)
Char *file;
{
	register Int i;
	register Char *from,*to;
	
	from = to = file;
	if(*from == '"')
	{
		i = strlen(from) - 1;
		if(*(from + i) == '"' && *(from + i - 1) != '\\')
		{
			from = to + 1;
			while(--i)
				if(*from == '\\' && *(from + 1) == '"')
				{
					*to++ = '"';
					from += 2;
					i--;	/* for the \ character */
				}
				else
					*to++ = *from++;
			*to = '\0';
		}
	}
}

/******************************************************************************\
|Routine: parse_filename
|Callby: edit main parse_fnm
|Purpose: Handles parsing of command line options and file names
|Arguments:
|    command is the command line buffer. this is returned in a modified form.
|    commlen is the length of the command buffer.
|    editfile is the returned name of the file to be edited.
|    recover is a flag indicating that we are in recovery mode, so edits come from the journal file.
|    recfile is the returned name of the journal file from which edits are to be recovered.
|    process is a flag indicating that we are in text processing mode, and must not prompt user.
|    multiple is a flag indicating that -m was specified on the command line.
|    binary is a flag indicating that -b or -h was specified on the command line.
|    news is a flag indicating that -n was specified.
\******************************************************************************/
Int parse_filename(command,commlen,editfile,recover,recfile,process,multiple,binary,news)
Char *command,*editfile,*recover,*recfile,*process,*multiple,*binary,*news;
Int commlen;
{
	Char nextfile[512];

/* parse command line options */
redo:
/* handle ABORT command */
	if(abortbuffer)	/* note, file_list_next will fail because command calls file_list_abort */
	{
		abortbuffer = 0;	/* make it a one-shot deal */
		command[0] = '\0';	/* terminate parsing from buffer */
	}
	parse_options(command,recover,recfile,process,multiple,binary,news);
	if(*news)
	{
		strcpy(editfile,command);
		abortbuffer = 1;
		edityet = 1;
		return(1);
	}
/* see if more files are coming from list or wildcard */
filelist:
	if(file_list_next(nextfile))
	{
		parse_options(nextfile,recover,recfile,process,multiple,binary,news);	/* we allow options in file lists, alas */
		strcpy(editfile,nextfile);
		if(*recover)
			unjournal_init(recfile,nextfile,&inhibit);	/* dump the file name that is in the journal file */
		if(!edityet)
		{
			edityet = 1;
			if(*process)
			{
				putpurge();
				putoff();
			}
		}
		return(1);
	}
/* see if more tokens are waiting in user buffer */
	if(get_token(command,nextfile))
	{
		strip_quotes(nextfile);
		file_list_init(nextfile);
		goto filelist;
	}
/* nothing on command line, prompt if appropriate */
	if(*process)
		cleanup(0);
	if(*recover)	/* they specified -r but no file, use file specified in journal file */
	{
		unjournal_init(recfile,editfile,&inhibit);	/* use the file name that is in the journal file */
		return(1);
	}
	if(specialskip && edityet)
	{
		if(specialskip == 2)
		{
			move(NROW,1);
			next();				/* Scroll up file name+lines */
		}
		specialskip = 0;
		return(0);
	}
/* prompt them for a file name */
	*process = *recover = *multiple = *binary = 0;
	inhibit.search = inhibit.paste = inhibit.line = inhibit.word = inhibit.chr = 0;
	marge(1,NROW);
	move(NROW,1);
	if(edityet)
		next();
	else
		edityet = 1;
	BOTROW = NROW;
	if(inquire("File",command,commlen,1))
	{
#ifndef VMS
		next();
#endif
		goto redo;
	}
	ers_end();
	return(0);
}

/******************************************************************************\
|Routine: abort_parsing
|Callby: command
|Purpose: Aborts the taking of multiple filenames from the command line.
|Arguments:
|    none
\******************************************************************************/
void abort_parsing()
{
	abortbuffer = 1;
}

