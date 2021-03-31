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
#include <string.h>

#include "memory.h"
#include "ctyp_dec.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "buffer.h"
#include "buf_dec.h"
#include "cmd_enum.h"
#include "regex.h"

#include "handy.h"

#define MAX_REGEX 64	/* maximum number of regexps that may be extracted from user's search string */

static Uchar normal[256] =
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
};

static Uchar table[257];	/* this is for user-specified search tables */
static Char general,beginning,wild,tabledriven,regexp,wordmode;
static Uchar *tbl1,*tbl2;
static struct re_pattern_buffer pbuf,wbuf[MAX_REGEX];
static Char startrec,startrecs[MAX_REGEX],endrec,endrecs[MAX_REGEX],must_recompile_regex;
static Char *delim = NULL;	/* word-delimiter table */
static Int nwords = 0;

extern Char *wordtable();

/******************************************************************************\
|Routine: match_init
|Callby: edit find_string word_fill
|Purpose: Initializes local variables for match_search.
|Arguments:
|    flags is the set of flags that constrain the search. They are:
|            [0] = g means case-insensitive, else (e) case-sensitive.
|            [1] = b means match position is at beginning of string, else (e) end.
|            [2] = w means WILDCARD is wildcard, else (n) not.
|            [3] = n means no search table, else (t) table in effect.
|            [4] = r means use regular expression searching, n means don't. Overrides the rest of the flags, except beginning/end.
|            [5] = c means search-by-character (the default), else (w) search-by-word.
\******************************************************************************/
void match_init(flags)
Char flags[6];
{
	general = flags[((int)SMODE_GENERAL) >> 1];
	beginning = flags[((int)SMODE_BEGINNING) >> 1];
	wild = flags[((int)SMODE_WILDCARDS) >> 1];
	tabledriven = flags[((int)SMODE_TABLE_DRIVEN) >> 1];
	if((regexp = flags[((int)SMODE_REGEXP) >> 1]) == 'r')
		must_recompile_regex = 1;
	wordmode = (flags[((int)SMODE_CHAR) >> 1] == 'w');
	tbl1 = (tabledriven == 't')? normal : (general == 'g')? _my_toupper_ : normal;
	tbl2 = (tabledriven == 't')? table : (general == 'g')? _my_toupper_ : normal;
	delim = wordtable();
}

/******************************************************************************\
|Routine: parse_regex
|Callby: match_search
|Purpose: Preprocesses regexps so they work right, and compiles them.
|Arguments:
|    currec is the record being compared to the search string.
|    curbyt is the position in the record where the comparison starts.
|    searchbuf is the buffer containing the search string.
\******************************************************************************/
void parse_regexp(pbuf,buf,startrec,endrec)
struct re_pattern_buffer *pbuf;
Char *buf;
Char *startrec,*endrec;
{
	Int i;
	Char *p;
	
	if(buf[0] == '^')	/* check for ^... */
	{
		*startrec = 1;
		i = strlen(buf) - 1;
		if(i > 0 && buf[1] == '^')	/* change ^^... to \^... */
			buf[0] = '\\';
		else
		{
			for(p = buf;i--;p++)	/* slide exp to left */
				*p = *(p + 1);
			*p = '\0';
		}
	}
	else
		*startrec = 0;
	i = strlen(buf) - 1;
	if(buf[i] == '$')	/* check for ...$ */
	{
		*endrec = 1;
		if(i > 1 && buf[i - 1] == '$')	/* change ...$$ to ...\$ */
			buf[i - 1] = '\\';
		else
			buf[i] = '\0';	/* shorten exp */
	}
	else
		*endrec = 0;
	re_compile_pattern(buf,strlen(buf),pbuf);	/* let the re_match call report an erroneous RE */
}

