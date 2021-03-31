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
#include <limits.h>
#include "file.h"
#ifdef NeXT
#include <sys/dir.h>
#else
#ifndef VMS
#ifdef DIRENT_PC
#include "direntpc.h"
#else
#include <dirent.h>
#endif
#endif
#endif

#include "memory.h"
#include "ctyp_dec.h"

#define CHARLES 0x79861876	/* magic number */

#ifdef VMS
typedef struct chardesc {unsigned short length,class;Char *string;} desc_node;
typedef struct chardesc *desc;
static Char wildcard[512],lastmatch[512];
#else
typedef struct namelist_str *namelist_ptr;
typedef struct namelist_str
{
	namelist_ptr next;
	Char *name;
} namelist_node;

typedef struct ff_str *ff_ptr;
typedef struct ff_str
{
	Int magic,matchcount;
	namelist_ptr firstname,curname;	/* these are loaded during scan. when user traverses, curname handles it. */
} ff_node;
#endif

/******************************************************************************\
|Routine: match_wild
|Callby: find_files
|Purpose: Provides VMS-style matching of file names to a template. Returns 1 on a match, else 0.
|Arguments:
|	template is the template string, which may contain * and % wildcards.
|	string is the string we are comparing to the template.
\******************************************************************************/
Int match_wild(template,string)
Char *template,*string;
{
	register Int remain_t,remain_s;
	register Char *t,*s,c;
	Int save_r_s,save_r_t;
	Char *save_t,*save_s;
#ifdef GNUDOS
	register Char sc;
#endif

	t = template;
	s = string;
	remain_t = strlen(t);
	remain_s = strlen(s);
	save_r_s = 0;
	while(1)
	{
		if(--remain_t < 0)
		{
			if(!remain_s)
				return(1);
		}
		else
		{
			if((c = *t++) == '*')
			{
				if(*t == '*')
				{
					t++;
					remain_t--;
				}
				else
				{
					if(!remain_t)
						return(1);
					save_s = s;
					save_t = t;
					save_r_s = remain_s;
					save_r_t = remain_t;
					continue;
				}
			}
			if(--remain_s < 0)
				return(0);
#ifdef GNUDOS
			sc = *s++;
			if(tolower(c) == tolower(sc))
#else
			if(c == *s++)
#endif
				continue;
			if(c == '?')
				continue;
		}
		if(--save_r_s < 0)
			return(0);
		save_s++;
		s = save_s;
		t = save_t;
		remain_s = save_r_s;
		remain_t = save_r_t;
	}
}

#ifndef VMS

/******************************************************************************\
|Routine: got_string
|Callby: find_files
|Purpose: Makes a buffer and copies a string into it.
|Arguments:
|	string is the string to be copied.
\******************************************************************************/
Char *got_string(string)
Char *string;
{
	Char *p;
	
	p = (Char *)imalloc(((string)? strlen(string) : 0) + 1);
	if(string)
		strcpy(p,string);
	else
		*p = '\0';
	return(p);
}

/******************************************************************************\
|Routine: ff_report_name
|Callby: find_files
|Purpose: Stores a validated, matching file name in dynamic memory.
|Arguments:
|	ff is the header of the (possibly empty) list of already-validated names.
|	name is the new name to append to the list.
\******************************************************************************/
void ff_report_name(ff,name)
ff_ptr ff;
Char *name;
{
	namelist_ptr p;
	Char *from,*to,c;
	
	if(!ff->firstname)
		ff->firstname = ff->curname = (namelist_ptr)&ff->firstname;
	p = ff->curname->next = (namelist_ptr)imalloc(sizeof(namelist_node));
	ff->curname = p;
	p->next = NULL;
	p->name = got_string(name);
	for(from = to = p->name;(c = *from++);)	/* eliminate double /s */
		if(c == '/')
		{
			if(*from != '/')
			{
				if(*from == '.' && *(from + 1) == '/')
				{
					*to++ = '/';
					from += 2;
				}
				else
					*to++ = c;
			}
		}
		else
			*to++ = c;
	*to = '\0';
	ff->matchcount++;
}

