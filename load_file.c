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
#include <errno.h>
#include <time.h>

#ifndef NO_MMAP
#include <sys/types.h>
#include <sys/mman.h>
#endif

#include "unistd.h"
#include "memory.h"
#include "file.h"
#include "ctyp_dec.h"

#define FILENAME_OFFSET 40	/* this is the offset from the beginning of diredit records to the file name */

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* NOTE: BLOCK_SIZE cannot be larger than BUFFER_SIZE */
#define BLOCK_SIZE 512
/* NOTE: BLOCK_SIZE must be an even multiple of BINARY_RECSIZE */
#define BUFFER_SIZE 2048

#include "rec.h"
#include "window.h"
#include "ed_dec.h"

typedef struct mmap_str *mmap_ptr;
typedef struct mmap_str
{
	mmap_ptr next;
	Char *adr;
	Int len;
} mmap_node;

static mmap_ptr base = NULL;
static Char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
static Char ftpbin = 0;	/* flags a binary ftp'ed file was just loaded */
static Char foundusr[128];
static Char foundpwd[128];
static Char fileonly = 0;	/* flag to prevent interpretation of fname as dirname */
static Char forcebinary = 0;	/* flag to force binary transfer of file */
static Char maperror = 0;	/* flag that a map_file error other than ENOENT has occurred */

extern Char *extension();
extern Char *filename();
extern Char *ftp_cwd();
extern Char *last_message();
#ifndef VMS
extern char *sys_errlist[];
#endif

/******************************************************************************\
|Routine: filename_offset
|Callby: command edit
|Purpose: Returns the offset to the file name in diredit records.
|Arguments:
|    none
\******************************************************************************/
Int filename_offset()
{
	return(FILENAME_OFFSET);
}

/******************************************************************************\
|Routine: mode_string
|Callby: read_in_diredit
|Purpose: Returns a string expressing the file's permissions, as in: rwxr-xr-x.
|Arguments:
|    mode is the file mode.
\******************************************************************************/
Char *mode_string(mode)
Int mode;
{
	static char rbuf[10];
	
	memset(rbuf,'-',9);
	rbuf[9] = '\0';
	if(mode & 1)
		rbuf[8] = 'x';
	if(mode & 2)
		rbuf[7] = 'w';
	if(mode & 4)
		rbuf[6] = 'r';
	if(mode & 8)
		rbuf[5] = 'x';
	if(mode & 16)
		rbuf[4] = 'w';
	if(mode & 32)
		rbuf[3] = 'r';
	if(mode & 64)
		rbuf[2] = 'x';
	if(mode & 128)
		rbuf[1] = 'w';
	if(mode & 256)
		rbuf[0] = 'r';
	return(rbuf);
}

/******************************************************************************\
|Routine: size_string
|Callby: read_in_diredit
|Purpose: Returns a string expressing the file size in 6 bytes.
|Arguments:
|    size is the size of the file in bytes.
\******************************************************************************/
Char *size_string(size)
Int size;
{
	static char rbuf[9];
	
	if(size > 99999999)
		sprintf(rbuf,"%5dmeg",size / 1000000);
	else
		sprintf(rbuf,"%8d",size);
	return(rbuf);
}

/******************************************************************************\
|Routine: parse_vms_perms
|Callby: parse_vms_mode read_in_diredit
|Purpose: Parses expressions like RWED. Returns unix-style mode.
|Arguments:
|    p is a pointer in the string that presumably contains the permissions. This
|      string may be empty, in which case it is terminated by , or \0 or ).
|    mode is the returned mode, as in rwx.
\******************************************************************************/
void parse_vms_perms(p,mode)
Char *p,*mode;
{
	memset(mode,'-',3);
	while(1)
		switch(*p++)
		{
			case 'R':
				mode[0] = 'r';
				break;
			case 'W':
				mode[1] = 'w';
				break;
			case 'E':
				mode[2] = 'x';
				break;
			case 'D':
				break;
			default:
				return;
		}
}

/******************************************************************************\
|Routine: parse_vms_mode
|Callby: read_in_diredit
|Purpose: Parses expressions like (RWED,RE,RE,). Returns unix-style mode.
|Arguments:
|    p is a pointer in the string that presumably contains the permissions.
|    mode is the returned mode, as in rwxr-xr-w.
\******************************************************************************/
void parse_vms_mode(p,mode)
Char *p,*mode;
{
	Char *q;
	
	mode[9] = '\0';
	if((q = strchr(p,'(')))
	{
		if((p = strchr(q,',')))
		{
			parse_vms_perms(++p,mode);
			if((q = strchr(p,',')))
			{
				parse_vms_perms(++q,mode + 3);
				if((p = strchr(q,',')))
					parse_vms_perms(++p,mode + 6);
				else
					goto badmode;
			}
			else
				goto badmode;
		}
		else
			goto badmode;
		return;
	}
badmode:
	strcpy(mode,"?????????");
}

/******************************************************************************\
|Routine: fix_vms_dir
|Callby: read_in_diredit
|Purpose: Converts *.dir;1 or *.DIR;1 to [.dir] or [.DIR]. Also strips device
|         and directory from the name, and packages it in [].
|Arguments:
|    file is the input file name.
|    ret is the returned file name (same as file if .dir not present).
\******************************************************************************/
void fix_vms_dir(file,ret)
Char *file,*ret;
{
	Char buf[512],*p,*q;
	
	strcpy(buf,file);
	q = buf;
	if((p = strstr(buf,".DIR;1")))
	{
		*p = '\0';
		while(q != p && *p != ']' && *p != '>')
			p--;
		if(q != p)
			p++;
		sprintf(ret,"[.%s]",p);
	}
	else if((p = strstr(buf,".dir;1")))	/* oddball lowercase VMS system */
	{
		*p = '\0';
		while(q != p && *p != ']' && *p != '>')
			p--;
		if(q != p)
			p++;
		sprintf(ret,"[.%s]",++p);
	}
	else	/* just strip device/directory */
	{
		for(p = buf,q = buf + strlen(buf) - 1;p != q && *q != ']' && *q != '>';)
			q--;
		if(q != p)
			q++;
		strcpy(ret,q);
	}
}

/******************************************************************************\
|Routine: fake_date
|Callby: read_in_diredit
|Purpose: Makes up a fake set of information about a file when parsing fails.
|Arguments:
|    none
\******************************************************************************/
void fake_date(size,year,month,day,hour,minute)
Int *size,*year;
Char *month;
Int *day,*hour,*minute;
{
	*size = 0;
	*year = 1955;
	strcpy(month,"Feb");
	*day = 8;
	*hour = 6;
	*minute = 35;
}

/******************************************************************************\
|Routine: parse_monthyear
|Callby: read_in_diredit
|Purpose: Deals with month and year in VMS listings.
|Arguments:
|    monthyear is the string that combines the month and year.
|    size is the (possibly) returned file size in bytes.
|    year is the (possibly) returned year.
|    month is the returned month.
|    day is the (possibly) returned day.
|    ihour is the (possibly) returned hour.
|    minute is the (possibly) returned minute.
|    mode is the returned file mode.
|    p is a pointer to the listing record.
\******************************************************************************/
void parse_monthyear(monthyear,size,year,month,day,ihour,minute,mode,p)
Char *monthyear;
Int *size,*year;
Char *month;
Int *day,*ihour,*minute;
Char *mode;
Char *p;
{
	Char *q;
	
	if((q = strchr(monthyear,'-')))
	{
		*q++ = '\0';
		strcpy(month,monthyear);
		if(my_sscanf(q,"%d",year) == 1)
		{
			parse_vms_mode(p,mode);
			return;
		}
	}
	fake_date(size,year,month,day,ihour,minute);	/* use fake date info */
}

/******************************************************************************\
|Routine: force_binary
|Callby: edit
|Purpose: Flags that the next file opened will be in binary mode.
|Arguments:
|    none
\******************************************************************************/
void force_binary()
{
	forcebinary = 1;
}

/******************************************************************************\
|Routine: ftp_fileonly
|Callby: edit
|Purpose: Flags that the next file opened will skip the directory check.
|Arguments:
|    none
\******************************************************************************/
void ftp_fileonly()
{
	fileonly = 1;
}

/******************************************************************************\
|Routine: netrc_password
|Callby: ftp_open
|Purpose: Returns netrc password or NULL string.
|Arguments:
|    none
\******************************************************************************/
Char *netrc_password()
{
	return(foundpwd);
}

