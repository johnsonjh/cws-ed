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

#include "memory.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"


/******************************************************************************\
|Routine: express
|Callby: get_colbyt inquire paint paint_window show_param
|Purpose: Converts character data to printable data for screen display.
|         Handles tab conversion and display of control characters.
|Arguments:
|    reclen is the length of the data to be converted.
|    rec is the record containing the data to be converted.
|    tab is the tab stop map.
|    tablen is the length of the tab stop map.
|    buf is the returned buffer that receives the converted data.
|    maxlen is the maximum amount of data to be returned in buf.
|    cvttabs is a flag that makes the routine treat spaces and tabs as control
|            characters.
\******************************************************************************/
Int express(reclen,rec,tab,tablen,buf,maxlen,cvttabs)
Int reclen,tablen,maxlen,cvttabs;
Char *rec,*tab,**buf;
{
	static Char *bufp = NULL;
	static Int bufl = 0;
	static Char *hexstring = (Char *)"\
000102030405060708090a0b0c0d0e0f\
101112131415161718191a1b1c1d1e1f\
202122232425262728292a2b2c2d2e2f\
303132333435363738393a3b3c3d3e3f\
404142434445464748494a4b4c4d4e4f\
505152535455565758595a5b5c5d5e5f\
606162636465666768696a6b6c6d6e6f\
707172737475767778797a7b7c7d7e7f\
808182838485868788898a8b8c8d8e8f\
909192939495969798999a9b9c9d9e9f\
a0a1a2a3a4a5a6a7a8a9aaabacadaeaf\
b0b1b2b3b4b5b6b7b8b9babbbcbdbebf\
c0c1c2c3c4c5c6c7c8c9cacbcccdcecf\
d0d1d2d3d4d5d6d7d8d9dadbdcdddedf\
e0e1e2e3e4e5e6e7e8e9eaebecedeeef\
f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";

	register Char *r,*t,*b,*end,*tabend,c;
	register Int l,i;
	register Char *p;
	Char *tmp;

	if(!bufp)
		bufp = (Char *)imalloc(bufl = 8192);
	if(!maxlen)
		maxlen = bufl;
	if(maxlen > bufl)
	{
		while(bufl < maxlen)
			bufl <<= 1;
		ifree(bufp);
		bufp = (Char *)imalloc(bufl);
	}
	r = rec;
	t = tab;
	b = bufp;
	end = b + bufl - 1;
	tabend = t + tablen;
	l = reclen;
	while(l > 0)
	{
		if((c = *r++) == '\t' && !cvttabs && !HEXMODE)
		{
			do
			{
				if(b - bufp >= tablen)
				{
					do
					{
						*b++ = ' ';
						if(b >= end)
						{
							tmp = (Char *)imalloc(bufl << 1);
							memcpy(tmp,bufp,bufl);
							b = tmp + (b - bufp);
							ifree(bufp);
							end = (bufp = tmp) + (bufl <<= 1) - 1;
						}
					} while(((b - bufp) & 7));
					break;
				}
				*b++ = ' ';
				if(b >= end)
				{
					tmp = (Char *)imalloc(bufl << 1);
					memcpy(tmp,bufp,bufl);
					b = tmp + (b - bufp);
					ifree(bufp);
					end = (bufp = tmp) + (bufl <<= 1) - 1;
				}
			} while(*++t == ' ');
		}
		else if((NONPRINTNCS(c) || ((c == 32) && cvttabs)) || HEXMODE)
		{
			i = ((Uchar)c);
			i &= 0xff;
			i <<= 1;
			p = hexstring + i;
			if(b + 4 >= end)
			{
				tmp = (Char *)imalloc(bufl << 1);
				memcpy(tmp,bufp,bufl);
				b = tmp + (b - bufp);
				ifree(bufp);
				end = (bufp = tmp) + (bufl <<= 1) - 1;
			}
			*b++ = '<';
			*b++ = *p++;
			*b++ = *p++;
			*b++ = '>';
			t += 4;
		}
		else
		{
			*b++ = c;
			if(b >= end)
			{
				tmp = (Char *)imalloc(bufl << 1);
				memcpy(tmp,bufp,bufl);
				b = tmp + (b - bufp);
				ifree(bufp);
				end = (bufp = tmp) + (bufl <<= 1) - 1;
			}
			t++;
		}
		if(t == tabend)
			t--;
		l--;
	}
	*b = 0;
	*buf = bufp;
	l = b - bufp;
	return(l);
}

