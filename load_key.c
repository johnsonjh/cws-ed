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
#include <string.h>
#include <stdlib.h>

#define MAXRECUR 8

#include "memory.h"
#include "ctyp_dec.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "key_dec.h"

typedef struct def_str *def_ptr;
typedef struct def_str
{
	def_ptr next;	/* next keydef in list */
	Schar value;	/* key value */
	Char lstloaded;	/* flag that list of defined subkeys is correct */
	Int length;	/* length of definition */
	Schar *values;	/* string of subkeys */
	Char *deflst;	/* string of key-is-defined flags */
	Int *repeats;	/* repeat counts for subkeys */
} def_node;

static Int forgiven = 0;
static def_ptr base = 0,last = 0;
static Int level = -1;
static def_ptr def[MAXRECUR];
static Schar *byt[MAXRECUR];
static Char *isdef[MAXRECUR];
static Int *cnt[MAXRECUR];
static Int remain[MAXRECUR];
static Int repeat[MAXRECUR];

static Schar *defbuf;	/* these are used while defining keys */
static Int *rptbuf;
static Int defcount = 0;	/* count of keys in definition */
static Int defining = -1;	/* which window has the "Defining key..." message, or -1 if no definition is in progress */
static Schar defvalue;	/* key being defined */
static Char ignore = 0;	/* flag to prevent evolution when user presses key that is to be defined */
static Char abortkey = 0;	/* for aborting key evolution when bad condition obtains */
static Char ignorealpha = 0;	/* flag to prevent evolution of alpha keys */
static Char has_been_putoff = 0;	/* flag that putoff() was called */

extern Char *window_title();

/******************************************************************************\
|Routine: was_putoff
|Callby: init_term
|Purpose: Checks whether we have to redisplay because we had called putoff();
|Arguments:
|    none
\******************************************************************************/
Int was_putoff()
{
	if(has_been_putoff)
	{
		has_been_putoff = 0;
		return(1);
	}
	return(0);
}

/******************************************************************************\
|Routine: unload_key
|Callby: load_key
|Purpose: Removes a key definition from the key database.
|Arguments:
|    value is the value of the key to toss.
\******************************************************************************/
void unload_key(value)
Schar value;
{
	def_ptr d,l;

	l = (def_ptr)&base;
	for(d = base;d;l = d,d = d->next)
		if(d->value == value)
		{
			l->next = d->next;
			for(last = l;last->next;last = last->next);
			ifree(d->repeats);
			ifree(d->values);
			ifree(d->deflst);
			ifree(d);
			for(d = base;d;d = d->next)	/* clear all list-loaded flags */
				d->lstloaded = 0;
			return;
		}
}

/******************************************************************************\
|Routine: load_key
|Callby: load_key restore_par
|Purpose: Stores a sequence of characters and repeat counts as a key definition
|         in the key database.
|Arguments:
|    value is the value of the key that has been defined.
|    length is the number of character/repeat pairs in the definition.
|    buf is the sequence of characters.
|    rpt is the array of repeat counts.
\******************************************************************************/
void load_key(value,length,buf,rpt)
Int value,length;
Schar *buf;
Int *rpt;
{
	register def_ptr new;

/* remove any existing definition for this key */
	unload_key((Schar)value);
	new = (def_ptr)imalloc(sizeof(def_node));
	new->values = (Schar *)imalloc(length);
	new->deflst = (Char *)imalloc(length);
	new->repeats = (Int *)imalloc(length * sizeof(Int));
	memcpy((Char *)new->values,(Char *)buf,length);
	memcpy(new->repeats,rpt,length * sizeof(Int));
	new->length = length;
	new->value = value;
	if(!base)
		base = new;
	else
		last->next = new;
	new->next = 0;
	last = new;
	for(new = base;new;new = new->next)	/* clear all list-loaded flags */
		new->lstloaded = 0;
}