/******************************************************************************\
|Routine: netrc_token
|Callby: find_netrc
|Purpose: Gets a token from the .netrc file.
|Arguments:
|    fp is the file pointer of the .netrc file.
\******************************************************************************/
Char *netrc_token(fp)
FILE *fp;
{
	static Char tok[512],*p;
	Int c;
	
	while(1)
	{
		if((c = fgetc(fp)) < 0)
		{
			fclose(fp);
			return(NULL);
		}
		if(isspace(c))
			continue;
		p = tok;
		*p++ = c;
		while(1)
		{
			if((c = fgetc(fp)) < 0)
			{
				*p++ = '\0';
				return(tok);
			}
			if(isspace(c))
			{
				*p++ = '\0';
				return(tok);
			}
			*p++ = c;
		}
	}
}

/******************************************************************************\
|Routine: find_netrc
|Callby: extract_host
|Purpose: Looks up a host in the user's netrc file. Returns pointer to username,
|         or NULL if no matching host is found.
|Arguments:
|    host is the host we're looking for.
\******************************************************************************/
Char *find_netrc(host)
Char *host;
{
	FILE *fp;
	Char *p,file[512];
	struct stat statbuf;
	
	foundpwd[0] = '\0';	/* we check for netrc password when opening connection */
#ifdef VMS
	strcpy(file,"SYS$LOGIN:.NETRC;");
#else
	strcpy(file,"$HOME/.netrc");
#endif
	envir_subs(file);
	if(!stat(file,&statbuf))
	{
		if((statbuf.st_mode & 0x1ff) == 0x180)	/* require rw------- permission */
		{
			if((fp = fopen(file,"r")))	/* open the .netrc file */
			{
				while((p = netrc_token(fp)))	/* get tokens */
				{
					if(!inscmp(p,"machine"))
					{
						if(!(p = netrc_token(fp)))	/* get machine name */
							return(NULL);
						if(!inscmp(host,p))	/* a hit on the machine name */
						{
							while((p = netrc_token(fp)))
							{
								if(!inscmp(p,"password"))
								{
									if(!(p = netrc_token(fp)))
										return(NULL);
									fclose(fp);
									strcpy(foundpwd,p);
									return(foundusr);
								}
								else if(!inscmp(p,"login"))
								{
									if(!(p = netrc_token(fp)))
										return(NULL);
									strcpy(foundusr,p);
								}
								else if(!inscmp(p,"machine"))
								{
									fclose(fp);
									return(foundusr);	/* user but no password, prompt them later */
								}
							}
							return(foundusr);	/* user but no password, prompt them later */
						}
					}
				}
			}
		}
	}
	return(NULL);
}

/******************************************************************************\
|Routine: was_binary
|Callby: load_file
|Purpose: Tells caller whether the last file loaded was loaded in BINARY mode.
|Arguments:
|    none
\******************************************************************************/
Int was_binary()
{
	if(ftpbin)
	{
		ftpbin = 0;
		return(1);
	}
	return(0);
}

/******************************************************************************\
|Routine: extract_host
|Callby: output_file read_in_diredit
|Purpose: Extracts host and user names from file spec, and pointer to path.
|Arguments:
|    i is the offset in lbuf of the path name.
|    lbuf is the complete file name.
|    host is the returned host name.
|    usr is the returned user name.
\******************************************************************************/
Char *extract_host(i,lbuf,host,usr)
Int i;
Char *lbuf,*host,*usr;
{
	Int l;
	Char *p,*q,*r;
	
	p = lbuf + i;	/* points to directory name */
	memcpy(host,lbuf + 1,(l = p - lbuf - 2));
	host[l] = '\0';
	if((q = strchr(host,'@')))
	{
		foundpwd[0] = '\0';	/* prevent old password from being used for new user/host */
		memcpy(usr,host,(l = q - host));
		usr[l] = '\0';
		for(q++,r = host;(*r++ = *q++););	/* eliminate the username from the host string */
	}
	else	/* no user specified, check .netrc */
	{
		if((q = find_netrc(host)))
			strcpy(usr,q);
		else
			strcpy(usr,"anonymous");	/* no .netrc, use "anonymous" */
	}
	return(p);
}

#ifndef NO_FTP
/******************************************************************************\
|Routine: host_filename
|Callby: output_file read_in_diredit
|Purpose: Parses filenames depending on host type.
|Arguments:
|    i is the offset in lbuf of the path name.
|    lbuf is the complete file name.
|    fname is the returned, processed file name.
\******************************************************************************/
Char *host_filename(i,lbuf,fname)
Int i;
Char *lbuf,*fname;
{
	Int ftpsystem;
	Char *p,*q,*r,buf[512];
	
	ftpsystem = ftp_system(lbuf);
	p = lbuf + i;	/* points to directory name */
	if(ftp_unixy(ftpsystem))	/* if server system is unix or dos or os/2 or wnt... */
	{
		if(!*p)
			strcpy(p,".");	/* turn /xxx.yyy.zzz: into /xxx.yyy.zzz:/ */
		while((q = strstr(lbuf + i,"/../")))	/* handle '..' present in file name */
		{
			for(r = q - 1;r > lbuf && *r != '/' && *r != ':';r--);
			if(*r == ':')	/* they tried to back up from top level */
			{
				*++r = '/';
				*++r = '\0';
			}
			else if(r != lbuf && *r == '/')	/* eat everything from preceeding / to last . in /../ */
				for(q += 3;(*r++ = *q++););
		}
		if(p[strlen(p) - 1] != '/')	/* be sure dirname ends with '/' */
			strcat(p,"/");
		if(!strncmp(p,"./",2))
		{
			sprintf(buf,"%s/%s",ftp_cwd(),p + 2);
			sprintf(p,"%s",buf);
			while((q = strstr(lbuf,"//")))
				for(r = q + 1;(*q++ = *r++););
		}
	}
	else	/* server is VMS or VM */
	{
		if(!*p)	/* nothing present past /host: */
			strcpy(p,ftp_cwd());
		else if(!strchr(p,'['))
		{
			strcpy(buf,ftp_cwd());
			strcat(buf,p);
			strcpy(p,buf);
		}
		if(p[strlen(p) - 1] == '/')	/* strip trailing '/' from dirname */
			p[strlen(p) - 1] = '\0';
		while((q = strstr(p,"][.")))	/* collapse things like [a][.b] to [a.b] */
			while((*q = *(q + 2)))
				q++;
		if((q = strstr(p,"[-]")))	/* move up in directory hierarchy if indicated */
		{
			if(ftpsystem == 'I' || ftpsystem == 'T')	/* VM and MTS systems don't have 'up' */
				*q = '\0';
			else
			{
				while(q > p)
				{
					if(*--q == '.')
					{
						strcpy(q,"]");
						break;
					}
					if(*q == ':')	/* they tried to move up from top level */
					{
						strcpy(q,ftp_cwd());
						break;
					}
				}
				if(q == p)
					strcpy(p,ftp_cwd());
			}
		}
		else if(!strncmp(p,"[]",2))
		{
			sprintf(buf,"%s%s",ftp_cwd(),p + 2);
			sprintf(p,"%s",buf);
		}
		else if(!strncmp(p,"[.",2))
		{
			strcpy(buf,ftp_cwd());
			strcat(buf,p);
			strcpy(p,buf);
			while((q = strstr(p,"][.")))	/* collapse things like [a][.b] to [a.b] */
				while((*q = *(q + 2)))
					q++;
		}
	}
	strcpy(fname,lbuf);	/* return the actual name */
	return(p);
}

/******************************************************************************\
|Routine: ftp_date
|Callby: load_file
|Purpose: Converts a directory listing date/time entry into yyyy mm-dd hh:mm.
|Arguments:
|    month is the month, one of Jan, Feb,...
|    day is the day of the month.
|    hour is hh:mm or yyyy.
\******************************************************************************/
Char *ftp_date(month,day,hour)
Char *month,*hour;
Int day;
{
	Char *buf;
	static Char buf2[128];
	time_t l;

/* get the year */
	if(hour[2] == ':')
	{
		l = time(0);
		buf = (Char *)ctime(&l);
		memcpy(buf2,buf + 20,4);
	}
	else
		memcpy(buf2,hour,4);
	buf2[4] = ' ';
/* do the month */
	sprintf(buf2 + 5,"%02d-",(int)((strstr(months,month) - months) / 3) + 1);
/* do the day */
	sprintf(buf2 + 8,"%02d",day);
	buf2[10] = ' ';
/* do hours, minutes */
	if(hour[2] != ':')
		strcpy(buf2 + 11,"00:00");
	else
		memcpy(buf2 + 11,hour,5);
	buf2[16] = '\0';
	return(buf2);
}
#endif

