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

#include "rec.h"
#include "buffer.h"

/******************************************************************************\
|Routine: include_file
|Callby: command
|Purpose: Includes an entire file in the current window.
|Arguments:
|    filename is the name of the file to include.
|    binary is a flag indicating the file in to be read in in binary mode.
\******************************************************************************/
void include_file(filename,binary)
Char *filename;
Char binary;
{
	rec_ptr nullrec;
	buf_node buf;
	Char buffer[512],dummy[512],*filebuf,ddummy;	/* dummy[] gets bookmark file name, which is ignored. this is probably a bug. */
	Int idummy,jdummy;

	buf.first = (rec_ptr)&buf.first;
	buf.last = (rec_ptr)&buf.first;
	buf.direction = 1;
	if(!(buf.nrecs = load_file(filename,(rec_ptr)&buf.first,dummy,&idummy,&jdummy,&ddummy,binary,&filebuf)))
		return;
	if(buf.nrecs > 0)
	{
		nullrec = (rec_ptr)imalloc(sizeof(rec_node));
		nullrec->length = 0;
		nullrec->data = NULL;
		insq(nullrec,buf.last);
		buf.nrecs++;
		insert(&buf);
		buffer_empty(&buf);
		if(filebuf)
			unmap_file(filebuf);
	}
	else if(buf.nrecs == -1)
	{
		sprintf(buffer,"Unable to open the file %s.",filename);
		slip_message(buffer);
		wait_message();
	}
/* otherwise, it was an ftp error (load_file returns -2) which has already been reported to user */
}