/******************************************************************************\
|Routine: ff_scan_file
|Callby: find_files
|Purpose: Generates a complete list of matching file names (in inode order).
|Arguments:
|	ff is the list header structure.
|	wild is a file specification possibly containing * or ? or ... wildcards.
\******************************************************************************/
void ff_scan_file(ff,wild)
ff_ptr ff;
Char *wild;
{
	Char *wil,wildcard;
	Char *pred,*foll,*full,*temp,*etc,*dirmatch,*temp2;
	Int lpred,lfoll,lname,letc;
	Char *p,*q;
	DIR *dp;	/* directory pointer */
	struct stat statbuf;
#ifdef NeXT
	struct direct *d;
#else
	struct dirent *d;
#endif
	
/* Algorithm:
 *
 *	if no wildcards:
 *		return(!stat(file))
 *	else if wild = /.../:
 *		find preceeding /: pred/.../foll
 *		if pred is null, pred = / (or /. if GNUDOS)
 *		opendir(pred), scan=pfile:
 *			if pfile is a directory:
 *				recurse with /pred/pfile/.../foll
 *				if foll contains /: folldir/folletc
 *					if pfile matches folldir
 *						recurse with pred/pfile/folletc
 *			else
 *				if foll does not contain /
 *					if pfile matches foll
 *						return a hit
 *	else
 *		find preceeding /: pred/foll
 *		if foll contains /:
 *			foll divides into dirmatch/etc
 *			if pred is null, pred = / (or /. if GNUDOS)
 *			opendir(pred), scan=pfile
 *				if pfile is a directory:
 *					if pfile matches dirmatch
 *						recurse with pred/pfile/foll
 *		else
 *			if pred is null, pred = / (or /. if GNUDOS)
 *			opendir(pred), scan=pfile
 *				if pfile matches foll
 *					return a hit
 */		
	wil = got_string(wild);	/* make a modifiable copy of the string */
	for(p = wil;*p;p++)
		if(*p == '*' || *p == '?' || !strncmp(p,"/.../",5))
			break;
	if(!*p)
	{
		if(!stat(wild,&statbuf))
			ff_report_name(ff,wild);
		ifree(wil);
		return;
	}
	if(*p == '/')	/* save the type of wildcard */
		p++;	/* advance to first . if it's a /.../ */
	wildcard = *p;
	while(--p != wil)	/* find the / preceeding the wildcard */
		if(*p == '/')
			break;
	*p++ = '\0';
	pred = got_string((*wil)? wil : (Char *)"/");	/* null predecessor means open root directory for scan */
	if(wildcard == '.')
	{
		while(*p++ != '/');	/* find the other end of the /.../ */
		foll = got_string((*p)? p : (Char *)"*");
		if((dp = opendir(pred)))
		{
			lpred = strlen(pred);
			lfoll = strlen(foll);
			while((d = readdir(dp)))	/* read entries */
			{
#ifndef GNUDOS
				if(!d->d_ino)	/* skip empty entries */
					continue;
#endif
				if(!strcmp(d->d_name,".") || !strcmp(d->d_name,".."))	/* ignore . and .. */
					continue;
/* build pred/name */
				full = (Char *)imalloc(lpred + 1 + (lname = strlen(d->d_name)) + 1);
				memcpy(full,pred,lpred);
				p = full + lpred;
				*p++ = '/';
				strcpy(p,d->d_name);
				if(!stat(full,&statbuf))	/* see if the file found is a directory */
				{
					if(S_ISDIR(statbuf.st_mode))
					{
						temp = (Char *)imalloc(lpred + 1 + lname + 5 + lfoll + 1);
						strcpy(temp,full);
						strcat(temp,"/.../");
						strcat(temp,foll);
						ff_scan_file(ff,temp);
						ifree(temp);
						if((p = (Char *)strchr(foll,'/')))
						{
							temp2 = got_string(foll);
							temp2[p - foll] = '\0';
							if(match_wild(temp2,d->d_name))
							{
								etc = got_string((*p)? p : (Char *)"*");
								temp = (Char *)imalloc(lpred + 1 + lname + 1 + strlen(etc) + 1);
								strcpy((q = temp),pred);
								q += lpred;
								*q++ = '/';
								strcpy(q,d->d_name);
								q += lname;
								*q++ = '/';
								strcpy(q,etc);	/* what if p is null string? */
								ff_scan_file(ff,temp);
								ifree(temp);
								ifree(etc);
							}
							ifree(temp2);
						}
					}
					else	/* it isn't a directory */
					{
						if(!strchr(foll,'/'))
						{
							if(match_wild(foll,d->d_name))
							{
								temp = (Char *)imalloc(lpred + 1 + lname + 1);
								strcpy((p = temp),pred);
								p += lpred;
								*p++ = '/';
								strcpy(p,d->d_name);
								ff_report_name(ff,temp);
								ifree(temp);
							}
						}
					}
				}	/* end of stat() block */
				ifree(full);
			}
			closedir(dp);
		}	/* end of opendir() block */
		ifree(foll);
	}
	else	/* wildcard is not /.../ */
	{
		foll = got_string((*p)? p : (Char *)"*");	/* p points past preceeding / */
		if((p = (Char *)strchr(foll,'/')))
		{
			*p++ = '\0';
			dirmatch = got_string(foll);
			etc = got_string((*p)? p : (Char *)"*");
			letc = strlen(etc);
			if((dp = opendir(pred)))
			{
				lpred = strlen(pred);
				while((d = readdir(dp)))	/* read entries */
				{
#ifndef GNUDOS
					if(!d->d_ino)	/* skip empty entries */
						continue;
#endif
					if(!strcmp(d->d_name,".") || !strcmp(d->d_name,".."))	/* ignore . and .. */
						continue;
/* build pred/name */
					full = (Char *)imalloc(lpred + 1 + (lname = strlen(d->d_name)) + 1);
					memcpy(full,pred,lpred);
					p = full + lpred;
					*p++ = '/';
					memcpy(p,d->d_name,lname);
					p += lname;
					*p = '\0';
					if(!stat(full,&statbuf))	/* see if the file found is a directory */
					{
						if(S_ISDIR(statbuf.st_mode))
						{
							if(match_wild(dirmatch,d->d_name))
							{
								temp = (Char *)imalloc(lpred + 1 + lname + 1 + letc + 1);
								strcpy((q = temp),pred);
								q += lpred;
								*q++ = '/';
								strcpy(q,d->d_name);
								q += lname;
								*q++ = '/';
								strcpy(q,etc);
								ff_scan_file(ff,temp);
								ifree(temp);
							}
						}	/* if it isn't a directory, we are not interested */
					}
					ifree(full);
				}
				closedir(dp);
			}
			ifree(etc);
			ifree(dirmatch);
		}
		else	/* foll contains no / */
		{
			if((dp = opendir(pred)))
			{
				lpred = strlen(pred);
				while((d = readdir(dp)))	/* read entries */
				{
#ifndef GNUDOS
					if(!d->d_ino)	/* skip empty entries */
						continue;
#endif
					if(!strcmp(d->d_name,".")) /* || !strcmp(d->d_name,"..")) */
						continue;  		/* Add this and .. not in dired */
/* build pred/name */
					if(match_wild(foll,d->d_name))
					{
						full = (Char *)imalloc(lpred + 1 + (lname = strlen(d->d_name)) + 1);
						memcpy(full,pred,lpred);
						p = full + lpred;
						*p++ = '/';
						memcpy(p,d->d_name,lname);
						p += lname;
						*p = '\0';
						ff_report_name(ff,full);
						ifree(full);
					}
				}
				closedir(dp);
			}
		}	/* end of foll-contains-/ ifelse block */
		ifree(foll);
	}
	ifree(wil);
}
#endif
	
