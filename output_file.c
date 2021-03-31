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
#include <stdio.h>
#include <errno.h>

#include "unistd.h"
#include "memory.h"
#include "file.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "buffer.h"

extern Char *extension();
extern Char *filename();
extern Char *extract_host();
extern Char *map_file();

static Int backupfiles = 0;
static Int specialnobak = 0;
static Char backupdir[512];

/******************************************************************************\
|Routine: output_buffer
|Callby: output_file
|Purpose: Generates a buffer containing what is to be written out.
|Arguments:
|	none
\******************************************************************************/
void output_buffer(recbuf)
buf_ptr recbuf;
{
	rec_ptr rec,otherrec;
	Int otherbyt,nrecs,i;
	Int offset;
	
	offset = get_offset(SELREC,SELBYT,get_seldir());
	if(!offset)
		return;
	else if(BOXCUT)
	{
		killer_box();
		copy_only(1);
		killer(offset,recbuf,0);
	}
	else if(offset < 0)	/* select marker preceeds current position */
	{
/* find the other end of the buffer, backwards */
		otherrec = CURREC;
		otherbyt = CURBYT;
		nrecs = 1;
		while(offset++ < 0)
		{
			if(!otherbyt)
			{
				if(otherrec->prev == BASE)
					break;
				otherrec = otherrec->prev;
				otherbyt = otherrec->length;
				nrecs++;
			}
			else
				otherbyt--;
		}
/* save records to buffer */
		if(otherrec == CURREC)
		{
			buffer_append(recbuf,CURREC,otherbyt,CURBYT);
			nrecs = 1;
		}
		else
		{
			rec = otherrec;
			buffer_append(recbuf,rec,otherbyt,rec->length);
			rec = rec->next;
			for(i = 1;i < nrecs - 1;i++)
			{
				buffer_append(recbuf,rec,0,rec->length);
				rec = rec->next;
			}
			if(CURBYT)
				buffer_append(recbuf,rec,0,CURBYT);
			else
				nrecs--;
		}
		recbuf->nrecs = nrecs;
	}
	else if(offset > 0)	/* select marker follows current position */
	{
/* find the other end of the buffer, forwards */
		otherrec = CURREC;
		otherbyt = CURBYT;
		nrecs = 1;
		while(offset-- > 0)
		{
			if(otherrec == BASE)
				break;
			if(otherbyt == otherrec->length)
			{
				otherrec = otherrec->next;
				otherbyt = 0;
				nrecs++;
			}
			else
				otherbyt++;
		}
/* save records to buffer */
		if(otherrec == CURREC)
		{
			buffer_append(recbuf,CURREC,CURBYT,otherbyt);
			nrecs = 1;
		}
		else
		{
			buffer_append(recbuf,CURREC,CURBYT,CURREC->length);
			rec = CURREC->next;
			for(i = 1;i < nrecs - 1;i++)
			{
				buffer_append(recbuf,rec,0,rec->length);
				rec = rec->next;
			}
			if(otherbyt)
				buffer_append(recbuf,rec,0,otherbyt);
			else
				nrecs--;
		}
		recbuf->nrecs = nrecs;
	}
}

/******************************************************************************\
|Routine: set_output_bak
|Callby: parse_fnm
|Purpose: Sets flag to cause backup of old versions to .BAK or backup directory.
|Arguments:
|	dirname is the directory to move backup files to, or NULL if .BAK files are
|           to be created.
\******************************************************************************/
void set_output_bak(dirname)
Char *dirname;
{
	backupfiles = 1;
	if(!dirname)
		backupdir[0] = '\0';
	else
	{
		strcpy(backupdir,dirname);
		envir_subs(backupdir);
		if(backupdir[strlen(backupdir) - 1] != '/')
			strcat(backupdir,"/");
	}
}

/******************************************************************************\
|Routine: set_output_nobak
|Callby: parse_fnm
|Purpose: Prevents backup of a file. A one-shot routine.
|Arguments:
|	none
\******************************************************************************/
void set_output_nobak()
{
	specialnobak = 1;
}