/******************************************************************************\
|Routine: proper_date
|Callby: read_in_diredit
|Purpose: Converts a statbuf mtime entry into yyyy mm-dd hh:mm.
|Arguments:
|    mtime is the statbuf mtime entry.
\******************************************************************************/
Char *proper_date(mtime)
time_t mtime;
{
	Char *buf,monbuf[4];
	static Char buf2[64];

	buf = ctime(&mtime);
	if(!buf)
	{
		strcpy(buf2,"1960 04-18 02:35");	/* CWS if mtime bogus */
		return(buf2);
	}
	memcpy(buf2,buf + 20,4);
	buf2[4] = ' ';
	memcpy(monbuf,buf + 4,3);
	monbuf[3] = '\0';
	sprintf(buf2 + 5,"%02d-",(int)((strstr(months,monbuf) - months) / 3) + 1);
	memcpy(buf2 + 8,buf + 8,8);
	if(buf2[8] == ' ')
		buf2[8] = '0';
	buf2[16] = '\0';
	return(buf2);
}

/******************************************************************************\
|Routine: map_file
|Callby: load_file
|Purpose: Maps a file into memory.
|Arguments:
|    file is the name of the file.
|    bytes is the returned size of the mapped area in bytes.
\******************************************************************************/
Char *map_file(file,bytes)
Char *file;
Int *bytes;
{
	struct stat statbuf;
	Int fd;
	Char *p,*errmsg,buf[512];
	mmap_ptr m;
#ifdef VMS
	Char *q;
	Int remain,nbytes;
#endif
#ifdef NO_MMAP
	Int l;
#endif
	
/* get the file's size in bytes */
	if(stat(file,&statbuf) < 0)
	{
#ifdef VMS
		return(0);	/* VMS doesn't really support errno, nor even vaxc$errno */
#else
report_error:
		if(errno == ENOENT)
			return(0);
		sprintf(buf,"Unable to open file '%s'.",file);
		slip_message(buf);
		errmsg = (Char *)sys_errlist[errno];
		slip_message(errmsg);
		wait_message();
		maperror = 1;
		return(0);
#endif
	}
		
/* open the file */
#ifdef VMS
	if((fd = open(file,O_RDONLY,0,"ctx=rec","rfm=fix","mrs=512","shr=get,put,del,mse,upi,upd")) < 0)
		return(0);
#else
	if((fd = open(file,O_RDONLY | O_BINARY,0)) < 0)
		goto report_error;
#endif
#ifdef NO_MMAP
	if(!(p = (Char *)imalloc(statbuf.st_size)))
	{
		close(fd);
		return(0);
	}
#ifdef VMS
	q = p;
	remain = statbuf.st_size;
	while(remain > 0)
	{
		if((nbytes = read(fd,q,remain)) <= 0)
			break;
		q += nbytes;
		remain -= nbytes;
	}
	statbuf.st_size -= remain;
#else
	if((l = read(fd,p,statbuf.st_size)) != statbuf.st_size)
	{
		slip_message("That file didn't read in correctly.");
		wait_message();
		statbuf.st_size = l;
	}
#endif
	close(fd);
#else
	p = (Char *)mmap(0,statbuf.st_size,PROT_READ | PROT_WRITE,MAP_PRIVATE,fd,0);
	close(fd);
	if(p == (Char *)(-1))
		return(0);
#endif
	for(m = (mmap_ptr)&base;m->next;m = m->next);
	m->next = (mmap_ptr)imalloc(sizeof(mmap_node));
	m = m->next;
	m->next = NULL;
	m->adr = p;
	*bytes = m->len = statbuf.st_size;
	return(p);
}

/******************************************************************************\
|Routine: unmap_file
|Callby: include_file main
|Purpose: Unmaps a file.
|Arguments:
|	adr is the base address of the mapped region.
\******************************************************************************/
void unmap_file(adr)
Char *adr;
{
	mmap_ptr m,p;
	
	for(p = (mmap_ptr)&base,m = base;m;p = m,m = m->next)
		if(m->adr == adr)
		{
#ifdef NO_MMAP
			ifree(adr);
#else
			munmap(adr,m->len);
#endif
			p->next = m->next;
			ifree(m);
			return;
		}
	vtend();
	printf("\nA call to unmap_file passed an unrecognized address.\n");
	cleanup(0);
	exit(0);
}

/******************************************************************************\
|Routine: checkbook
|Callby: load_file
|Purpose: Checks whether a file has a bookmark. Returns the position found in
|         the bookmark file, or 0.
|Arguments:
|    editfile is the name of the file.
|    bookmark is the returned bookmark file name, or null string.
|    byteoffset is the byte offset within the record.
\******************************************************************************/
Int checkbook(editfile,bookmark,byteoffset)
Char *editfile,*bookmark;
Int *byteoffset;
{
#ifdef GNUDOS
	bookmark[0] = '\0';
	return(0);
#else
	FILE *fp;
	Int initline;
	
	strcpy(bookmark,editfile);
	strcat(bookmark,"-EDbookmark");
	initline = 0;
	if((fp = fopen(bookmark,"r")))
	{
		if(my_fscanf(fp,"%d,%d",&initline,byteoffset) == 2)
		{
			if(initline < 1)
				initline = 1;
			fclose(fp);
		}
		else
		{
			fclose(fp);
			bookmark[0] = '\0';
		}
	}
	else
		bookmark[0] = '\0';
	return(initline);
#endif
}

/******************************************************************************\
|Routine: read_stdin
|Callby: load_file
|Purpose: Reads an ASCII file from standard input, creating records.
|Arguments:
|	fname is the buffer that receives the file name.
|	base is the base of the file's record queue.
\******************************************************************************/
Int read_stdin(fname,base)
Char *fname;
rec_ptr base;
{
	Char buffer[BUFFER_SIZE];
	register rec_ptr new;
	register Int i,nrecs;
	
	nrecs = 0;
	strcpy(fname," Standard Input ");
	new = NULL;
	while(gets(buffer))
	{
		if((i = strlen(buffer)) == BUFFER_SIZE)
			i--;
		buffer[i++] = '\n';
		nrecs++;
		new = (rec_ptr)imalloc(sizeof(rec_node));	/* get new record */
		insq(new,base->prev);
		new->data = (Char *)imalloc(i);	/* get buffer for record */
		if(--i > 0)	/* copy record data if present */
			memcpy(new->data,buffer,i);
		new->data[i] = 0;
		new->length = i;
		new->recflags = 1;
	}
	return(nrecs);
}

/******************************************************************************\
|Routine: read_stdin_binary
|Callby: load_file
|Purpose: Reads a binary file from standard input, creating records.
|Arguments:
|	fname is the buffer that receives the file name.
|	base is the base of the file's record queue.
\******************************************************************************/
Int read_stdin_binary(fname,base)
Char *fname;
rec_ptr base;
{
	Char buffer[BUFFER_SIZE];
	register Char *p,*stop;
	register rec_ptr new;
	register Int i,nrecs;

	nrecs = 0;
	strcpy(fname," Standard Input ");
	for(p = buffer,stop = buffer + BINARY_RECSIZE;(i = getchar()) != EOF;)
	{
		*p++ = i;
		if(p == stop)
		{
			nrecs++;
			new = (rec_ptr)imalloc(sizeof(rec_node));
			insq(new,base->prev);
			new->data = (Char *)imalloc(BINARY_RECSIZE);
			memcpy(new->data,(p = buffer),BINARY_RECSIZE);
			new->length = BINARY_RECSIZE;
			new->recflags = 1;
		}
	}
	if((i = p - buffer))
	{
		nrecs++;
		new = (rec_ptr)imalloc(sizeof(rec_node));
		insq(new,base->prev);
		new->data = (Char *)imalloc(i);
		memcpy(new->data,buffer,i);
		new->length = i;
		new->recflags = 1;
	}
	return(nrecs);
}

/******************************************************************************\
|Routine: read_in
|Callby: load_file
|Purpose: Reads an ASCII file, creating records.
|Arguments:
|	mapped is the mapped file pointer.
|   nbytes is the size of the mapped region.
|	base is the base of the file's record queue.
\******************************************************************************/
Int read_in(mapped,nbytes,base)
Char *mapped;
Int nbytes;
rec_ptr base;
{
	rec_ptr new;
	register Int nrecs,inprog;
	register Char *p,*q,*stop;
	
	nrecs = 0;
	new = NULL;
	inprog = 0;
	for(stop = mapped + nbytes,q = p = mapped;p != stop;p++)
		if(*p == '\n')
		{
			nrecs++;
			new = (rec_ptr)imalloc(sizeof(rec_node));	/* get new record */
			insq(new,base->prev);
			new->data = q;	/* use mapped address as record data */
			if((new->length = p - q) > 0)
				if(new->data[new->length - 1] == '\r')	/* NULL-terminate the buffer (mapped data are MAP_PRIVATE) */
					new->length--;
			new->data[new->length] = '\0';
			new->recflags = 0;	/* don't free this buffer, it's mapped file data */
			q = p + 1;
			inprog = 0;
		}
		else
			inprog = 1;
	if(inprog)	/* the last record doesn't have newline termination, handle it specially */
	{
#ifdef GNUDOS
		for(stop = q;stop != p;stop++)
			if(*stop == 26)
				break;
		if(stop != p)
		{
			*stop = '\0';
			if(!strlen(q))
				return(nrecs);
			p = stop;
		}
#endif
		nrecs++;
		new = (rec_ptr)imalloc(sizeof(rec_node));	/* get new record */
		insq(new,base->prev);
		new->length = p - q;
		new->data = (Char *)imalloc(new->length + 1);	/* use mapped address as record data */
		memcpy(new->data,q,new->length);	/* copy data to buffer */
		new->data[new->length] = 0;	/* NULL-terminate the buffer (mapped data are MAP_PRIVATE) */
		new->recflags = 1;	/* this buffer should be freed */
	}
	return(nrecs);
}