/******************************************************************************\
|Routine: find_files
|Callby: cfg file_list_i load_file
|Purpose: To initialize a wildcard search. Returns a nonzero context value
|         if successful, else zero.
|Arguments:
|	wild is the wildcard filename string.
\******************************************************************************/
Long find_files(wild)
Char  *wild;
{
#ifdef VMS
	desc_node a,b;
	Int status;
	Long context;
	Char *p,c;
	Char ret[512];

	strcpy(wildcard,wild);
	a.length = strlen(wildcard);
	a.class = 0x010e;
	a.string = wildcard;
	b.length = 128;
	b.class = 0x010e;
	b.string = ret;
	context = 0;
	if((status = lib$find_file(&a,&b,&context)) & 1)
	{
		if((p = strchr(ret,' ')))
			*p = '\0';
		strcpy(lastmatch,ret);
	}
	else
	{
		lastmatch[0] = '\0';
		context = 0;
	}
	return(context);
#else
	ff_ptr ff;
	Char *wil,*q;
	Char **tags;
	namelist_ptr p;
	Int n;
#ifdef GNUDOS
	Char c,*newwil,*w;
#endif

	wil = got_string(wild);
#ifdef GNUDOS
/*	Check for 2nd char ':' (if so, device letter 1st char, directory after)
 *	if directory spec starts with /, add /. to front
 *	else if directory spec starts with non ../ or ./ add ./
 */
	for(q = wil;(c = *q);q++)
		if(c == '\\')	/* convert \ to / */
			*q = '/';
	w = wil;
	newwil = (Char *)imalloc(strlen(wil) + 3);	/* make buffer for fixed-up file name */
	q = newwil;
	if(*(w + 1) == ':')	/* copy the device spec if present */
	{
		*q++ = *w++;
		*q++ = *w++;
	}
	*q = '\0';	/* terminate the device spec */
	if(*w == '/')		/* if the first char is /, prepend /. */
		strcat(q,"/.");
	else if(!(			/* if the first char is not ./ or ../, prepend ./ */
		(*w == '.' && *(w + 1) == '/') || 
		(*w == '.' && *(w + 1) == '.' && *(w + 2) == '/')
		))
		strcat(q,"./");
	strcat(q,w);
	ifree(wil);
	wil = newwil;
#else
	wil = got_string(wild);
	if(!(
		*wil == '/' || 
		(*wil == '.' && *(wil + 1) == '/') || 
		(*wil == '.' && *(wil + 1) == '.' && *(wil + 2) == '/')
		))	/* if the first char is not /, prepend ./ */
	{
		q = (Char *)imalloc(strlen(wil) + 3);
		strcpy(q,"./");
		strcat(q,wil);
		ifree(wil);
		wil = q;
	}
#endif
	ff = (ff_ptr)imalloc(sizeof(ff_node));
	ff->magic = CHARLES;
	ff->matchcount = 0;
	ff->firstname = NULL;
	ff_scan_file(ff,wil);
	ifree(wil);
/* sort namelist */
	if(ff->matchcount)
	{
		tags = (Char **)imalloc((ff->matchcount + 1) * sizeof(Char *));
		for(n = 0,p = ff->firstname;p;p = p->next)
			tags[n++] = p->name;
		cqsort_p(n,tags);
		for(n = 0,p = ff->firstname;p;p = p->next)
			p->name = tags[n++];
		ifree(tags);
	}
	ff->curname = ff->firstname;
	return((Long)ff);
#endif
}