/******************************************************************************\
|Routine: defined_key
|Callby: init_term load_key
|Purpose: Tests whether a key has a definition associated with it. Returns 1
|         if the key is defined, else 0. Also stores the repeat count
|         associated with the key so it can start evolving.
|Arguments:
|    value is the key we are testing.
|    rept is the repeat count that was applied to the key when it was pressed
|            (or when it was retrieved from another key definition).
\******************************************************************************/
Int defined_key(value,rept)
Schar value;
Int rept;
{
	register Schar *p;
	register Char *q;
	register def_ptr d,e;
	register Int i;

	if(ignore)
		return(0);
	if(defining >= 0 && level < 0 && value != 11)
	{
		defbuf[defcount] = value;
		rptbuf[defcount++] = rept;
	}
	if(level == MAXRECUR-1)
		return(0);
	if(!(value & 0x80))
		if(isalpha(value))
			if(ignorealpha || defining >= 0)
				return(0);
	for(d = base;d;d = d->next)
		if(d->value == value)	/* key is defined */
		{
			def[++level] = d;
			repeat[level] = rept;
			byt[level] = d->values;
			isdef[level] = d->deflst;
			if(!d->lstloaded)
			{
				for(q = d->deflst,p = d->values,i = d->length;i--;p++,q++)	/* load the def list */
				{
					*q = 0;
					for(e = base;e;e = e->next)
						if(e->value == *p)
						{
							*q = 1;	/* this key in definition is itself defined */
							break;
						}
				}
				d->lstloaded = 1;
			}
			remain[level] = d->length;
			cnt[level] = d->repeats;
			if(!has_been_putoff && rept > 19)
			{
				has_been_putoff = 1;
				putoff();
			}
			return(1);
		}
	return(0);
}

/******************************************************************************\
|Routine: next_key
|Callby: init_term load_key
|Purpose: Retrieves the next character/repeat pair in an evolving key. Returns
|         0 if the definition has come to an end (or has been aborted), else 1.
|Arguments:
|    val is the returned key value.
|    rept is the returned repeat count.
\******************************************************************************/
Int next_key(val,rept)
Char *val;
Int *rept;
{
	if(abortkey)
	{
		abortkey = 0;
		level = -1;
		return(0);
	}
	if(!remain[level]--)	/* if we are out of chars at the current level */
		if(--repeat[level] <= 0)	/* if there are no more repetitions at this level */
			return((--level < 0)? 0 : next_key(val,rept));	/* if at top level, fail, else recurse */
		else	/* another repetition at this level */
		{
			byt[level] = def[level]->values;
			isdef[level] = def[level]->deflst;
			remain[level] = def[level]->length - 1;
			cnt[level] = def[level]->repeats;
		}
	*val = *byt[level]++;	/* return next char */
	*rept = *cnt[level]++;
	if(!has_been_putoff && *rept > 19)
	{
		has_been_putoff = 1;
		putoff();
	}
	if(!isalpha(*val) || *val & 0x80)	/* prevent recursion for alpha keys */
		if(isdef[level][byt[level] - def[level]->values - 1])
			if(defined_key(*val,*rept))
				next_key(val,rept);
	return(1);
}

/******************************************************************************\
|Routine: define_key
|Callby: edit inquire
|Purpose: Handles creation of new key definitions. Called when they press ^K.
|Arguments:
|    none
\******************************************************************************/
void define_key()
{
	Int i;
	Char buf[512];
	
/* if definition is in progress, terminate definition */
	if(defining >= 0)
	{
		unload_key(defvalue);
		if(defcount)
			load_key(defvalue,defcount,defbuf,rptbuf);
		ifree(rptbuf);
		ifree(defbuf);
		ignorealpha = 0;
		move(WINDOW[defining].toprow - 1,1);	/* redisplay the file name */
		ers_end();
		reverse();
		i = defining;
		defining = -1;	/* this must be set first */
		putz(window_title(i));
		normal();
		move(CURROW,CURCOL);
		return;
	}
	move(WINDOW[0].toprow - 1,1);	/* CURWINDOW is bad if window is closed! */
	ers_end();
	reverse();
	putz("Press the key you want to define.");
	normal();
	move(CURROW,CURCOL);
/* start new definition */
	ignore = 1;
	get_next_key(&defvalue);
	ignore = 0;
	if(defvalue == 11)
	{
		move(WINDOW[0].toprow - 1,1);	/* redisplay the file name */
		ers_end();
		reverse();
		putz(window_title(0));
		normal();
		move(CURROW,CURCOL);
		return;
	}
	move(WINDOW[0].toprow - 1,1);
	ers_end();
	reverse();
	sprintf(buf,"Defining key '%s'...",keyname[defvalue + 128]);
	putz(buf);
	normal();
	move(CURROW,CURCOL);
	defining = 0;
	ignorealpha = 1;
	defbuf = (Schar *)imalloc(4096);
	rptbuf = (Int *)imalloc(4096*sizeof(int));
	defcount = 0;
}

