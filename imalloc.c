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
#include <stddef.h>

#include "rec.h"
#include "window.h"
#include "ed_dec.h"

#define GETSIZE 131584	/* NOTE: GETSIZE must be a multiple of 512 */

typedef struct free_str *free_ptr;
typedef struct free_str
{
	Int size;
	free_ptr next,prev;
	Int data[1];
} free_node;

typedef struct zone_str *zone_ptr;
typedef struct zone_str
{
	zone_ptr next,prev;
	free_ptr first,last,current;
	Int data[1];
} zone_node;

/******************************************************************************\
|Routine: zone_size
|Callby: imalloc
|Purpose: Returns the size of a zone that will hold a given number of bytes.
|Arguments:
|	size is the size of the largest imalloc anticipated.
\******************************************************************************/
size_t zone_size(size)
size_t size;
{
	if(size > GETSIZE - sizeof(zone_node) - sizeof(free_node))
		return((size + sizeof(zone_node) + sizeof(free_node) + 511) & ~511);
	else
		return(GETSIZE);
}

/******************************************************************************\
|Routine: ifree
|Callby: buffer_empty cfrendly command edit express find_files get_colbyt init_term inquire killer load_file load_key main rec_insert rec_merge rec_trim remove_win slip_message sort_recs toss_data
|Purpose: To free got memory.
|Arguments:
|	block is the address of a previously got block.
\******************************************************************************/
void ifree(block)
void *block;
{
	register free_ptr this,other,p,n;
	register zone_ptr z;
	register Int size,qok;
	
#ifdef NO_SBRK
	free(block);
	return;
#else
	qok = 0;
	this = (free_ptr)((Int *)block - 2);
	z = (zone_ptr)this->next;	/* the zone header for the zone this block lies in */
	if((size = this->data[-4]) > 0)	/* coalesce with predecessor */
	{
		other = (free_ptr)((Int *)this - size - 3);
		other->size += 3 - this->size;
		if(this == z->current)
			z->current = other;
		this = other;
		qok = 1;
	}
	else
		this->size = -this->size;
	other = (free_ptr)&this->data[this->size];
	if((size = other->size) > 0)	/* coalesce with follower */
	{
		n = other->next;
		p = other->prev;
		if(qok)	/* the block-of-interest is already in the queue due to coalescence, just remove follower from queue */
		{
			n->prev = p;
			p->next = n;
		}
		else	/* the block-of-interest is not in the queue, make it replace the follower */
		{
			this->next = n;
			this->prev = p;
			p->next = this;
			n->prev = this;
		}
		this->size += size + 3;
		if(other == z->current)
			z->current = this;
		qok = 1;
	}
	if(!qok)
	{
		p = z->last;
		n = (free_ptr)&z->prev;	/* prev corresponds to size in free node */
		this->next = n;
		this->prev = p;
		p->next = n->prev = this;
	}
	this->data[this->size - 1] = this->size;
	if(!z->current)
		z->current = this;
#endif
}

/******************************************************************************\
|Routine: imalloc
|Callby: buffer_app carriage_ret cfrendly command copy_buffer cqsort do_grep edit
|        express find_files get_colbyt help_get_kw help_load imalloc include_file
|        init_term inquire insert_win load_buffer load_file load_key main
|        new_window openline rec_copy rec_insert rec_merge rec_split rec_trim
|        restore_par slip_message sort_recs str_to_buf wincom
|Purpose: To get memory.
|Arguments:
|	size is the number of bytes to get.
\******************************************************************************/
void *imalloc(size)
size_t size;
{
	static zone_ptr start_zone = NULL;
	static zone_ptr scan_zone;
	static Int zonesize;
	static zone_node base;
	
	register free_ptr n,p,fit,other;
	register free_ptr scan_free,header;
	register Int actual;
	Int new,remain;
	struct {void *first,*last;} region;
	
#ifdef NO_SBRK
	void *vp;
	
	if(!(vp = (void *)malloc(size)))
	{
		move(NROW,1);
		putout();
		printf("\r\n\nRan out of memory, sorry.\r\n");
		cleanup(-1);
	}
	return(vp);
#else
/* calculate the required size of the storage area, in ints, minimum 2 */
	if((actual = (size + 3) >> 2) < 1)
		actual = 1;
	if(!(scan_zone = start_zone))
/* on first call, initialize the zone queue header */
	{
		base.next = &base;
		base.prev = &base;
		zonesize = (sizeof(zone_node) + sizeof(free_node))/sizeof(int);
	}
	else
	{
/* scan through all existing zones */
		do
		{
			if((scan_free = scan_zone->current))
			{
				header = (free_ptr)&scan_zone->prev;
/* scan until a fit is found */
				do
				{
					if((remain = scan_free->size - actual) >= 0)
					{
						fit = scan_free;
						n = fit->next;
						p = fit->prev;
						if(remain < 4)
						{
							actual = fit->size;
							p->next = fit->next;
							n->prev = fit->prev;
							if(n == p)
								scan_zone->current = NULL;
							else
							{
								if(n == header)
									n = n->next;
								scan_zone->current = n;
							}
						}
						else
						{
							scan_zone->current = other = (free_ptr)&fit->data[actual];
							other->prev = p;
							other->next = n;
							p->next = n->prev = other;
							remain -= 3;
							other->size = other->data[remain - 1] = remain;
						}
						fit->size = fit->data[actual - 1] = -actual;
						start_zone = scan_zone;
						fit->next = (free_ptr)scan_zone;	/* set zone header backpointer */
						return((void *)&fit->prev);
					}
					if((scan_free = scan_free->next) == header)	/* pass through header */
						scan_free = scan_free->next;
				} while(scan_free != scan_zone->current);
			}
			if((scan_zone = scan_zone->next) == (zone_ptr)&base)    /* pass through header */
				scan_zone = scan_zone->next;
		}while(scan_zone != start_zone);
	}
/* no space available, create new zone */
	new = zone_size(actual << 2);
#ifdef VMS
	if(!(SYS$EXPREG(new >> 9,&region,0,0) & 1))
#else
	if((region.first = (Char *)sbrk(new)) == (void *)(-1))
#endif
	{
		move(NROW,1);
		putout();
		printf("\r\n\nRan out of memory, sorry.\r\n");
		cleanup(-1);
	}
	new >>= 2;	/* convert to ints */
	start_zone = (zone_ptr)region.first;
	start_zone->next = &base;
	start_zone->prev = base.prev;
	base.prev->next = start_zone;
	base.prev = start_zone;
	start_zone->data[0] = -1;	/* used-tag for backup from first region */
	scan_free = start_zone->current = start_zone->first = start_zone->last = (free_ptr)&start_zone->data[1];
	scan_free->next = scan_free->prev = (free_ptr)&start_zone->prev;        /* zone header appears to contain a free node */
	scan_free->size = scan_free->data[new - zonesize - 1] = new - zonesize;
	scan_free->data[new - zonesize] = -1;	/* used-tag for following last region */
	return(imalloc(size));
#endif
}

