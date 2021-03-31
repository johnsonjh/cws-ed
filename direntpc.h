/*
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

#ifdef WIN32
/* This provides opendir,readdir,closedir function subset for ED in NT */
#include <windows.h>

struct dirent {
	char d_name[256];
};

typedef struct direct {
	char d_name[256];
	int d_first;
	HANDLE d_hFindFile;
} DIR;

DIR *opendir(n)
char *n;						/* directory to open */
{
	DIR *d;						/* malloc'd return value */
	char *p;					/* malloc'd temporary string */
	WIN32_FIND_DATA fd;

	d = (DIR *)imalloc(sizeof(DIR));
	p = (Char *)imalloc(strlen(n) + 5);
	strcpy(p,n);
	strcat(p,"/*.*");
	d->d_hFindFile = FindFirstFile(p,&fd);
	ifree(p);
	if(d->d_hFindFile == INVALID_HANDLE_VALUE)
	{
		ifree(d);
		return NULL;
	}
	strcpy(d->d_name,fd.cFileName);
	d->d_first = 1;
	return d;
}

struct dirent *readdir(d)
DIR *d;							 /* directory stream to read from */
/* Return pointer to first or next directory entry, or NULL if end. */
{
	WIN32_FIND_DATA fd;

	if(d->d_first)
		d->d_first = 0;
	else if(!FindNextFile(d->d_hFindFile,&fd))
		return NULL;
	else
		strcpy(d->d_name,fd.cFileName);
	return (struct dirent *)d;
}

int closedir(d)
DIR *d;
{
	int i;

	i = FindClose(d->d_hFindFile);
	ifree(d);
	return i;
}
#endif