/******************************************************************************\
|Routine: store_keys
|Callby: store_param
|Purpose: Stores all key definitions in the user's startup file.
|Arguments:
|    fp is the startup file pointer.
\******************************************************************************/
void store_keys(fp)
FILE *fp;
{
	Int n,pos;
	def_ptr d;
	Char buf[128];

	for(n = 0,d = base;d;d = d->next)
		n++;
	fprintf(fp,"ndefined=%d\n",n);
	for(pos = 0,d = base;d;d = d->next)
	{
		sprintf(buf,"%x ",((int)d->value) & 0xff);
		fputs(buf,fp);
		pos += strlen(buf);
		sprintf(buf,"%d ",d->length);
		fputs(buf,fp);
		pos += strlen(buf);
		for(n = 0;n < d->length;n++)
		{
			sprintf(buf,"%x %x ",((int)d->values[n]) & 0xff,d->repeats[n]);
			fputs(buf,fp);
			if((pos += strlen(buf)) > 80)
			{
				fprintf(fp,"\n");
				pos = 0;
			}
		}
	}
	if(pos)
		fprintf(fp,"\n");
}

/******************************************************************************\
|Routine: definition_inprog
|Callby: inquire ref_window
|Purpose: Tests whether a key is currently being defined. Returns the window
|         in which the "Defining key..." message appears, or -1 if no definition
|         is in progress.
|Arguments:
|    none
\******************************************************************************/
Int definition_inprog()
{
	return(defining);
}

/******************************************************************************\
|Routine: abort_key
|Callby: command copier down_arrow edit killer left_arrow move_eol move_word right_arrow up_arrow wincom
|Purpose: Flags a condition that should make key definitions stop evolving. This
|         would typically be a failed search, an attempt to move off the ends of
|         the window, or an attempt to kill off the end of the window.
|Arguments:
|    none
\******************************************************************************/
void abort_key()
{
	if(!forgiven)
		if(level >= 0)	/* ignore unless key is in progress */
			abortkey = 1;
}

/******************************************************************************\
|Routine: ignore_alpha
|Callby: inquire
|Purpose: Prevents evolution of alpha keys. This is helpful when getting search
|         strings and the like.
|Arguments:
|    none
\******************************************************************************/
void ignore_alpha()	/* prevent alpha evolution */
{
	ignorealpha = 1;
}


/******************************************************************************\
|Routine: see_alpha
|Callby: inquire
|Purpose: Reestablishes evolution of alpha keys. Converse of ignore_alpha.
|Arguments:
|    none
\******************************************************************************/
void see_alpha()
{
	ignorealpha = 0;
}

/******************************************************************************\
|Routine: forgive
|Callby: edit
|Purpose: Can't remember.
|Arguments:
|	none
\******************************************************************************/
void forgive()
{
	forgiven = 1;
}

/******************************************************************************\
|Routine: begrudge
|Callby: edit
|Purpose: Can't remember.
|Arguments:
|	none
\******************************************************************************/
void begrudge()
{
	forgiven = 0;
}

/******************************************************************************\
|Routine: unload_defs
|Callby: command
|Purpose: Unloads all key definitions to a def file.
|Arguments:
|    none
\******************************************************************************/
void unload_defs(file)
Char *file;
{
	FILE *fp;
	Int n,pos,val,ind;
	def_ptr d;
	Char buf[128],buf2[128];

	if((fp = fopen(file,"w")))
	{
		for(d = base;d;d = d->next)
		{
			val = ((int)d->value) + 128;
			sprintf(buf,"key:%s =",keyname[val]);
			fputs(buf,fp);
			pos = strlen(buf);
			for(n = 0;n < d->length;n++)
			{
				ind = ((int)d->values[n]) + 128;
				sprintf(buf,"[%d]",d->repeats[n]);
				if(pos + 1 + strlen(buf) + strlen(keyname[ind]) > 80)
				{
					fprintf(fp,"\n        ");
					pos = 8;
				}
				sprintf(buf2," %s%s",buf,keyname[ind]);
				fputs(buf2,fp);
				pos += strlen(buf2);
			}
			fprintf(fp,"\n");
		}
		fclose(fp);
	}
}

