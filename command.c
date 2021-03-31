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
#include <time.h>
#include <string.h>

#include "unistd.h"
#include "ctyp_dec.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "buffer.h"
#include "buf_dec.h"
#include "cmd_enum.h"
#include "cmd_def.h"
#include "keyvals.h"

extern Char *filename();
extern Char *insstr();
#ifndef NO_FTP
extern Char *host_filename();
#endif

#ifdef VMS
typedef struct desc_str
{
	unsigned short length,class;
	Char *string;
} desc_node;
#endif

/******************************************************************************\
|Routine: parse_string
|Callby: command
|Purpose: Parses a token from a command, skipping the command itself.
|Arguments:
|    buf - the command buffer.
|    l - the length of the command buffer.
\******************************************************************************/
Char *parse_string(buf,l)
register Char *buf;
Int l;
{
	register Char *end;
	
	end = buf + l;
	while(buf != end && !isspace(*buf))	/* skip nonblank characters (they are part of the command) */
		buf++;
	if(buf != end)
		while(buf != end && isspace(*buf))	/* skip blank characters */
			buf++;
	return(buf);
}

/******************************************************************************\
|Routine: parse_decimal
|Callby: command
|Purpose: Parses a decimal number value from a command.
|Arguments:
|    buf - the command buffer.
|    l - the length of the command buffer.
\******************************************************************************/
Int parse_decimal(buf,l)
Char *buf;
Int l;
{
	Char *p;
	Int i;
	
	p = parse_string(buf,l);
	if(!strlen(p))
		return(0);	/* 0 is the default */
	if(my_sscanf(p,"%d",&i) == 1)
		return(i);
	bell();
	return(0);
}