/******************************************************************************\
|Routine: match_search
|Callby: edit find_string inquire word_fill
|Purpose: Determines whether the current position is on a matching string.
|         Returns the length of the matching string.
|Arguments:
|    currec is the record being compared to the search string.
|    curbyt is the position in the record where the comparison starts.
|    searchbuf is the buffer containing the search string.
\******************************************************************************/
Int match_search(currec,curbyt,searchbuf)
rec_ptr currec;
Int curbyt;
buf_ptr searchbuf;
{
	register Uchar *cr,*cs,*t1,*t2,*p,*q;
	register Int c1,c2;
	rec_ptr r,s,tr,ts;
	Int lr,ls,ss;
	Int offset;
	Char rbuf[1100],*pp,*qq;
	Int i,j;
	Char buf[1024];
	
	if((s = searchbuf->first) == (rec_ptr)&searchbuf->first)
		return(0);
	if(wordmode && searchbuf->nrecs > 1)
		return(0);
	if(must_recompile_regex)
	{
		must_recompile_regex = 0;
		if(wordmode)
		{
			ts = s;
			ss = 0;
			nwords = 0;
			strcpy(buf,s->data);
			strcat(buf," ");
			for(pp = buf;(qq = strchr(pp,' '));pp = ++qq)
			{
				memcpy(rbuf,pp,(i = qq - pp));
				rbuf[i] = '\0';
				parse_regexp(wbuf + nwords,rbuf,startrecs + nwords,endrecs + nwords);
				if(++nwords > 64)
					break;
			}
		}
		else
		{
			strcpy(buf,searchbuf->first->data);
			parse_regexp(&pbuf,buf,&startrec,&endrec);
		}
	}
	r = currec;
	cr = (Uchar *)&r->data[curbyt];
	offset = 0;
	t1 = tbl1;
	t2 = tbl2;
	if(beginning == 'b')
	{
		lr = r->length - curbyt;
		if(WINDOW[CURWINDOW].binary)	/* binary-mode search ignores linebreaks, has special handling for <0d> in search buffer */
		{
			if(curbyt == r->length)	/* disallow matches at eol in begin mode */
				return(0);
			cs = (Uchar *)s->data;	/* start at beginning of search buffer's first record */
			ls = s->length;
			while(1)
			{
				if(r == BASE)	/* hit end of file */
					return(0);
/* insure we have another search buffer byte */
				while(!ls)	/* this record in search buffer ends, advance */
				{
					if((s = s->next) == (rec_ptr)&searchbuf->first)	/* end of search buffer, succeed */
						return(1);
					while(!lr)	/* insure another file byte is available */
					{
						if((r = r->next) == BASE)
							return(0);
						lr = r->length;
						cr = (Uchar *)r->data;
					}
					if(*(t2 + *cr++) != 13)	/* require a CR to match the search buffer linebreak */
						return(0);
					lr--;
					offset++;	/* advance for CR */
					cs = (Uchar *)s->data;	/* new pointers */
					ls = s->length;
				}
/* insure we have another file byte */
				while(!lr)	/* ran out on file record, advance */
				{
					if((r = r->next) == BASE)
						return(0);
					lr = r->length;
					cr = (Uchar *)r->data;
				}
				if(wild == 'n')
				{
					if(*(t1 + *cs++) != *(t2 + *cr++))	/* compare */
						return(0);
				}
				else
				{
					if(*cs == WILDCARD)	/* wildcards match anything */
					{
						cs++;
						cr++;
					}
					else
						if(*(t1 + *cs++) != *(t2 + *cr++))	/* compare */
							return(0);
				}
				ls--;
				lr--;
				offset++;	/* advance offset */
			}
		}
		if(regexp == 'r' && !wordmode)
		{
			if(startrec && curbyt)	/* ignore attempts not at beginning of record when ^... present */
				return(0);
			if((i = re_match(&pbuf,(const char *)cr,lr,0,NULL)) == -2)
				goto regex_error;
			if(i < 0)
				return(0);
			if(endrec && (curbyt + i != r->length))	/* require match at end of record when ...$ present */
				return(0);
			return(i);	/* note that re_match may return 0 as a success code. hmmm. */
		}
		if(wild == 'n' || regexp == 'r')	/* no wild cards, or wordmode regex processing */
		{
/*
word mode (beginning):

0) find a word in s, else succeed.
1) find a word in r, else fail.
2) if words don't compare equal, fail.
6) go to 2)
*/
			if(wordmode)
			{
				if(r == BASE)
					return(0);
				if(regexp != 'r')
				{
					for(cs = (Uchar *)s->data,ls = s->length;ls--;cs++)
						if(!delim[*cs])
							break;
					if(!ls)
						return(0);	/* search string contains only delims, or is empty */
				}
				if(!on_a_word(r,curbyt))	/* we can only succeed when on a word */
					return(0);
				ss = 0;	/* this is the current offset in the search string */
				tr = r;	/* this is a temporary rec_ptr for traversing the file */
				ts = s;	/* this is a temporary rec_ptr for traversing the search string */
				if(regexp == 'r')
				{
					if(!nwords)
						return(0);
					for(j = 0;j < nwords;j++)
					{
						if(startrecs[j] && curbyt)	/* ignore attempts not at beginning of record when ^... present */
							return(0);
						if(!get_word_start(BASE,&tr,&curbyt,&lr,&offset))
							return(0);
						if((i = re_match(wbuf + j,(const char *)tr->data + curbyt,lr,0,NULL)) == -2)
							goto regex_error;
						if(i < 0)
							return(0);
						if(endrecs[j] && (curbyt + i != r->length))	/* require match at end of record when ...$ present */
							return(0);
						curbyt += lr;	/* advance to past end of word in file */
					}
				}
				else
				{
					while(1)
					{
						if(!get_word_start(s,&ts,&ss,&ls,NULL))
							break;	/* we have a hit */
						if(!get_word_start(BASE,&tr,&curbyt,&lr,&offset))
							return(0);
						if(ls != lr)	/* word length mismatch */
							return(0);
						for(p = (Uchar *)ts->data + ss,q = (Uchar *)tr->data + curbyt,i = ls;i--;p++,q++)	/* compare the two words */
							if(*(t1 + *p) != *(t2 + *q))
								return(0);
						curbyt += lr;	/* advance to past end of word in file */
						ss += ls;	/* advance to past end of word in search string */
					}
				}
			}
			else	/* not wordmode */
				do
				{
					if(r == BASE)
						return(0);
					if((ls = s->length) > lr)
						return(0);
					for(cs = (Uchar *)s->data;ls--;)
					{
						if(*(t1 + *cs++) != *(t2 + *cr++))
							return(0);
						offset++;
					}
					if((s = s->next) != (rec_ptr)&searchbuf->first) /* insure the file record ends here too */
					{
						if(cr != (Uchar *)&r->data[r->length])	/* search record ends, file record continues */
							return(0);
						r = r->next;
						lr = r->length;
						cr = (Uchar *)r->data;
						offset++;
					}
				} while(s != (rec_ptr)&searchbuf->first);
		}
		else	/* WILDCARD wild cards allowed */
		{
			if(wordmode)
			{
				for(cs = (Uchar *)s->data,ls = s->length;ls--;cs++)
					if(!delim[*cs])
						break;
				if(!ls)
					return(0);	/* search string contains only delims, or is empty */
				if(r == BASE)
					return(0);
				if(!on_a_word(r,curbyt))	/* we can only succeed when on a word */
					return(0);
				ss = 0;	/* this is the current offset in the search string */
				tr = r;	/* this is a temporary rec_ptr for traversing the file */
				ts = s;	/* this is a temporary rec_ptr for traversing the search string */
				while(1)
				{
					if(!get_word_start(s,&ts,&ss,&ls,NULL))
						break;	/* we have a hit */
					if(!get_word_start(BASE,&tr,&curbyt,&lr,&offset))
						return(0);
					if(ls != lr)	/* word length mismatch */
						return(0);
					for(p = (Uchar *)ts->data + ss,q = (Uchar *)tr->data + curbyt,i = ls;i--;p++,q++)	/* compare the two words */
						if(*p != WILDCARD)
							if(*(t1 + *p) != *(t2 + *q))
								return(0);
					curbyt += lr;	/* advance to past end of word in file */
					ss += ls;	/* advance to past end of word in search string */
				}
			}
			else
				do
				{
					if(r == BASE)
						return(0);
					if((ls = s->length) > lr)
						return(0);
					for(cs = (Uchar *)s->data;ls--;)
					{
						c1 = *cs++;
						c2 = *cr++;
						if(c1 != WILDCARD)
							if(*(t1 + c1) != *(t2 + c2))
								return(0);
						offset++;
					}
					if((s = s->next) != (rec_ptr)&searchbuf->first) /* insure the file record ends here too */
					{
						if(cr != (Uchar *)&r->data[r->length])	/* search record ends, file record continues */
							return(0);
						r = r->next;
						lr = r->length;
						cr = (Uchar *)r->data;
						offset++;
					}
				} while(s != (rec_ptr)&searchbuf->first);
		}
	}
	else	/* we are SET SEARCH END */
	{
		lr = curbyt;
		if(regexp == 'r' && !wordmode)
		{
			if(endrec && (curbyt != r->length))	/* ignore attempts not at end of record when ...$ is present */
				return(0);
			while(lr > 0)
			{
				if((i = re_match(&pbuf,(const char *)(r->data + curbyt - lr),lr,0,NULL)) == -2)
					goto regex_error;
				else if(i >= 0)
				{
					if(lr == i)
					{
						if(startrec && (i != curbyt))	/* require match at beginning of record when ^... is present */
							return(0);
						return(-lr);
					}
				}
				lr--;
			}
			return(0);
		}
		s = searchbuf->last;
		if(WINDOW[CURWINDOW].binary)	/* binary-mode search ignores linebreaks, has special handling for <0d> in search buffer */
		{
			if(!curbyt)	/* disallow bol matches in end mode */
				return(0);
			ls = s->length;
			cs = (Uchar *)s->data + ls;	/* start at the end of search buffer's last record */
			while(1)
			{
				if(r == BASE)	/* hit end of file */
					return(0);
/* insure we have another search buffer byte */
				while(!ls)	/* hit beginning of this record in search buffer, advance */
				{
					if((s = s->prev) == (rec_ptr)&searchbuf->first)	/* hit beginning of search buffer, succeed */
						return(1);
					while(!lr)	/* insure another file byte is available */
					{
						if((r = r->prev) == BASE)
							return(0);
						lr = r->length;
						cr = (Uchar *)r->data + lr;
					}
					if(*(t2 + *--cr) != 13)	/* require a CR to match the search buffer linebreak */
						return(0);
					lr--;
					offset++;	/* advance for CR */
					ls = s->length;
					cs = (Uchar *)s->data + ls;	/* new pointers */
				}
/* insure we have another file byte */
				while(!lr)	/* ran out on file record, advance */
				{
					if((r = r->prev) == BASE)
						return(0);
					lr = r->length;
					cr = (Uchar *)r->data + lr;
				}
				if(wild == 'n')
				{
					if(*(t1 + *--cs) != *(t2 + *--cr))	/* compare */
						return(0);
				}
				else
				{
					if(*--cs == WILDCARD)
						cr--;
					else
						if(*(t1 + *cs) != *(t2 + *--cr))	/* compare */
							return(0);
				}
				ls--;
				lr--;
				offset++;	/* advance offset */
			}
		}
		if(wild == 'n' || regexp == 'r')	/* no wild cards, or in regex mode */
		{
			if(wordmode)
			{
				if(regexp != 'r')
				{
					for(cs = (Uchar *)s->data,ls = s->length;ls--;cs++)
						if(!delim[*cs])
							break;
					if(!ls)
						return(0);	/* search string contains only delims, or is empty */
				}
				if(r == BASE)
					return(0);
				if(!on_a_wordend(r,curbyt))	/* we can only succeed when on a wordend */
					return(0);
				ss = strlen(s->data);	/* this is the current offset in the search string */
				tr = r;	/* this is a temporary rec_ptr for traversing the file */
				ts = s;	/* this is a temporary rec_ptr for traversing the search string */
				if(regexp == 'r')
				{
					if(!nwords)
						return(0);
					for(j = nwords - 1;j >= 0;j--)
					{
						if(startrecs[j] && curbyt)	/* ignore attempts not at beginning of record when ^... present */
							return(0);
						if(!get_word_end(BASE,&tr,&curbyt,&lr,&offset))
							return(0);
						if((i = re_match(wbuf + j,(const char *)tr->data + curbyt,lr,0,NULL)) == -2)
							goto regex_error;
						if(i < 0)
							return(0);
						if(endrecs[j] && (curbyt + i != r->length))	/* require match at end of record when ...$ present */
							return(0);
					}
				}
				else
				{
					while(1)
					{
						if(!get_word_end(s,&ts,&ss,&ls,NULL))
							break;	/* we have a hit */
						if(!get_word_end(BASE,&tr,&curbyt,&lr,&offset))
							return(0);
						if(ls != lr)	/* word length mismatch */
							return(0);
						for(p = (Uchar *)ts->data + ss,q = (Uchar *)tr->data + curbyt,i = ls;i--;p++,q++)	/* compare the two words */
							if(*(t1 + *p) != *(t2 + *q))
								return(0);
					}
				}
			}
			else	/* not wordmode */
				do
				{
					if(r == BASE)
						return(0);
					if((ls = s->length) > lr)
						return(0);
					for(cs = (Uchar *)&s->data[ls];ls--;)
					{
						if(*(t1 + *--cs) != *(t2 + *--cr))
							return(0);
						offset--;
					}
					if((s = s->prev) != (rec_ptr)&searchbuf->first) /* insure the file record ends here too */
					{
						if(cr != (Uchar *)r->data)      /* search record ends, file record continues */
							return(0);
						r = r->prev;
						lr = r->length;
						cr = (Uchar *)&r->data[lr];
						offset--;
					}
				} while(s != (rec_ptr)&searchbuf->first);
		}
		else	/* WILDCARD wild cards allowed */
		{
			if(wordmode)
			{
				for(cs = (Uchar *)s->data,ls = s->length;ls--;cs++)
					if(!delim[*cs])
						break;
				if(!ls)
					return(0);	/* search string contains only delims, or is empty */
				if(r == BASE)
					return(0);
				if(!on_a_wordend(r,curbyt))	/* we can only succeed when on a wordend */
					return(0);
				ss = strlen(s->data);	/* this is the current offset in the search string */
				tr = r;	/* this is a temporary rec_ptr for traversing the file */
				ts = s;	/* this is a temporary rec_ptr for traversing the search string */
				while(1)
				{
					if(!get_word_end(s,&ts,&ss,&ls,NULL))
						break;	/* we have a hit */
					if(!get_word_end(BASE,&tr,&curbyt,&lr,&offset))
						return(0);
					if(ls != lr)	/* word length mismatch */
						return(0);
					for(p = (Uchar *)ts->data + ss,q = (Uchar *)tr->data + curbyt,i = ls;i--;p++,q++)	/* compare the two words */
						if(*p != WILDCARD)
							if(*(t1 + *p) != *(t2 + *q))
								return(0);
				}
			}
			else
				do
				{
					if(r == BASE)
						return(0);
					if((ls = s->length) > lr)
						return(0);
					for(cs = (Uchar *)&s->data[ls];ls--;)
					{
						c1 = *--cs;
						c2 = *--cr;
						if(c1 != WILDCARD)
							if(*(t1 + c1) != *(t2 + c2))
								return(0);
						offset--;
					}
					if((s = s->prev) != (rec_ptr)&searchbuf->first) /* insure the file record ends here too */
					{
						if(cr != (Uchar *)r->data)      /* search record ends, file record continues */
							return(0);
						r = r->prev;
						lr = r->length;
						cr = (Uchar *)&r->data[lr];
						offset--;
					}
				} while(s != (rec_ptr)&searchbuf->first);
		}
	}
	return(offset);
regex_error:
	strcpy(rbuf,"'");
	memcpy(rbuf + 1,SEARCHBUF.first->data,SEARCHBUF.first->length);
	strcpy(rbuf + SEARCHBUF.first->length + 1,"' is not a valid regular expression.");
	slip_message(rbuf);
	wait_message();
	return(0);
}

/******************************************************************************\
|Routine: set_search_table
|Callby: restore_par set_param
|Purpose: Sets up a search table.
|Arguments:
|     file is the name of the search table, to be read in from disk file.
\******************************************************************************/
Int set_search_table(file)
Char *file;
{
	FILE *fp;
	Int l;
	Uchar buf[512];
	
	strip_quotes(file);
	envir_subs(file);
	for(l = 0;l < 256;l++)
		table[l] = l;
	if(!(fp = fopen(file,"r")))
		return(0);
	while(fgets((Char *)buf,sizeof(buf),fp))
		table[buf[0]] = buf[1];
	fclose(fp);
	return(1);
}