/******************************************************************************\
|Routine: load_defs
|Callby: command
|Purpose: Loads all key definitions from an unloaded def file. Returns the
|         number of matching strings (should be 1) or -1 when the buffer
|         empties.
|Arguments:
|    pp - the pointer in the user buffer.
|    count - a flag indicating that a [n] count should be parsed.
|    pv - the returned key value.
|    pr - the returned repeat count.
\******************************************************************************/
Int get_keyval(pp,count,pv,pr)
Char **pp;
Int count;
Int *pv;
Int *pr;
{
	Char *p,hit[256],c;
	Int i,j,hits;
	
	p = *pp;
	if(*p == '\n' || *p == '\0')
		return(-1);
	while(*p == ' ')	/* skip blanks */
		if(*++p == '\n')
			return(-1);
	if(count)
	{
		if(*p++ != '[')
			return(0);
		for(i = 0;(c = *p++) != ']';i = 10 * i + c - '0');	/* parse the repeat count */
		*pr = i;
	}
	memset(hit,1,sizeof(hit));	/* set hit list */
	hits = 256;
	i = 0;	/* column number in keyname array */
	while(1)
	{
		if((c = *p++) == ' ')
			c = 0;
		else if(c == '\n')
			c = 0;
		for(j = 0;j < 256;j++)
			if(hit[j])
				if(c != keyname[j][i])
				{
					hit[j] = 0;
					hits--;
				}
		if(!c)
			break;
		i++;
	}
	if(hits == 1)
	{
		for(i = 0;i < 256;i++)
			if(hit[i])
				break;
		*pv = i - 128;
	}
	*pp = p;
	return(hits);
}

/******************************************************************************\
|Routine: load_defs
|Callby: command
|Purpose: Loads all key definitions from an unloaded def file.
|Arguments:
|    none
\******************************************************************************/
void load_defs(file)
Char *file;
{
	FILE *fp;
	Schar keys[8192];
	Char buf[2048],errorbuf[2048],*p,*q;	/* keys is signed char */
	Int nkeys,value,val,rpt,i;
	Int repeat[8192];
	def_ptr d,n;
	
	if((fp = fopen(file,"r")))
	{
		for(d = base;d;d = n)	/* remove all defined keys */
		{
			n = d->next;
			ifree(d->repeats);
			ifree(d->values);
			ifree(d->deflst);
			ifree(d);
		}
		base = last = 0;
		while(fgets(buf,sizeof(buf),fp))	/* read from file */
		{
			if(!strncmp(buf,"key:",4))	/* a new key starts */
			{
redo:
				q = p = buf + 4;
				i = get_keyval(&p,0,&value,&rpt);
				if(i != 1)
					goto error;
				while(*p == ' ' || *p == '=')
					p++;
				nkeys = 0;
contin:
				while(1)
				{
					q = p;
					if((i = get_keyval(&p,1,&val,&rpt)) < 0)
						break;	/* get another line */
					if(i != 1)
						goto error;
					repeat[nkeys] = rpt;
					keys[nkeys++] = val;
				}
				if(!fgets(buf,sizeof(buf),fp))	/* end of file, complete def */
				{
					load_key(value,nkeys,keys,repeat);
					break;
				}
				if(!strncmp(buf,"key:",4))	/* new key starts, complete def and redo  */
				{
					load_key(value,nkeys,keys,repeat);
					goto redo;
				}
				p = buf;
				goto contin;
			}
		}
		fclose(fp);
	}
	return;
error:
	fclose(fp);
	if(!i)
		sprintf(errorbuf,"Unrecognized key name:%s",q);
	else if(i > 1)
		sprintf(errorbuf,"Ambiguous key name:%s",q);
	slip_message(errorbuf);
	wait_message();
}