/******************************************************************************\
|Routine: next_file
|Callby: cfg file_list_i load_file
|Purpose: Returns the next matching file name. Returns 1 on success, else zero.
|Arguments:
|	context is a context value previously returned by find_files.
|	retbuf gets the returned file name.
\******************************************************************************/
Int next_file(context,retbuf)
Long context;
Char *retbuf;
{
#ifdef VMS
	desc_node a,b;
	Char *p,c;

	if(lastmatch[0] == '\0')
		return(0);
	strcpy(retbuf,lastmatch);
	a.length = strlen(wildcard);
	a.class = 0x010e;
	a.string = wildcard;
	b.length = 128;
	b.class = 0x010e;
	b.string = lastmatch;
	if(lib$find_file(&a,&b,&context) & 1)
	{
		if((p = strchr(lastmatch,' ')))
			*p = '\0';
	}
	else
		lastmatch[0] = '\0';
	return(1);
#else
	namelist_ptr p;
	ff_ptr ff;
	
	ff = (ff_ptr)context;
	if(ff->magic != CHARLES)
	{
		retbuf[0] = '\0';
		return(0);
	}
	if(!(p = ff->curname))
	{
		ff->curname = ff->firstname;
		retbuf[0] = '\0';
		return(0);
	}
	ff->curname = p->next;
	strcpy(retbuf,p->name);
	return(1);
#endif
}

/******************************************************************************\
|Routine: find_files_end
|Callby: cfg file_list_i load_file
|Purpose: To clean up after find_file() is called.
|Arguments:
|	context is the context value used by find_file().
\******************************************************************************/
void find_files_end(context)
Long context;
{
#ifdef VMS
	if(context)
		lib$find_file_end(&context);
#else
	ff_ptr ff;
	namelist_ptr p,q;
	
	if((ff = (ff_ptr)context))
		if(ff->magic == CHARLES)
		{
			for(p = ff->firstname;p;)
			{
				q = p -> next;
				ifree(p->name);
				ifree(p);
				p = q;
			}
			ifree(ff);
		}
#endif
}