/******************************************************************************\
|Routine: output_file
|Callby: command
|Purpose: Writes a record queue (or a select range) out to a disk file. Returns
|         the number of records written, or -1 if an error occurs.
|Arguments:
|    name is the name of the file to create. If NULL, use name of window.
|    window is the number of the window to write out.
\******************************************************************************/
Int output_file(name,window)
Char *name;
Int window;
{
	static Char *addon = (Char *)".BAK";
	static Char *eof = "\r\n";

	rec_ptr r,base;
	FILE *fp;
	Int fd,statval;
	Int count,i,j,bytes,stoploop,ftpsystem;
	Char *p,c,bakname[512],fname[512],tmpname[512],host[512],usr[512],command[512],lbuf[65536];
	buf_node buf,*recbuf;
	struct stat statbuf;
#ifdef VMS
	Char binarybuf[512];
	Int remain;
#endif

	buf.first = (rec_ptr)&buf.first;	/* initialize buffer */
	buf.last = (rec_ptr)&buf.first;
	recbuf = &buf;
	strcpy(fname,(name)? name: WINDOW[window].filename);
	if((ftpsystem = ftp_system(fname)) == 'V')
		if((p = strchr(fname,';')))
			*p = '\0';
#ifndef NO_FTP
	if((i = host_in_name(fname)))
	{
		p = extract_host(i,fname,host,usr);
		if(!ftp_open(host,usr))
			return(-1);
		sprintf(command,"TYPE %c",(WINDOW[window].binary)? 'i' : 'a');
		if(!ftp_command(command))
			return(-1);
		if(ftpsystem == 'I')
			p = strchr(p,']') + 1;
		sprintf(command,"STOR %s",p);
		if(!ftp_data(command))
		{
			ftp_command("TYPE a");
			return(-1);
		}
		if(!SELREC)	/* write entire buffer for window */
			r = base = WINDOW[window].base;
		else	/* write select range */
		{
			output_buffer(recbuf);
			r = base = (rec_ptr)&recbuf->first;
		}
		count = 0;
		if(WINDOW[window].binary)
		{
			while((r = r->next) != base)
				if(r->length)
				{
					if(!ftp_put_binary(r->data,r->length))
						break;
					count += r->length;
				}
			ftp_put_binary(r->data,0);
		}
		else
		{
			while((r = r->next) != base)
			{
				if(!r->length)
				{
					if(!ftp_put_record(eof))
						break;
				}
				else
				{
					strcpy(lbuf,r->data);
					strcat(lbuf,eof);
					if(!ftp_put_record(lbuf))
						break;
				}
				count++;
			}
			ftp_put_record(NULL);
		}
		buffer_empty(recbuf);
		ftp_command("TYPE a");
		if(r != base)
			return(-1);
		else
			return((WINDOW[window].binary)? ((count + 511) >> 9) : count);
	}
#endif	/* not NO_FTP */
	strcpy(tmpname,fname);
	envir_subs(tmpname);
	*(filename(tmpname)) = '\0';
	strcat(tmpname,"__out__.tmp");
	statval = stat(fname,&statbuf);	/* we must always call stat() so we can chmod below */
	if(backupfiles && !specialnobak)
		if(!statval)	/* stat will return 0 if the file exists and is stat-able */
		{
			if(!strlen(backupdir))
			{
				strcpy(bakname,fname);
				p = (Char *)extension(bakname);
				if(!p || p == bakname)	/* turn .cshrc into .cshrc.BAK */
					strcat(bakname,addon);
				else
				{
					c = *(p - 1);	/* check for /x/z/.cshrc sort of thing */
					if(c == '/' || c == '\\')
						strcat(bakname,addon);
					else
						strcpy(p,addon);
				}
			}
			else
			{
				strcpy(bakname,backupdir);
				strcat(bakname,filename(fname));
			}
			stoploop = 0;
retrybackup:
			if(rename(fname,bakname))
			{
				if(errno == EXDEV)	/* rename() to different device, copy the file instead */
				{
					if((p = map_file(fname,&bytes)))
					{
#ifdef VMS
						fd = creat(bakname,0,"ctx=rec","rfm=fix","fop=mxv","mrs=512","shr=nil");
#else
#ifdef GNUDOS
						fd = open(bakname,O_RDWR | O_CREAT | O_TRUNC | O_BINARY,S_IWRITE | S_IREAD);
#else
						fd = open(bakname,O_RDWR | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif
#endif
						if(fd >= 0)
						{
							if(write(fd,p,bytes) == bytes)
							{
								close(fd);
								unmap_file(p);
								goto do_output;
							}
							close(fd);
						}
						unmap_file(p);
					}
				}
				else if(errno == ENOENT && !stoploop)	/* directory doesn't exist? */
				{
					stoploop = 1;
					strcpy(lbuf,backupdir);
					if(*(p = lbuf + strlen(lbuf) - 1) == '/')
						*p = '\0';
					if(!mkdir(lbuf,0x1ff))
						goto retrybackup;
				}
				if(strlen(backupdir))
				{
					sprintf(lbuf,"Unable to rename the existing file to %s.\n",backupdir);
					slip_message(lbuf);
				}
				else
					slip_message("Unable to rename that file to .BAK file.");	/* don't let them create output file */
				wait_message();
				return(-1);
			}
		}
	specialnobak = 0;
do_output:
	if(WINDOW[window].binary)
	{
#ifdef VMS
		if((fd = creat(fname,0,"ctx=rec","rfm=fix","fop=mxv","mrs=512","shr=nil")) >= 0)
			goto create_ok;
#else
		if(!strcmp(fname," Standard Input "))
		{
			fd = -1;
			goto create_ok;
		}
		else
#ifdef GNUDOS
			if((fd = open(tmpname,O_RDWR | O_CREAT | O_TRUNC | O_BINARY,S_IWRITE | S_IREAD)) >= 0)
#else
			if((fd = open(tmpname,O_RDWR | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) >= 0)
#endif
				goto create_ok;
#endif	/* VMS */
	}
	else
	{
#ifdef VMS
		if((fp = fopen(fname,"w","ctx=rec","rfm=var","rat=cr","shr=nil","fop=mxv")))
			goto create_ok;
#else
		if(!strcmp(fname," Standard Input "))
		{
			fp = stdout;
			goto create_ok;
		}
		else
			if((fp = fopen(tmpname,"w")))
				goto create_ok;
#endif	/* VMS */
	}
/* unable to create output file */
	slip_message("Unable to create output file.");
	wait_message();
	return(-1);
create_ok:
	if(!SELREC)	/* write entire buffer for window */
		r = base = WINDOW[window].base;
	else	/* write select range */
	{
		output_buffer(recbuf);
		r = base = (rec_ptr)&recbuf->first;
	}
/* write data to output file */
	count = 0;
	if(WINDOW[window].binary)
	{
#ifdef VMS
		remain = 512;
#endif
		while((r = r->next) != base)
		{
#ifdef VMS
			if(i = r->length)
			{
				if(i <= remain)
				{
					memcpy(binarybuf + 512 - remain,r->data,i);
					remain -= i;
				}
				else
				{
					p = r->data;
					if(remain)
						memcpy(binarybuf + 512 - remain,p,remain);
					if(fd < 0)
						for(j = 0;j < 512;j++)
							putchar(binarybuf[j]);
					else
						if(write(fd,binarybuf,512) != 512)
							goto write_err;
					p += remain;
					i -= remain;
					while(i >= 512)
					{
						memcpy(binarybuf,p,512);
						if(fd < 0)
							for(j = 0;j < 512;j++)
								putchar(binarybuf[j]);
						else
							if(write(fd,binarybuf,512) != 512)
								goto write_err;
						p += 512;
						i -= 512;
					}
					if(i)
						memcpy(binarybuf,p,i);
					remain = 512 - i;
				}
			}
#else
			if(r->length)
				if(fd < 0)
					for(j = 0;j < r->length;j++)
						putchar(r->data[j]);
				else
					if(write(fd,r->data,r->length) != r->length)
						goto write_err;
#endif
			count += r->length;
		}
#ifdef VMS
		if(remain < 512)
		{
			if(remain)
				memset(binarybuf + 512 - remain,0,remain);
			if(fd < 0)
				for(j = 0;j < 512;j++)
					putchar(binarybuf[j]);
			else
				if(write(fd,binarybuf,512) != 512)
					goto write_err;
		}
#endif
		if(fd >= 0)
			close(fd);
	}
	else
	{
		while((r = r->next) != base)
		{
			if(!r->length)
			{
				if(fprintf(fp,"\n") < 0)
					goto write_err;
			}
			else
			{
				for(i = 0;i < r->length;i++)
					if(!r->data[i])
						break;
				if(i == r->length)
				{
					if(fprintf(fp,"%s\n",r->data) < 0)
						goto write_err;
				}
				else
				{
					for(i = 0;i < r->length;i++)
						if(fprintf(fp,"%c",r->data[i]) < 0)
							goto write_err;
					if(fprintf(fp,"\n") < 0)
						goto write_err;
				}
			}
			count++;
		}
		fclose(fp);
	}
	if(rename(tmpname,fname))
		remove(tmpname);
	buffer_empty(recbuf);
	if(!statval)
		chmod(fname,statbuf.st_mode);
	if(r != base)
		return(-1);
	else
		return((WINDOW[window].binary)? ((count + 511) >> 9) : count);
write_err:
	bell();
	if(WINDOW[window].binary)
	{
		close(fd);
		if(fd >= 0)
			remove(tmpname);
	}
	else
	{
		fclose(fp);
		if(fp != stdout)
			remove(tmpname);
	}
	buffer_empty(recbuf);
	slip_message("I am unable to create the output file. Perhaps the disk is full.");
	wait_message();
	return(-1);
}