/******************************************************************************\
|Routine: read_in_binary
|Callby: load_file
|Purpose: Reads a file in binary mode, creating 32-byte records.
|Arguments:
|	mapped is the mapped file pointer.
|   nbytes is the size of the mapped region.
|	base is the base of the file's record queue.
\******************************************************************************/
Int read_in_binary(mapped,nbytes,base)
Char *mapped;
Int nbytes;
rec_ptr base;
{
	register Int i,nrecs;
	register Char *p,*stop;
	rec_ptr new;
	
	nrecs = 0;
	for(stop = mapped + nbytes - BINARY_RECSIZE,p = mapped;p <= stop;p += BINARY_RECSIZE)
	{
		nrecs++;
		new = (rec_ptr)imalloc(sizeof(rec_node));
		insq(new,base->prev);
		new->data = p;
		new->length = BINARY_RECSIZE;
		new->recflags = 0;
	}
	if((i = (mapped + nbytes) - p))
	{
		nrecs++;
		new = (rec_ptr)imalloc(sizeof(rec_node));
		insq(new,base->prev);
		new->data = p;
		new->length = i;
		new->recflags = 0;
	}
	return(nrecs);
}

/******************************************************************************\
|Routine: host_in_name
|Callby: load_file read_in_diredit
|Purpose: Returns >0 if /xxx.yyy.zzz: is present in file name. Return value is
|         the offset to the character after the colon.
|Arguments:
|	lbuf is the file name.
\******************************************************************************/
Int host_in_name(lbuf)
Char *lbuf;
{
	Char *p,c;
	
#ifdef NO_FTP
	return(0);
#else
	if(lbuf[0] == '/')
	{
		for(p = lbuf + 1;(c = *p);p++)
		{
			if(c == ':')
				break;
			if(!isalpha(c) && !isdigit(c) && c != '+' && c != '.' && c != '-' && c != '_' && c != '@')
			{
				c = '\0';
				break;
			}
		}
		if(c)	/* it's an /xxx.yyy.zzz: filename */
			return(++p - lbuf);
	}
	return(0);
#endif
}

