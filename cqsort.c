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

#include "unistd.h"
#include "memory.h"
#include "ctyp_dec.h"
#include "mxch_dec.h"

/******************************************************************************\
|Routine: random_key
|Callby: cqsort_z_sub cqsort_zz_sub
|Purpose: Returns a random value from a range.
|Arguments:
|	l,r are the left and right ends of the sorting region.
\******************************************************************************/
Int random_key(l,r)
Int l,r;
{
	return(l + (rand() % (r - l + 1)));
}

/******************************************************************************\
|Routine: find_infinity
|Callby: main
|Purpose: Loads largest with that value that strcmp considers the largest.
|Arguments:
|	none
\******************************************************************************/
void find_infinity()
{
	Int i,j;
	Char s1[2],s2[2];
	static Char tryout[3] = {CHAR_MAX,SCHAR_MAX,UCHAR_MAX};
	
	s1[1] = s2[1] = '\0';
	for(i = 0;i < sizeof(tryout);i++)
	{
		s2[0] = tryout[i];
		for(j = 0;j < 256;j++)
		{
			s1[0] = j;
			if(strcmp(s1,s2) > 0)
				break;
		}
		if(j == 256)
		{
			largest = tryout[i];
			return;
		}
	}
	printf("I was unable to determine the largest Char value.\n");
	exit(0);
}
	
/******************************************************************************\
|Routine: cqsort_z_sub
|Callby: cqsort
|Purpose: Performs a quicksort on file names.
|Arguments:
|	tag is an array of pointers to the strings. tag is what actually gets sorted.
|	l is the index in tag of the leftmost end of the portion of it to be sorted.
|	r is the index of the rightmost end.
\******************************************************************************/
void cqsort_z_sub(tag,l,r)
register Char **tag;
register Int l;
register Int r;
{
	register Char *key,*temp;
	register Int save_r,k;

	if(r - l < 2)
	{
		if(strcmp((Char *)tag[l],(Char *)tag[r]) > 0)
		{
			temp = tag[l];
			tag[l] = tag[r];
			tag[r] = temp;
		}
	}
	else
	{
		key = tag[k = random_key(l,r)];
		tag[k] = tag[l];
		tag[l] = key;
		k = l;
		save_r = r++;
		while(1)
		{
			while(strcmp(tag[++l],key) < 0);
			while(strcmp(tag[--r],key) > 0);
			if(l < r)
			{
				temp = tag[l];
				tag[l] = tag[r];
				tag[r] = temp;
			}
			else
				break;
		}
		temp = tag[k];
		tag[k] = tag[r];
		tag[r] = temp;
		if(r - k > 1)
			cqsort_z_sub(tag,k,r - 1);
		if(save_r - r > 1)
			cqsort_z_sub(tag,r + 1,save_r);
	}
}

/******************************************************************************\
|Routine: cqsort_p
|Callby: find_files
|Purpose: Quicksorts an array of pointers to strings.
|Arguments:
|	nelem is the number of elements in the tag array.
|	tag is the array of strings.
\******************************************************************************/
void cqsort_p(nelem,tag)
Int nelem;
Char **tag;
{
	Char inf[256];

	memset(inf,largest,255);
	inf[sizeof(inf) - 1] = 0;
	tag[nelem] = inf;
	cqsort_z_sub(tag,0,nelem - 1);
}

/******************************************************************************\
|Routine: cqsort_zz_sub
|Callby: cqsort
|Purpose: Performs a quicksort on character pointers.
|Arguments:
|	tag is an array of pointers to the string pointers.
|	l is the index in tag of the leftmost end of the portion of it to be sorted.
|	r is the index of the rightmost end.
\******************************************************************************/
void cqsort_zz_sub(tag,l,r)
register Char ***tag;
register Int l;
register Int r;
{
	register Char *key,**temp;
	register Int save_r,k;

	if(r - l < 2)
	{
		if(strcmp((Char *)*tag[l],(Char *)*tag[r]) > 0)
		{
			temp = tag[l];
			tag[l] = tag[r];
			tag[r] = temp;
		}
	}
	else
	{
		temp = tag[k = random_key(l,r)];
		tag[k] = tag[l];
		tag[l] = temp;
		key = *tag[k = l];
		save_r = r++;
		while(1)
		{
			while(strcmp(*tag[++l],key) < 0);
			while(strcmp(*tag[--r],key) > 0);
			if(l < r)
			{
				temp = tag[l];
				tag[l] = tag[r];
				tag[r] = temp;
			}
			else
				break;
		}
		temp = tag[k];
		tag[k] = tag[r];
		tag[r] = temp;
		if(r - k > 1)
			cqsort_zz_sub(tag,k,r - 1);
		if(save_r - r > 1)
			cqsort_zz_sub(tag,r + 1,save_r);
	}
}

/******************************************************************************\
|Routine: cqsort_s
|Callby: sort_recs
|Purpose: Quicksorts an array of pointers to strings, returning a tag array.
|Arguments:
|	nelem is the number of elements in the tag array.
|	data is the pointer to the key in the first structure.
|	size is the size of each structure in the data array.
\******************************************************************************/
Int *cqsort_s(nelem,data,size)
Int nelem;
Char *data;
Int size;
{
	Char ***tag,inf[256];
	Char *a,**base;
	Int i,*itag;

	tag = (Char ***)imalloc((nelem + 1) * sizeof(Char **));
	a = data;
	base = (Char **)a;
	for(i = 0;i <= nelem;a += size)	/* initialize the tags */
		tag[i++] = (Char **)a;
	memset(inf,largest,255);
	inf[sizeof(inf) - 1] = 0;
	a = inf;
	tag[nelem] = &a;	/* set infinite key */
	cqsort_zz_sub(tag,0,nelem - 1);
	itag = (Int *)tag;				/* Assumes sizeof(Int) <= sizeof(void *) */
	for(i = 0;i < nelem;i++)
		itag[i] = (Int)(((Char *)tag[i] - (Char *)base) / size);
	return(itag);
}

