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

#include "memory.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"

/******************************************************************************\
|Routine: copy_records
|Callby: wincom
|Purpose: Copies the contents of an existing set of records.
|Arguments:
|    source is the header of the existing record queue.
|    base is the header of the new, copied queue.
|    binsource is a flag that the source buffer is binary.
|    bindest is a flag that the destination buffer is binary.
\******************************************************************************/
Int copy_records(source,dest,binsource,bindest)
rec_ptr source,dest;
Int binsource,bindest;
{
	register rec_ptr old,end,new;
	register Int i,j,count;
	Char buf[BINARY_RECSIZE + 1];

	dest->next = dest->prev = dest;
	count = 0;
/* source and dest are the same type */
	if(binsource == bindest)
		for(old = source->next,end = (rec_ptr)&source->next;old != end;old = old->next)
		{
			i = old->length;
			new = (rec_ptr)imalloc(sizeof(rec_node));
			insq(new,dest->prev);
			new->data = (Char *)imalloc(i + 1);
			if(i > 0)
				memcpy(new->data,old->data,i);
			new->data[i] = 0;
			new->length = i;
			new->recflags = 1;	/* it is a freeable buffer */
			count++;
		}
/* source is binary, dest is not. this never happens with the current setup, since there isn't a -notb option */
/*	else if(binsource)
	{
		for(i = 0,old = source->next,end = (rec_ptr)&source->next;old != end;old = old->next)
		{
			for(j = 0;i < sizeof(buf) && j < old->length;j++)
				if((buf[i++] = old->data[j]) == '\n')
				{
					buf[--i] = '\0';
					new = (rec_ptr)imalloc(sizeof(rec_node));
					insq(new,dest->prev);
					new->data = (Char *)imalloc(i + 1);
					if(i > 0)
						memcpy(new->data,buf,i);
					new->data[i] = 0;
					new->length = i;
					new->recflags = 1;
					count++;
					i = 0;
				}
		}
		if(i)
		{
			new = (rec_ptr)imalloc(sizeof(rec_node));
			insq(new,dest->prev);
			new->data = (Char *)imalloc(i + 1);
			if(i > 0)
				memcpy(new->data,buf,i);
			new->data[i] = 0;
			new->length = i;
			new->recflags = 1;
			count++;
		}
	}*/
/* source is not binary, dest is binary */
	else
	{
		for(i = 0,old = source->next,end = (rec_ptr)&source->next;old != end;old = old->next)
		{
			for(j = 0;j <= old->length;j++)
			{
				if(j == old->length)
					buf[i++] = '\n';
				else
					buf[i++] = old->data[j];
				if(i >= BINARY_RECSIZE)
				{
					buf[i] = '\0';
					new = (rec_ptr)imalloc(sizeof(rec_node));
					insq(new,dest->prev);
					new->data = (Char *)imalloc(BINARY_RECSIZE + 1);
					memcpy(new->data,buf,BINARY_RECSIZE);
					new->data[BINARY_RECSIZE] = 0;
					new->length = BINARY_RECSIZE;
					new->recflags = 1;	/* it is a freeable buffer */
					count++;
					i = 0;
				}
			}
		}
		if(i)
		{
			new = (rec_ptr)imalloc(sizeof(rec_node));
			insq(new,dest->prev);
			new->data = (Char *)imalloc(i + 1);
			if(i > 0)
				memcpy(new->data,buf,i);
			new->data[i] = 0;
			new->length = i;
			new->recflags = 1;	/* it is a freeable buffer */
			count++;
		}
	}
	return(count);
}