/******************************************************************************\
|Routine: read_in_diredit
|Callby: load_file
|Purpose: Creates a diredit buffer.
|Arguments:
|	fname is the buffer that receives the title string. If a VMS system, this is
|         in the form disk:[dir.dir...]. Returns 1 on success, -2 on ftp failure.
|	buf is the directory name.
|	base is the base of the file's record queue.
\******************************************************************************/
Int read_in_diredit(fname,buf,base)
Char *fname,*buf;
rec_ptr base;
{
	Char retbuf[512],lbuf[512],cwd[512],host[512],record[512],command[512],usr[64],fbuf[512],fbuf1[512],monthyear[512],msgbuf[128];
	rec_ptr new,tmp;
	struct stat statbuf;
	Int i,l,nrecs,ihour,ftpsystem,lasttime,thistime,ticker,mustrestore;
	Long context;
	Char *p,*q,c;
/* variables used for parsing remote listings */
	Char mode[64],group[64],owner[64],month[64],hour[64],*recptr;
	Int links,size,day,pred,year,minute,imonth;
	
	strcpy(lbuf,buf);
/* handle remote host directories */
#ifndef NO_FTP
	if((i = host_in_name(lbuf)))	/* this is true if /xxx.yyy.zzz: is in the file name */
	{
		p = extract_host(i,lbuf,host,usr);
/* get the directory listing from the remote host */
		if(!ftp_open(host,usr))
			return(-2);
		ftpsystem = ftp_system(buf);
		p = host_filename(i,lbuf,fname);
		if(ftpsystem == 'I' || ftpsystem == 'T')	/* VM systems emulate VMS systems, mostly */
		{
			if(strlen(p))
			{
				p++;	/* advance past [ character */
				if((q = strchr(p,']')))	/* find ] character */
				{
					if(!*(q + 1))	/* only a directory is specified, strip [, ] from VM dirname */
						*q = '\0';
					else
						return(-1);	/* there is a file name present, fail */
				}
				sprintf(command,"CWD %s",p);
			}
			else
				command[0] = '\0';
		}
		else
		{
			if(ftpsystem == 'M' || ftpsystem == 'O' || ftpsystem == 'W')
			{
				if(strlen(p) > 1)
				{
					if(!(strlen(p) == 3 && *(p + 1) == ':' && *(p + 2) == '/'))
						if(*(q = p + strlen(p) - 1) == '/')
							*q = '\0';
				}
				strcpy(mode,"rwxrwxrwx");
			}
			sprintf(command,"CWD %s",p);
		}
		if(strlen(command))
			if(!ftp_command(command))	/* the -1 tells caller to try opening it as a file, rather than a directory */
				if(!strstr(last_message(),"ataset"))
					return(-1);
		if(!ftp_data("LIST"))
			return(-2);
		new = (rec_ptr)imalloc(sizeof(rec_node));
		insq(new,base->prev);
/* insert a record allowing user to move up */
		if(ftp_unixy(ftpsystem))
		{
			new->data = (Char *)imalloc(FILENAME_OFFSET + strlen("..") + 1);	/* get buffer for record */
			sprintf(new->data,"d        0                              ..");
		}
		else
		{
			new->data = (Char *)imalloc(FILENAME_OFFSET + strlen("[-]") + 1);	/* get buffer for record */
			sprintf(new->data,"d        0                              [-]");
		}
		new->length = strlen(new->data);
		new->recflags = 1;
		nrecs = 1;
		if(ftpsystem == 'U')
		{
			if(!ftp_record(record))	/* return 1 because ".." or "[-]" is always present */
				return(1);
			if(!strstr(record,"otal"))
				goto nototal;
		}
		lasttime = time(0);
		ticker = 0;
/* get directory listing */
		while(ftp_record(record))
		{
			if(!strlen(record))
				continue;			
/* check for report of files listed */
			ticker = (ticker + 1) & 31;
			if(!ticker)
			{
				thistime = time(0);
				if(thistime - lasttime > 2)
				{
					if(ttychk() == 3)
					{
						ftp_abort();
						break;
					}
					lasttime = thistime;
					sprintf(msgbuf,"Total files so far: %d.",nrecs);
					move(BOTROW,1);
					ers_end();
					reverse();
					putz(msgbuf);
					normal();
					putout();
					mustrestore = 1;
				}
			}
nototal:
/* unix systems */
/*
dr-xr-xr-x   1 owner    group             0 Aug 13 11:13 cs
-r-xr-xr-x   1 owner    group          1420 Aug  3  8:53 README.TXT
*/
			if(ftpsystem == 'U')
			{
				memcpy(mode,record + 1,9);	/* get mode separately, since it may glom into link count */
				mode[9] = '\0';
				p = record + 10;
				if(*p == '+')
					p++;
				if(my_sscanf(p,"%d %s %s %d %s %d %s %n",&links,group,owner,&size,month,&day,hour,&pred) != 7)
					if(my_sscanf(p,"%d %s %d %s %d %s %n",&links,owner,&size,month,&day,hour,&pred) != 6)
					{
						strcpy(mode,"?????????");	/* no permission info */
						fake_date(&size,&year,month,&day,&ihour,&minute);
						strcpy(hour,"06:35");
/* maybe it looks like this?
GPC_PLB/              =  Picture Level Benchmark files
README              685  General info about this site
README.TeX3.141    3237  
TeX3.141.tar.Z  9664553  
a.e.d/                -  Archive of alt.education.disabled postings
anyone/               =  World writable directory for new software
bin/                  -  
describe-1.8.sgi.tar.Z
                 124985  Tools to provide descriptive ls, what you are seeing
 */
						if(strlen(record) >= 23 && (record[22] == '-' || record[22] == '='))
						{
							size = 0;
							for(p = record + 21;*p == ' ';p--);
							if(*p == '/')
							{
								*p = '\0';
								strcpy(fbuf,record);
								record[0] = 'd';	/* record[0] is reported to the first column of the diredit record */
							}
							else
							{
								*++p = '\0';
								strcpy(fbuf,record);
								record[0] = '-';
							}
						}
						else if((p = strchr(record,' ')))
						{
							if(my_sscanf(p,"%d",&size) != 1)
								size = 0;
							*p = '\0';
							strcpy(fbuf,record);
							record[0] = '-';
						}
						else
						{
							strcpy(fbuf,record);
							if(!ftp_record(record))
								break;
							if(my_sscanf(record,"%d",&size) != 1)
								size = 0;
							if(fbuf[strlen(fbuf) - 1] == '/')
								record[0] = 'd';
							else
								record[0] = '-';
						}
						recptr = fbuf;
					}
				if(mode[0] != '?')	/* note that this will not happen if the weird format above obtains */
					for(recptr = p + pred;*recptr == ' ';recptr++);	/* skip spaces after the hour, leaving recptr aimed at file name  */
			}
/* MSDOS system
 124436480           CDISK.TAR   Thu Jul 22 16:53:32 1993
<dir>                      SAV   Thu Jul 22 16:53:46 1993
 */
			else if(ftpsystem == 'M')
			{
				if(!strncmp(record,"<dir>",(pred = strlen("<dir>"))))
					size = 0;
				else
					if(my_sscanf(record,"%d%n",&size,&pred) != 1)
						continue;
				for(p = record + pred;isspace(*p);p++);
				for(q = p + 1;!isspace(*q);q++);
				*q = '\0';
				strcpy(fbuf,p);
				q += 7;	/* skip foward past day-of-week */
				if(my_sscanf(q,"%s%d%d:%d:%d%d",month,&day,&ihour,&minute,&i,&year) != 6)
					continue;
				sprintf(hour,"%02d:%02d",ihour,minute);
				recptr = fbuf;
				record[0] = (record[0] == '<')? 'd' : '-';
			}
/* OS/2 system
                 0           DIR   07-24-93   13:55  DOS
              8440      A          09-08-92   20:35  OS2LDR.MSG
*/
			else if(ftpsystem == 'O')
			{
				if(my_sscanf(record,"%d",&size) != 1)
					continue;
				record[0] = (record[29] == 'D')? 'd' : '-';
				if(my_sscanf(record + 35,"%d-%d-%d%s",&i,&day,&year,hour) != 4)
					continue;
				memcpy(month,months + (i - 1) * 3,3);
				month[3] = '\0';
				year += (year < 70)? 2000 : 1900;
				strcpy(fbuf,record + 53);
				recptr = fbuf;
			}
/* MTS system
r                 Jul 21  1991 ADDHELP.ZIP
*/
			else if(ftpsystem == 'T')
			{
				size = 0;
				if(my_sscanf(record,"%s%s%d%s%s",fbuf,month,&day,hour,fbuf1) != 5)
					if(my_sscanf(record,"%s%d%s%d%s%s",fbuf,&size,month,&day,hour,fbuf1) != 6)
						continue;
				strcpy(mode,"---------");	/* MTS reports as rw or whatever */
				if(strchr(fbuf,'r'))
					mode[0] = mode[3] = mode[6] = 'r';
				if(strchr(fbuf,'w'))
					mode[1] = mode[4] = mode[7] = 'w';
				if(strchr(fbuf,'e') || strchr(fbuf,'x'))
					mode[2] = mode[5] = mode[8] = 'x';
				recptr = fbuf1;
			}
/* VM system */
			else if(ftpsystem == 'I')
			{
/*$READ-ME FIRST    V         71         27          1  4/28/93  3:22:47 PUBLIC*/
				if(my_sscanf(record,"%s%s%s%d%d%d%d/%d/%d%d:%d",fbuf,fbuf1,monthyear,&i,&i,&size,&imonth,&day,&year,&ihour,&minute) == 11)
				{
					size *= 8;	/* it's going to be multiplied by 512 below. assume 4096-byte blocks */
					strcpy(mode,"?????????");	/* VM doesn't report permission info */
					memcpy(month,months + 3 * (imonth - 1),3);
					month[3] = '\0';
					year += (year < 70)? 2000 : 1900;
					strcat(fbuf,".");
					strcat(fbuf,fbuf1);
					recptr = fbuf;
				}
/*APR87                       FILE41 PS FB    133 11440*/
				else if(my_sscanf(record,"%s%s%s%s%d%d",fbuf,fbuf1,fbuf1,fbuf1,&i,&i) == 6)
				{
					strcpy(mode,"?????????");	/* VM doesn't report permission info */
					fake_date(&size,&year,month,&day,&ihour,&minute);
					recptr = fbuf;
				}
				else
				{
					if((p = strchr(record,' ')))
					{
						*p = '\0';
						sprintf(fbuf,"[%s]",record);
						fake_date(&size,&year,month,&day,&ihour,&minute);
						strcpy(mode,"?????????");
						recptr = fbuf;
					}
					else
						continue;
				}
			}
/* WNT system */
			else if(ftpsystem == 'W')
			{
/*
08-10-93  09:50AM                   96 GO.BAT
05-06-93  10:53AM       <DIR>          OADDOS
*/
				if(my_sscanf(record,"%d-%d-%d%d:%s%s%s",&day,&imonth,&year,&ihour,fbuf,fbuf1,monthyear) == 7)
				{
					if(fbuf[2] == 'P')
						ihour += 12;
					fbuf[2] = '\0';
					my_sscanf(fbuf,"%d",&minute);
					sprintf(hour,"%02d:%02d",ihour,minute);
					memcpy(month,months + (imonth - 1) * 3,3);
					month[3] = '\0';
					year += (year < 70)? 2000 : 1900;
					if(!strcmp(fbuf1,"<DIR>"))
					{
						record[0] = 'd';
						size = 0;
					}
					else
					{
						record[0] = '-';
						my_sscanf(fbuf1,"%d",&size);
					}
					recptr = monthyear;
				}
/*
dr-xr-xr-x   1 owner    group             0 Aug 13 11:13 cs
-r-xr-xr-x   1 owner    group          1420 Aug  3  8:53 README.TXT
*/
				else
				{
					memcpy(mode,record + 1,9);	/* get mode separately, since it may glom into link count */
					mode[9] = '\0';
					p = record + 10;
					if(*p == '+')
						p++;
					if(my_sscanf(p,"%d %s %s %d %s %d %s %n",&links,group,owner,&size,month,&day,hour,&pred) != 7)
						if(my_sscanf(p,"%d %s %d %s %d %s %n",&links,owner,&size,month,&day,hour,&pred) != 6)
							continue;
					for(recptr = p + pred;*recptr == ' ';recptr++);	/* skip spaces after the hour, leaving recptr aimed at file name  */
				}
			}
			else if((p = strchr(record,';')))	/* VMS has ; only where filename appears */
			{
/*
22AVG.FIL;1                     File ID:  (69,1,0)
Size:           2/3             Owner: [RHR]
Created:   27-DEC-1987 17:31    Revised:  24-MAY-1989 11:00
Expires:   <None specified>     Backup:   <None specified>
File organization:  Sequential
File attributes:    Allocation: 3, Extend: 0, Global buffer count: 0
                    No version limit
Record format:      Fixed length, maximum 512 bytes
Journaling enabled: None
File protection:    System:RWED,Owner:RWED,Group:RE,World:
Access Cntrl List:  None
 */
				if(strstr(record,"File ID:"))	/* this server uses DIR/FULL listing, yawn */
				{
					while(!isspace(*++p));	/* trim blanks after file name */
					*p = '\0';
					fix_vms_dir(record,fbuf);	/* fix up .DIR;1 files */
					if(!ftp_record(record))
						break;
					for(p = record;!isspace(*p);p++);	/* retrieve file size */
					my_sscanf(p,"%d",&size);
					if(!ftp_record(record))
						break;
					if(!(p = strstr(record,"Revised:")))	/* retrieve revision date */
					{
						fake_date(&size,&year,month,&day,&ihour,&minute);
bad_mode:
						strcpy(mode,"?????????");
					}
					else
					{
						p += 8;
						while(isspace(*p))
							p++;
						for(q = p;(c = *q);q++)
							if(c == '-' || c == ':')
								*q = ' ';
						my_sscanf(p,"%d %s %d %d %d",&day,month,&year,&ihour,&minute);
						month[1] = tolower(month[1]);
						month[2] = tolower(month[2]);
						while(!strstr(record,"File protection:"))	/* read forward until protection info appears */
							if(!(i = ftp_record(record)))
								break;
						if(!i)	/* premature termination of listing */
							break;
						if(!(p = strstr(record,"Owner:")))	/* parse out the protection info */
							goto bad_mode;
						parse_vms_perms(p += strlen("Owner:"),mode);
						if(!(p = strstr(record,"Group:")))
							goto bad_mode;
						parse_vms_perms(p += strlen("Group:"),mode + 3);
						if(!(p = strstr(record,"World:")))
							goto bad_mode;
						parse_vms_perms(p += strlen("World:"),mode + 6);
						mode[9] = '\0';
					}
				}
/* parse other VMS listing formats */
				else
				{
					while(*p && !isspace(*p))	/* find end of record or whitespace */
						p++;
					if(!*p)	/* only the filename appears, it's either a two-liner, or a horrible system */
					{
						strcpy(fbuf,record);
						if(!ftp_record(record))
							break;
						if(!isspace(record[0]))	/* it's one of those horrible systems that don't tell you anything about files */
						{
							fake_date(&size,&year,month,&day,&ihour,&minute);	/* use fake date info */
							strcpy(mode,"?????????");	/* use confused permission info */
							fix_vms_dir(fbuf,fbuf);	/* handle .DIR;1 files */
							new = (rec_ptr)imalloc(sizeof(rec_node));	/* load the previously-read file name */
							insq(new,base->prev);
							new->data = (Char *)imalloc((l = strlen(fbuf)) + 1 + FILENAME_OFFSET);	/* get buffer for record */
							sprintf(new->data,"%c %s %s  %4d %02d-%02d %02d:%02d  %s",((fbuf[0] == '[')? 'd' : ' '),size_string(size),mode,year,i,day,ihour,minute,fbuf);
							new->data[l + FILENAME_OFFSET] = 0;
							new->length = l + FILENAME_OFFSET;
							new->recflags = 1;
							nrecs++;
							while(strchr(record,';'))	/* get the rest of the file names */
							{
/* check for report of files listed */
								ticker = (ticker + 1) & 31;
								if(!ticker)
								{
									thistime = time(0);
									if(thistime - lasttime > 2)
									{
										if(ttychk() == 3)
										{
											ftp_abort();
											break;
										}
										lasttime = thistime;
										sprintf(msgbuf,"Total files so far: %d.",nrecs);
										move(BOTROW,1);
										ers_end();
										reverse();
										putz(msgbuf);
										normal();
										putout();
										mustrestore = 1;
									}
								}
								fix_vms_dir(record,record);
								new = (rec_ptr)imalloc(sizeof(rec_node));	/* get new record */
								insq(new,base->prev);
								new->data = (Char *)imalloc((l = strlen(record)) + 1 + FILENAME_OFFSET);	/* get buffer for record */
								sprintf(new->data,"%c %s %s  %4d %02d-%02d %02d:%02d  %s",((record[0] == '[')? 'd' : ' '),size_string(size),mode,year,i,day,ihour,minute,record);
								new->data[l + FILENAME_OFFSET] = 0;
								new->length = l + FILENAME_OFFSET;
								new->recflags = 1;
								nrecs++;
								if(!ftp_record(record))
									break;
							}
							if(mustrestore && NWINDOWS)
								paint(BOTROW,BOTROW,FIRSTCOL);
							return(nrecs);
						}
						p = record;
					}
					else	/* it was a two-liner */
					{
						*p++ = '\0';
						strcpy(fbuf,record);
					}
					fix_vms_dir(fbuf,fbuf);
/* at this point, we have the filename or [.dirname] in fbuf, and p points at whitespace hopefully preceeding one of the following:

DIGLIB.DIR;1        %RMS-E-PRV, insufficient privilege or file protection violation
LOGIN.COM;2          no privilege for attempted operation
DINK.BCK;1                441   1-FEB-1993 10:02 [ANONYMOUS] (RWED,RWED,RE,RE)
VMS-DECWINDOWS.DIR;1
                            1   1-SEP-1990 17:07 [ASD,BRAND] (RWE,RWE,RE,RE)
USER$DISK:[ANONYMOUS]ETHER.TXT;2                  12/12         4-AUG-1988 11:40
00-INDEX.ALL;4        5-NOV-1992 14:43:39    18092/36     (RWED,RWED,R,R)
00._THIS_SITE_NO_LONGER_EXISTS;1
                     30-JUN-1992 09:04:23        0/0      (RWED,RWED,,)
CAESAR.DIR;1              1  13-JUN-1992 13:18:42.55  (RWED,RWED,RE,RE)
archie.rfa;2                    MAR 22 16:04 1993    24064 (,RWE,RE,RE)
VISIT$DISK:[ANONYMOUS]DDN.TAR;1
FTPSTUFF.DIR;1  1/3     29-APR-1991    [INTERNET,ANONYMOUS]   (RWE,RWE,RE,RE)

*/
/* check for privilege problems */
					if(strchr(p,'%') || strstr(p,"no privilege"))
					{
						fake_date(&size,&year,month,&day,&ihour,&minute);
						strcpy(mode,"?????????");
					}
/* try the known forms one at a time */
					else
					{
						strcpy(mode,"?????????");	/* default is meaningless permission info */
/*DINK.BCK;1                441   1-FEB-1993 10:02 [ANONYMOUS] (RWED,RWED,RE,RE)*/
						if(my_sscanf(p,"%d%d-%s%d:%d",&size,&day,monthyear,&ihour,&minute) == 5)
							parse_monthyear(monthyear,&size,&year,month,&day,&ihour,&minute,mode,p);
/*USER$DISK:[ANONYMOUS]ETHER.TXT;2                  12/12         4-AUG-1988 11:40*/
						else if(my_sscanf(p,"%d/%d%d-%s%d:%d",&size,&i,&day,monthyear,&ihour,&minute) == 6)
							parse_monthyear(monthyear,&size,&year,month,&day,&ihour,&minute,mode,p);
/*00-INDEX.ALL;4        5-NOV-1992 14:43:39    18092/36     (RWED,RWED,R,R)*/
						else if(my_sscanf(p,"%d-%s%d:%d:%d%d/%d",&day,monthyear,&ihour,&minute,&i,&i,&size) == 7)
							parse_monthyear(monthyear,&size,&year,month,&day,&ihour,&minute,mode,p);
/*CAESAR.DIR;1              1  13-JUN-1992 13:18:42.55  (RWED,RWED,RE,RE)*/
						else if(my_sscanf(p,"%d%d-%s%d:%d",&size,&day,monthyear,&ihour,&minute) == 5)
							parse_monthyear(monthyear,&size,&year,month,&day,&ihour,&minute,mode,p);
/*FTPSTUFF.DIR;1  1/3     29-APR-1991    [INTERNET,ANONYMOUS]   (RWE,RWE,RE,RE)*/
						else if(my_sscanf(p,"%d/%d%d-%s",&size,&i,&day,monthyear) == 4)
						{
							ihour = minute = 0;
							parse_monthyear(monthyear,&size,&year,month,&day,&ihour,&minute,mode,p);
						}
/*archie.rfa;2                    MAR 22 16:04 1993    24064 (,RWE,RE,RE)*/
						else if(my_sscanf(p,"%s%d%d:%d%d%d",month,&day,&ihour,&minute,&year,&size) != 6)
							fake_date(&size,&year,month,&day,&ihour,&minute);
					}
					month[1] = tolower(month[1]);
					month[2] = tolower(month[2]);
				}
				if((p = strchr(fbuf,']')) && !strstr(fbuf,"[."))
					recptr = ++p;
				else
					recptr = fbuf;
			}
			else	/* ignore extraneous (non-filename) records */
				continue;
/* add to the buffer */
			new = (rec_ptr)imalloc(sizeof(rec_node));	/* get new record */
			insq(new,base->prev);
			new->data = (Char *)imalloc(FILENAME_OFFSET + (l = strlen(recptr)) + 1);	/* get buffer for record */
			if(ftp_unixy(ftpsystem))
				sprintf(new->data,"%c %s %s  %s  %s",(record[0] == '-')? ' ' : record[0],size_string(size),mode,ftp_date(month,day,hour),recptr);
			else if(ftpsystem == 'T')
			{
				i = ((strstr(months,month) - months) / 3) + 1;
				sprintf(new->data,"  %s %s  %s  %s",size_string(size),mode,ftp_date(month,day,hour),recptr);
			}
			else
			{
				i = ((strstr(months,month) - months) / 3) + 1;
				size <<= 9;	/* VMS reports number of 512-byte blocks */
				sprintf(new->data,"%c %s %s  %4d %02d-%02d %02d:%02d  %s",((fbuf[0] == '[')? 'd' : ' '),size_string(size),mode,year,i,day,ihour,minute,recptr);
			}
			new->data[FILENAME_OFFSET + l] = 0;
			new->length = FILENAME_OFFSET + l;
			new->recflags = 1;
			nrecs++;
		}	/* end of while(ftp_record(record)) */
/* remove any . or .. entries that are not the first record */
		for(new = base->next->next;new != base;new = new->next)
			if(!strcmp(new->data + FILENAME_OFFSET,".") || !strcmp(new->data + FILENAME_OFFSET,".."))
			{
				tmp = new->prev;
				remq(new);
				ifree(new->data);
				ifree(new);
				nrecs--;
				new = tmp;
			}
		if(mustrestore && NWINDOWS)
			paint(BOTROW,BOTROW,FIRSTCOL);
		return(nrecs);
	}
#endif	/* not NO_FTP */
#ifdef GNUDOS
	while((p = strchr(lbuf,'\\')))
		*p = '/';
#endif
/* convert the name to absolute pathname */
#ifndef VMS
	if(lbuf[0] != '/')
	{
#ifdef GNUDOS
		if(lbuf[1] != ':')
		{
#endif
#ifdef NeXT
		getwd(cwd);
#else
		getcwd(cwd,sizeof(cwd));
#endif
#ifdef GNUDOS
		while((p = strchr(cwd,'\\')))
			*p = '/';
#endif
		strcat(cwd,"/");
		strcat(cwd,lbuf);
		strcpy(lbuf,cwd);
#ifdef GNUDOS
		}
#endif
	}
	if(lbuf[strlen(lbuf) - 1] != '/')	/* add / to end */
		strcat(lbuf,"/");
	while((p = strstr(lbuf,"//")))		/* eliminate all // */
		for(q = p + 1;(*p++ = *q++););
	while((p = strstr(lbuf,"/./")))		/* eliminate all /./ */
		for(q = p + 2;(*p++ = *q++););
	while((p = strstr(lbuf,"/../")))	/* convert /../ to abs pathname */
	{
		q = p + 3;
		if(!(p == lbuf || (p == lbuf + 2 && lbuf[1] == ':')))
			while(*--p != '/');			/* back up unless at start of string */
		while((*p++ = *q++));
	}
#endif	/* not VMS */
/* set the title */
	strcpy(fname,lbuf);
	nrecs = 0;
/* load the files */
#ifdef VMS
	strcat(lbuf,"*.*;*");
#else
	strcat(lbuf,"*");
#endif
	if((context = find_files(lbuf)))
	{
#ifdef VMS
		new = (rec_ptr)imalloc(sizeof(rec_node));
		insq(new,base->prev);
		new->data = (Char *)imalloc(FILENAME_OFFSET + 3 + 1);	/* get buffer for record */
		sprintf(new->data,"d        0                              [-]");
		new->length = strlen(new->data);
		new->recflags = 1;
#endif
		while(next_file(context,retbuf))
		{
			if(!nrecs++)
			{
				p = retbuf;
				l = filename(p) - p;
			}
			if(stat(retbuf,&statbuf))
			{
				statbuf.st_size = 0;
				statbuf.st_mode = 0;
				statbuf.st_mtime = 0;
			}
			new = (rec_ptr)imalloc(sizeof(rec_node));	/* get new record */
			insq(new,base->prev);
			new->data = (Char *)imalloc((i = strlen(retbuf) - l) + FILENAME_OFFSET + 1);	/* get buffer for record */
#ifdef VMS
			if(S_ISDIR(statbuf.st_mode))
			{
				strcpy(cwd,filename(retbuf));
				*(strstr(cwd,".DIR")) = '\0';
				sprintf(new->data,"d %s %s  %s  [.%s]",size_string(statbuf.st_size),mode_string(statbuf.st_mode),proper_date(statbuf.st_mtime),cwd);
				i = strlen(cwd) + 3;
			}
			else
#endif
			sprintf(new->data,"%c %s %s  %s  %s",S_ISDIR(statbuf.st_mode)? 'd' : ' ',
				size_string(statbuf.st_size),mode_string(statbuf.st_mode),proper_date(statbuf.st_mtime),retbuf + l);
			new->data[FILENAME_OFFSET + i] = 0;
			new->length = FILENAME_OFFSET + i;
			new->recflags = 1;
		}
		find_files_end(context);
	}
	return(nrecs);
}