/******************************************************************************\
|Routine: command
|Callby: edit
|Purpose: Implements the command interface. Returns 1 when they EXIT, QUIT or
|         ABORT. Returns -1 when they EXIT when posting. Returns -2 when they
|         QUIT or ABORT when posting.
|Arguments:
|    grepdata is an array of GREP'ed buffers.
|    greptitle is an array of titles of GREP'ed buffers.
\******************************************************************************/
Int command(grepdata,greptitle)
rec_ptr *grepdata;
Char **greptitle;
{
	Int i,j,l,choice,count,save_curwindow;
	Char buf[512],*a,*b,*c,title[512],fname[512],bookbuf[512],fname2[512];
	time_t t;
	rec_node r,*base,*p,*q;
	buf_node multibuf;	/* buffer for insertion of repeated chars */
	rec_node multirec;
#ifdef VMS
	desc_node desc;
#endif

	switch(choice = parse_command("Command",buf,sizeof(buf),&l,&i,commands,NUM_COMMANDS,"command"))
	{
		case COM_EXIT:
			unselect();
			save_window();	/* so bookmark code has up-to-date WINDOW[[i].currec */
			if(WINDOW[0].diredit)
				return(1);
/* if a posting window is present, return special value */
			for(i = 0;i < NWINDOWS;i++)
				if(WINDOW[i].news == 4)
					return(-1);
#ifndef NO_NEWS
			if(WINDOW[0].news)	/* rewrite .newsrc */
			{
				if(newsrc_save())
					return(1);
				return(0);
			}
#endif
			if(insstr(buf," -n") || insstr(buf,"-n "))
				set_output_nobak();
			if((i = output_file(0,0)) >= 0)
			{
				marge(1,NROW);
				FIRSTCOL = 1;
				move(NROW,1);
				next();
				reverse();
				strcpy(fname,WINDOW[0].filename);
#ifdef VMS
				if((a = strchr(fname,';')))
					*a = '\0';
#endif
				sprintf(buf,"%s, %d records.",WINDOW[0].filename,i);
				putz(buf);
				normal();
				putout();
				return(1);
			}
			break;
		case COM_QUIT:
			save_window();	/* so bookmark code has up-to-date WINDOW[[i].currec */
/* if a posting window is present, return special value */
			for(i = 0;i < NWINDOWS;i++)
				if(WINDOW[i].news == 4)
					return(-2);
			return(1);
		case COM_HELP:
			help(buf + i);
			break;
		case COM_SET:
			if((choice = parse_command(NULL,buf,sizeof(buf),&l,&i,params,NUM_PARAMS,"parameter")) < -1)
				return(0);
			else if(choice == -1)
				choice = parse_command("Set what",buf,sizeof(buf),&l,&i,params,NUM_PARAMS,"parameter");
			if(choice >= 0 && choice < NUM_PARAMS - NUM_NOTSETTABLE)	/* ignore SET KEYS, POSI, MODI */
				set_param(choice,buf,sizeof(buf),l,i);
			break;
		case COM_SHOW:
			if((choice = parse_command(NULL,buf,sizeof(buf),&l,&i,params,NUM_PARAMS,"parameter")) < -1)
				return(0);
			if(choice >= -1 && choice < NUM_PARAMS - 1)	/* ignore SHOW KEYS */
				show_param(choice);
			break;
		case COM_STORE:
			if((choice = parse_command(NULL,buf,sizeof(buf),&l,&i,params,NUM_PARAMS,"parameter")) < -1)
				return(0);
			store_param(choice);
			break;
		case COM_RESTORE:
			if((choice = parse_command(NULL,buf,sizeof(buf),&l,&i,params,NUM_PARAMS,"parameter")) < -1)
				return(0);
			restore_par(choice);
			if(choice == 0 || choice == -1)
				ref_display();
			break;
		case COM_INCLUDE:
			if(WINDOW[CURWINDOW].diredit)
			{
				bell();
				slip_message("You can't INCLUDE a file in a directory listing.");
				wait_message();
				break;
			}
			if(strlen(a = parse_string(buf + i,l - i)))	/* parse include file name */
				strcpy(fname,a);
			else
			{
				if((l = inquire("File to include",buf,sizeof(buf),1)) > 0)
					strcpy(fname,buf);
				else
					fname[0] = '\0';
			}
			if(fname[0])
			{
				strip_quotes(fname);
				include_file(fname,WINDOW[CURWINDOW].binary);	/* calls load_file(), which calls envir_subs() */
			}
			break;
		case COM_WRITE:
			if(WINDOW[CURWINDOW].diredit)
			{
				bell();
				slip_message("You cannot WRITE a directory, sorry.");
				wait_message();
				break;
			}
			if(strlen(a = parse_string(buf + i,l - i)))	/* parse write-to file name */
			{
				strcpy(fname,a);
				strip_quotes(fname);
				envir_subs(fname);
				output_file(fname,CURWINDOW);
			}
			else
			{
				unselect();
				output_file(0,CURWINDOW);
			}
			break;
		case COM_CALCULATE:
			if(!calculate(CURREC->data,buf))
				abort_key();
			buffer_empty(&KILLBUF);
			r.next = &r;
			r.prev = &r;
			r.length = strlen(buf);
			r.data = (Char *)imalloc(r.length+1);
			strcpy(r.data,buf);
			buffer_append(&KILLBUF,&r,0,r.length);
			KILLBUF.nrecs = KILLBUF.direction = 1;
			ifree(r.data);
			break;
		case COM_DATE:
			t = time(0);
			a = (Char *)ctime(&t);
			sprintf(buf,"%2.2s-%3.3s-%4.4s",a+8,a+4,a+20);
			buffer_empty(&KILLBUF);
			r.next = &r;
			r.prev = &r;
			r.length = strlen(buf);
			r.data = (Char *)imalloc(r.length+1);
			strcpy(r.data,buf);
			buffer_append(&KILLBUF,&r,0,r.length);
			KILLBUF.nrecs = KILLBUF.direction = 1;
			ifree(r.data);
			break;
		case COM_TIME:
			t = time(0);
			a = (Char *)ctime(&t);
			sprintf(buf,"%8.8s",a+11);
			buffer_empty(&KILLBUF);
			r.next = &r;
			r.prev = &r;
			r.length = strlen(buf);
			r.data = (Char *)imalloc(r.length+1);
			strcpy(r.data,buf);
			buffer_append(&KILLBUF,&r,0,r.length);
			KILLBUF.nrecs = KILLBUF.direction = 1;
			ifree(r.data);
			break;
		case COM_ABORT:
			save_window();	/* so bookmark code has up-to-date WINDOW[[i].currec */
			file_list_abort();	/* make file_list_next() fail */
			abort_parsing();	/* make parse_filename() fail */
			parse_skip(0);	/* insure special no-prompt parse_filename state is reset */
			cancel_multiple();	/* insure not in multiple-mode */
/* if a posting window is present, return special value */
			for(i = 0;i < NWINDOWS;i++)
				if(WINDOW[i].news == 4)
					return(-2);
			return(1);
		case COM_GREP:
		case COM_PERG:
			if(!strlen(a = parse_string(buf + i,l - i)))	/* parse search string */
			{
				if(!(l = inquire("String to search for",buf,sizeof(buf),1)))
					break;
				a = buf;
			}
			if(NWINDOWS >= MAX_WINDOWS)
			{
				slip_message("You must close a window first.");
				wait_message();
				break;
			}
			unselect();
			paint(BOTROW,BOTROW,FIRSTCOL);
			base = (rec_ptr)imalloc(sizeof(rec_node));
			base->next = base->prev = base;
			base->length = 0;
			base->data = NULL;
			do_grep(WINDOW[CURWINDOW].base,base,a,(choice == (int)COM_GREP),GREPMODE);
			save_curwindow = CURWINDOW;
			i = dir_depth(WINDOW[CURWINDOW].filename);
			if(grepdata[i])
			{
				toss_data(grepdata[i]);
				grepdata[i] = NULL;
			}
			if(greptitle[i])
			{
				ifree(greptitle[i]);
				greptitle[i] = NULL;
			}
			if(WINDOW[CURWINDOW].diredit)
			{
				sprintf(title,"%s - %s window for string '%s'",WINDOW[CURWINDOW].filename,(choice == (int)COM_GREP)? "GREP" : "PERG",a);
				grepdata[i] = (rec_ptr)imalloc(sizeof(rec_node));
				grepdata[i]->length = 0;
				grepdata[i]->data = NULL;
				copy_records(base,grepdata[i],0,0);
				greptitle[i] = (Char *)imalloc(strlen(title) + 1);
				strcpy(greptitle[i],title);
			}
			else
				sprintf(title,"%s window for string '%s'",(choice == (int)COM_GREP)? "GREP" : "PERG",a);
			save_window();
			insert_window(title,base,0,0);	/* note, this routine may call set_window in order to scroll reduced windows */
			set_non_subp();
			WINDOW[CURWINDOW].diredit = WINDOW[save_curwindow].diredit;
			return(0);
		case COM_BYTE:
			if(WINDOW[CURWINDOW].diredit)
			{
				bell();
				slip_message("You cannot insert data into a directory, sorry.");
				wait_message();
				break;
			}
			i = parse_decimal(buf + i,l - i);
			if(i > 255)
				i = 255;
			if(i < 0)
				i = 0;
			copy_only(0);
			multibuf.nrecs = multibuf.direction = 1;
			multibuf.first = &multirec;	/* initialize repeated-char buffer */
			multibuf.last = &multirec;
			multirec.next = (rec_ptr)&multibuf;
			multirec.prev = (rec_ptr)&multibuf;
			multirec.length = 1;
			multirec.data = buf;
			multirec.data[0] = i;
			insert(&multibuf);
			break;
		case COM_FILE:
			if(strlen(a = parse_string(buf + i,l - i)))	/* parse any additional info following the FILE command */
				strcpy(bookbuf,a);
			else
				strcpy(bookbuf,"name");
			bookbuf[0] = tolower(bookbuf[0]);
			buffer_empty(&KILLBUF);
			r.next = &r;
			r.prev = &r;
			if(WINDOW[CURWINDOW].diredit)
			{
				if(CURREC == BASE)
				{
					abort_key();
					bell();
					break;
				}
				switch(bookbuf[0])
				{
					case 'd':
						strcpy(fname,WINDOW[CURWINDOW].filename);
						break;
					case 'n':
						strcpy(fname,CURREC->data + filename_offset());
						break;
					default:
						strcpy(fname,WINDOW[CURWINDOW].filename);
						strcat(fname,CURREC->data + filename_offset());
				}
			}
			else
			{
				strcpy(fname,WINDOW[CURWINDOW].filename);
#ifndef NO_FTP
				if((i = host_in_name(fname)))
				{
					a = host_filename(i,fname,fname2);
					if(*((b = fname2 + strlen(fname2) - 1)) == '/')
						*b = '\0';
					if(ftp_unixy(ftp_system(fname2)))
					{
						for(b = fname2 + strlen(fname2);--b != fname2;)
							if(*b == '/' || *b == '\\')
								break;
					}
					else
					{
						for(b = fname2 + strlen(fname2);--b != fname2;)
							if(*b == ']' || *b == '>' || *b == ':')
								break;
					}
					switch(bookbuf[0])
					{
						case 'd':
							*++b = '\0';
							strcpy(fname,fname2);
							break;
						case 'n':
							strcpy(fname,++b);
							break;
						default:
							strcpy(fname,fname2);
							break;
					}
				}
				else
#endif
				{
					switch(bookbuf[0])
					{
						case 'd':
							*(filename(fname)) = '\0';
							break;
						case 'n':
							strcpy(bookbuf,filename(fname));
							strcpy(fname,bookbuf);
							break;
						default:
							break;
					}
				}
			}
			if((r.length = strlen(fname)))
			{
				r.data = (Char *)imalloc(r.length + 1);
				strcpy(r.data,fname);
				buffer_append(&KILLBUF,&r,0,r.length);
				KILLBUF.nrecs = KILLBUF.direction = 1;
				ifree(r.data);
			}
			break;
		case COM_TRIM:
			if(WINDOW[CURWINDOW].diredit)
			{
				bell();
				slip_message("You cannot TRIM a directory listing.");
				wait_message();
				break;
			}
			trim();
			break;
		case COM_CD:
			if(strlen(a = parse_string(buf + i,l - i)))	/* parse directory name */
				strcpy(fname,a);
			else
			{
				if((l = inquire("Directory to move to",buf,sizeof(buf),1)) > 0)
					strcpy(fname,buf);
				else
					fname[0] = '\0';
			}
			if(fname[0])
			{
				strip_quotes(fname);
				envir_subs(fname);
				if(chdir(fname) < 0)
				{
					sprintf(title,"Couldn't change to directory %s.",fname);
					slip_message(title);
					wait_message();
				}
			}
			break;
		case COM_PWD:
#ifdef NeXT
			if(!getwd(fname))
#else
			if(!getcwd(fname,sizeof(fname)))
#endif
			{
				slip_message("I am unable to get the CWD for some reason.");
				wait_message();
			}
			else
			{
				sprintf(title,"Current directory: %s",fname);
				slip_message(title);
				wait_message();
			}
			break;
		case COM_BIGGER:
			if(!(i = parse_decimal(buf + i,l - i)))
				i = 1;
			if(i > 0)
				bigger_window(i);
			else
				smaller_window(-i);
			break;
		case COM_SMALLER:
			if(!(i = parse_decimal(buf + i,l - i)))
				i = 1;
			if(i > 0)
				smaller_window(i);
			else
				bigger_window(-i);
			break;
		case COM_LOAD:
			if(strlen(a = parse_string(buf + i,l - i)))
				strcpy(fname,a);
			else
			{
				if((l = inquire("File to load key definitions from",buf,sizeof(buf),1)) > 0)
					strcpy(fname,buf);
				else
					fname[0] = '\0';
			}
			if(fname[0])
			{
				strip_quotes(fname);
				envir_subs(fname);
				load_defs(fname);
			}
			break;
		case COM_UNLOAD:
			if(strlen(a = parse_string(buf + i,l - i)))
				strcpy(fname,a);
			else
			{
				if((l = inquire("File to unload key definitions to",buf,sizeof(buf),1)) > 0)
					strcpy(fname,buf);
				else
					fname[0] = '\0';
			}
			if(fname[0])
			{
				strip_quotes(fname);
				envir_subs(fname);
				unload_defs(fname);
			}
			break;
		case COM_SORT:
			if(tolower(buf[0]) == 's')
				if(tolower(buf[1]) == 'o')
					if(tolower(buf[2]) == 'r')
						if(tolower(buf[3]) == 't')
						{
							j = 0;
							if(strlen(a = parse_string(buf + i,l - i)))
								if(tolower(*a) == 'r')
									j = 1;
							sort_recs(j);
							break;
						}
			slip_message("You cannot abbreviate the SORT command; it's too dangerous.");
			wait_message();
			break;
#ifndef GNUDOS
		case COM_BOOKMARK:
			strcpy(bookbuf,WINDOW[CURWINDOW].filename);
			strcat(bookbuf,"-EDbookmark");
			WINDOW[CURWINDOW].bookmark = (Char *)imalloc(strlen(bookbuf) + 1);
			strcpy(WINDOW[CURWINDOW].bookmark,bookbuf);
			save_window();
			update_bookmark(CURWINDOW);
			break;
#endif
		case COM_DELETE:
			if(!WINDOW[CURWINDOW].diredit)
			{
				slip_message("The DELETE command only applies to directory-mode windows.");
				wait_message();
				break;
			}
/* insure some deletions exist */
			p = q = WINDOW[CURWINDOW].base;
			for(p = p->next;p != q;p = p->next)
				if(p->recflags & 2)
					break;
			if(p == q)
			{
				slip_message("There are no files marked for deletion.");
				wait_message();
			}
			else
			{
/* require the first four characters of 'delete' */
				if(tolower(buf[0]) == 'd')
					if(tolower(buf[1]) == 'e')
						if(tolower(buf[2]) == 'l')
							if(tolower(buf[3]) == 'e')
							{
/* scan the buffer and delete them */
								p = q = WINDOW[CURWINDOW].base;
								for(count = 0,p = p->next;p != q;p = p->next)
									if(p->recflags & 2)
										if(strcmp(p->data + filename_offset(),"..") && strcmp(p->data + filename_offset(),"[-]"))
										{
											strcpy(fname,WINDOW[CURWINDOW].filename);
											strcat(fname,p->data + filename_offset());
#ifndef NO_FTP
											if(host_in_name(fname))
											{
												sprintf(buf,"DELE %s",p->data + filename_offset());
												if(!ftp_command(buf))
													goto delete_error;
											}
											else
#endif
											{
#ifdef VMS
												desc.string = fname;
												desc.length = strlen(fname);
												desc.class = 0x010e;
												if(!(LIB$DELETE_FILE(&desc) & 1))
#else
												if(remove(fname))
#endif
												{
delete_error:
													sprintf(fname,"Unable to delete file %s, aborting.",p->data + filename_offset());
													slip_message(fname);
													wait_message();
													break;
												}
											}
											count++;
										}
/* re-create the buffer, which should be different now */
								if(count)
								{
									base = (rec_ptr)imalloc(sizeof(rec_node));
									base->length = 0;
									base->data = NULL;
									base->prev = base->next = base;
									if(read_in_diredit(fname,WINDOW[CURWINDOW].filename,base))
									{
										toss_data(BASE);
										BASE = base;
										CURROW = TOPROW;
										CURCOL = 1;
										FIRSTCOL = 1;
										WANTCOL = 1;
										DIRECTION = 1;
										SELREC = 0;
										TOPREC = BOTREC = CURREC = BASE->next;
										for(i = TOPROW;i < BOTROW;i++)	/* establish BOTREC */
										{
											if(BOTREC == BASE)
												break;
											BOTREC = BOTREC->next;
										}
										if(i < BOTROW)
											BOTREC = 0;
										ifree(WINDOW[CURWINDOW].filename);
										WINDOW[CURWINDOW].filename = (Char *)imalloc(strlen(fname) + 1);	/* this line cannot be merged with the following one... */
										strcpy(WINDOW[CURWINDOW].filename,fname);	/* ...without generating a pointer mismatch error under VMS cc */
										WINDOW[CURWINDOW].base = BASE;
										dir_store(CURWINDOW,WINDOW[CURWINDOW].filename,BASE);
										save_window();
										ref_window(CURWINDOW);
									}
									else
									{
										ifree(base);
										bell();
										slip_message("I was unable to re-read the directory. Deleted files may persist.");
										wait_message();
									}
								}
								break;
							}
				slip_message("You cannot abbreviate the DELETE command; it's too dangerous.");
				wait_message();
			}
			break;
		case COM_SUBSTITUTE:
			if(WINDOW[CURWINDOW].diredit)
			{
				bell();
				slip_message("You cannot make substitutions in a directory listing.");
				wait_message();
				break;
			}
			if(strlen(a = parse_string(buf + i,l - i)))
				strcpy(fname,a);
			else
			{
				if((l = inquire("Substitution command",buf,sizeof(buf),1)) > 0)
					strcpy(fname,buf);
				else
					fname[0] = '\0';
			}
			if((i = fname[0]))	/* c will be the delimiter */
			{
				if(!(a = strchr(fname + 1,i)))
					goto subst_err;
				if(!(b = strchr(a + 1,i)))
					goto subst_err;
				*a++ = '\0';
				*b = '\0';
				if(!strlen(fname + 1))
					goto subst_err;
				if(SEARCH_FLAGS[((int)SMODE_BEGINNING) >> 1] == 'b')	/* if they are in beginning mode, work backwards from the end of file */
				{
					emulate_key((Schar)KEY_ENDOFFILE,1);
					emulate_key((Schar)KEY_FIND,1);
					for(c = fname + 1;(i = *c++);)
						emulate_key((Schar)i,1);
					emulate_key((Schar)KEY_BACKWARD,1);
				}
				else	/* if they are in end mode, work forward from the top of file */
				{
					emulate_key((Schar)KEY_TOPOFFILE,1);
					emulate_key((Schar)KEY_FIND,1);
					for(c = fname + 1;(i = *c++);)
						emulate_key((Schar)i,1);
					emulate_key((Schar)KEY_FORWARD,1);
				}
				emulate_key((Schar)KEY_SELECT,1);
				for(c = a;(i = *c++);)
					emulate_key((Schar)i,1);
				emulate_key((Schar)KEY_KILL,1);
				emulate_key((Schar)KEY_SUBSTITUTE,0x7fffffff);
			}
			break;
subst_err:
			slip_message("The SUBST command requires a string of the form: /string1/string2/");
			slip_message("You can replace '/' with any character not appearing in string1 or string2.");
			wait_message();
			break;
		default:	/* error parsing command */
			return(0);
	}
	paint(BOTROW,BOTROW,FIRSTCOL);
	return(0);
}