#ifndef NO_FTP
/******************************************************************************\
|Routine: read_in_ftp
|Callby: load_file
|Purpose: Downloads a file in FTP mode.
|Arguments:
|	fname is the buffer that receives the title string. If a VMS system, this is
|         in the form disk:[dir.dir...]. Returns total records, or -2 if ftp
|         fails, or -1 if CWD fails.
|	buf is the file name.
|	base is the base of the file's record queue.
|   binary is a flag that the file is to be read in binary mode.
\******************************************************************************/
Int read_in_ftp(fname,buf,base,binary)
Char *fname,*buf;
rec_ptr base;
Char binary;
{
	rec_ptr new;
	Int i,l,m,nrecs,ftpsystem,mustrestore;
	Int lasttime,thistime,ticker,totalread,lasttotal,throughput,filesize,percent;
	Char *p,*q,host[512],dir[512],fil[512],lbuf[512],*record,msgbuf[128];
	Char command[512],usr[64];
	
	ftpsystem = ftp_system(buf);	/* this works because we tried read_in_diredit first */
	strcpy(lbuf,buf);
	i = host_in_name(lbuf);
	p = extract_host(i,lbuf,host,usr);
	strcpy(dir,p);
	if(ftp_unixy(ftpsystem))
	{
		if(dir[(l = strlen(dir) - 1)] == '/')
			dir[l] = '\0';
		if(!(q = strrchr(dir,'/')))
		{
			strcpy(fil,dir);
			strcpy(dir,".");	/* No directory present, use default (cws) */
		}
		else
		{
			*q++ = '\0';
			strcpy(fil,q);
			if(!strlen(dir))
				strcpy(dir,"/");
		}
	}
	else
	{
		if((q = strchr(dir,']')))
		{
			strcpy(fil,++q);
			*q = '\0';
		}
		else if((q = strchr(dir,':')))
		{
			strcpy(fil,++q);
			*q = '\0';
		}
		else
		{
			strcpy(fil,dir);
			dir[0] = '\0';
		}
	}
/* open the connection to the remote host */
	if(!ftp_open(host,usr))
		return(-2);
	p = host_filename(i,lbuf,fname);
	if(*(q = fname + strlen(fname) - 1) == '/')	/* trim trailing / */
		*q = '\0';
	if(ftpsystem == 'I')
	{
		if(strlen(++p))
		{
			if((q = strchr(p,']')))
				*q = '\0';
			sprintf(command,"CWD %s",p);
			if(!ftp_command(command))
			{
				sprintf(command,"Unable to move to the directory '%s'.",p);
				ftp_error(command);
				return(-2);
			}
		}
	}
	else
	{
		if(ftpsystem == 'T')
		{
			if(dir[0] == '[')
			{
				strcpy(buf,dir + 1);
				if((q = strchr(buf,']')))
					*q = '\0';
				strcpy(dir,buf);
			}
		}
		sprintf(command,"CWD %s",dir);
		ftp_command(command);	/* we ignore errors here because some servers don't let you even CWD to where you are */
	}
	mustrestore = 0;	/* flag that we have issued progress messages and must set them right */
	totalread = 0;	/* count of bytes read from remote host */
	lasttotal = 0;	/* count at last report (for calculating throughput) */
	if(binary)	/* a binary-mode get */
	{
		if(!ftp_command("TYPE i"))
			return(-2);
		sprintf(command,"RETR %s",fil);
		if(!ftp_data(command))
		{
			if(!ftp_command("TYPE a"))
				return(-2);
			return(-2);
		}
		nrecs = 0;
		ftpbin = 1;
		lasttime = time(0);
		ticker = 0;
		filesize = 0;
		if(CURWINDOW >= 0)
			if(CURREC && CURREC != WINDOW[CURWINDOW].base)
				if(my_sscanf(CURREC->data + 1,"%d",&filesize) != 1)
					filesize = 0;
		record = (Char *)imalloc(512);	/* the 512 here must match what ftp_binary expects */
		while((l = ftp_binary(record)))
		{
			p = record;
			while(l > 0)
			{
				m = (l > 32)? 32 : l;	/* limit records to 32 bytes */
				new = (rec_ptr)imalloc(sizeof(rec_node));	/* get new record */
				insq(new,base->prev);
				new->data = (Char *)imalloc(m + 1);	/* get buffer for record */
				memcpy(new->data,p,m);
				new->data[m] = '\0';
				new->length = m;
				new->recflags = 1;
				nrecs++;
				totalread += m;
				if(filesize)	/* report progress */
				{
					ticker = (ticker + 1) & 31;
					if(!ticker)
					{
						thistime = time(0);
						if(thistime - lasttime > 2)
						{
							if(ttychk() == 3)
							{
								ftp_abort();
								goto abort_binary;
							}
							throughput = (totalread - lasttotal) / 2;
							lasttotal = totalread;
							lasttime = thistime;
							percent = (int)((100.0 * (float)totalread) / ((float)filesize));
							sprintf(msgbuf,"Retrieving (binary mode) %s - %d%% (throughput = %d)",fil,percent,throughput);
							move(BOTROW,1);
							ers_end();
							reverse();
							putz(msgbuf);
							normal();
							putout();
							mustrestore = 1;
						}
					}
				}
				l -= 32;
				p += 32;
			}
abort_binary:
			;
		}
		ifree(record);
		if(!ftp_command("TYPE a"))
			return(-2);
	}
	else	/* an ascii mode get */
	{
		lasttime = time(0);
		ticker = 0;
		filesize = 0;
		if(CURWINDOW >= 0)
			if(CURREC && CURREC != WINDOW[CURWINDOW].base)
				if(my_sscanf(CURREC->data + 1,"%d",&filesize) != 1)
					filesize = 0;
		sprintf(command,"RETR %s",fil);
		if(!ftp_data(command))
			return(-2);
		nrecs = 0;
		record = (Char *)imalloc(32768);	/* this limits the size of FTP-acquired ascii records to 32768 bytes */
		while(ftp_ascii(record,32768))
		{
			new = (rec_ptr)imalloc(sizeof(rec_node));	/* get new record */
			insq(new,base->prev);
			if((l = strlen(record)))
				if(record[l - 1] == '\r')
					l--;
			new->data = (Char *)imalloc(l + 1);	/* get buffer for record */
			strcpy(new->data,record);
			new->data[l] = '\0';
			new->length = l;
			new->recflags = 1;
			nrecs++;
			totalread += l;
			if(filesize)
			{
				ticker = (ticker + 1) & 31;
				if(!ticker)
				{
					thistime = time(0);
					if(thistime - lasttime > 2)
					{
						if(ttychk() == 3)
						{
							ftp_abort();
							break;
						}
						throughput = (totalread - lasttotal) / 2;
						lasttotal = totalread;
						lasttime = thistime;
						percent = (100 * totalread) / filesize;
						sprintf(msgbuf,"Retrieving (ascii mode) %s - %d%% (throughput = %d)",fil,percent,throughput);
						move(BOTROW,1);
						ers_end();
						reverse();
						putz(msgbuf);
						normal();
						putout();
						mustrestore = 1;
					}
				}
			}
		}
		ifree(record);
	}
	if(mustrestore)
		paint(BOTROW,BOTROW,FIRSTCOL);
	return(nrecs);
}
#endif

/******************************************************************************\
|Routine: load_file
|Callby: edit include_file main wincom
|Purpose: Reads an entire file into a record queue. Returns the number of records,
|         or -1 if the file doesn't exist, or -2 if it was an ftp file and an
|         error occurred (and has already been reported).
|Arguments:
|    fname is the name of the file to read in.
|    base is the record queue into which the file goes.
|    bookmark is the name of the bookmark file, if one exists.
|    bookpos is the number of lines to move down in the buffer.
|    bookbyt is the byte offset within the marked record.
|    diredit is a returned flag that this is a directory file.
|    binary is a flag that the file should be loaded in binary mode.
|    filebuf is a returned pointer to the mapped file, or NULL.
\******************************************************************************/
Int load_file(fname,base,bookmark,bookpos,bookbyt,diredit,binary,filebuf)
Char *fname;
rec_ptr base;
Char *bookmark;
Int *bookpos,*bookbyt;
Char *diredit;
Char binary;
Char **filebuf;
{
	register Char *p,*q;
	Int i,l,nbytes,bfound;
	Long context;
	Char ext[512];
	Char buffer[512];
	Char buf[512],retbuf[512],lbuf[512];
	Char *mapped;
	struct stat statbuf;
#ifdef VMS
	Int m;
	struct desc_str
	{
		unsigned short length,class;
		Char *string;
	} desc;
#endif

	if(forcebinary)
	{
		forcebinary = 0;
		binary = 1;
	}
	bookmark[0] = '\0';
	maperror = 0;	/* no map_file error has occurred */
	*filebuf = NULL;	/* default is no file buffer */
	strcpy(buf,fname);
	strip_quotes(buf);
	envir_subs(buf);
/* check for -b or -h in file name */
	if(!binary)	/* if they specified binary, we don't need or want to check. if they didn't, and -b or -h is present, it overrides. */
		if((bfound = parse_option(buf,'b')) || parse_option(buf,'h'))
		{
			binary = 1;
			strcpy(fname,buf);	/* parse_option has remove the -b */
		}
	if(!strlen(fname))	/* this happens if they just say -b or -h without a file */
		if(bfound)
			strcpy(fname,"-b");	/* this lets them actually edit a file called -b */
		else
			strcpy(fname,"-h");
	if(strchr(buf,'*') || strchr(buf,'?') || strstr(buf,"..."))	/* find first match if wildcards are present */
		if((context = find_files(buf)))
		{
			if(next_file(context,retbuf))
				strcpy(buf,retbuf);
			find_files_end(context);
		}
	base->next = base->prev = base;
	strcpy(lbuf,fname);
#ifdef VMS
	m = strlen(lbuf) - 1;
	for(i = 0;i <= m;i++)	/* convert <> to [] */
		if(lbuf[i] == '<')
			lbuf[i] = '[';
		else if(lbuf[i] == '>')
			lbuf[i] = ']';
	if(lbuf[m] == ']')	/* is a directory file */
	{
		*diredit = 1;
		if((p = strstr(buf,"[]")))
		{
			desc.class = 0x010e;
			desc.length = sizeof(retbuf);
			desc.string = retbuf;
			SYS$SETDDIR(NULL,&i,&desc);
			retbuf[i] = '\0';
			strcpy(p,retbuf);
		}
		return(read_in_diredit(fname,buf,base));
	}
#endif
#ifndef NO_FTP
	if((i = host_in_name(fname)))
	{
		if(!fileonly)
		{
			if((i = read_in_diredit(fname,buf,base)) > 0)
			{
				*diredit = 1;
				return(i);
			}
			else if(i == -2)	/* we fall through here if it returns -1, which means CWD failed */
				return(-2);
		}
		fileonly = 0;	/* ftp_fileonly is a one-shot deal */
/* open as directory failed, try opening as file */
		*diredit = 0;
		if(buf[(l = strlen(buf) - 1)] == '/')
			buf[l] = '\0';
		return(read_in_ftp(fname,buf,base,binary));
	}
#endif
	fileonly = 0;	/* just to be sure */
	if(!stat(fname,&statbuf))
		if(S_ISDIR(statbuf.st_mode))
		{
			*diredit = 1;
			return(read_in_diredit(fname,buf,base));
		}
	*diredit = 0;
	if(!strncmp(buf,"./",2))
		for(p = buf,q = buf + 2;(*p++ = *q++););
	if(binary)
	{
#ifndef VMS
		if(!strcmp(buf,"-"))
			return(read_stdin_binary(fname,base));
#endif
		if((*filebuf = mapped = map_file(buf,&nbytes)))
		{
			if((i = checkbook(buf,bookmark,bookbyt)))
				*bookpos = i;
			strcpy(fname,buf);
			return(read_in_binary(mapped,nbytes,base));
		}
		else if(maperror)
			return(-2);
	}
	else
	{
#ifndef VMS
		if(!strcmp(buf,"-"))
			return(read_stdin(fname,base));
#endif
		if((*filebuf = mapped = map_file(buf,&nbytes)))
		{
			if((i = checkbook(buf,bookmark,bookbyt)))
				*bookpos = i;
			strcpy(fname,buf);
			return(read_in(mapped,nbytes,base));
		}
		else if(maperror)
			return(-2);
	}
/* file as given not found, try extensions if none present */
	if(!(p = (Char *)extension(buf)))
	{
		p = buf + strlen(buf);	/* skip ahead in the file name, preparing to append extensions */
		*p++ = '.';
		strcpy(buffer,DEFAULT_EXT);	/* prepare to parse extensions from string */
		while(get_token(buffer,ext))
		{
			strcpy(p,ext);
			if((*filebuf = mapped = map_file(buf,&nbytes)))
			{
				if((i = checkbook(buf,bookmark,bookbyt)))
					*bookpos = i;
				strcpy(fname,buf);
				if(binary)
					return(read_in_binary(mapped,nbytes,base));
				else
					return(read_in(mapped,nbytes,base));
			}
			else if(maperror)
				return(-2);
		}
		*--p = '\0';	/* remove the extension before returning to caller */
	}
	return(-1);
}

