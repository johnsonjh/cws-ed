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
#include <math.h>
#ifdef VMS
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include "ctyp_dec.h"

static Uchar list[256];	/* for [...] expressions */

/******************************************************************************\
|Routine: tentothe
|Callby: libc
|Purpose: Returns powers of ten.
|Arguments:
|	i is the power to raise ten to.
\******************************************************************************/
double tentothe(i)
Int i;
{
	static double powers[] =
	{
		1e0,1e1,1e2,1e3,1e4,1e5,1e6,1e7,1e8,1e9,1e10,1e11,1e12,1e13,1e14,1e15,
		1e16,1e17,1e18,1e19,1e20,1e21,1e22,1e23,1e24,1e25,1e26,1e27,1e28,1e29,1e30,1e31,
		1e32,1e33,1e34,1e35,1e36,1e37,1e38
	};
	Int j;
	
	j = (sizeof(powers) / sizeof(double)) - 1;
	if(i > j)
		i = j;
	else if(i < -j)
		i = -j;
	return((i >= 0)? powers[i] : (1.0 / powers[-i]));
}

/******************************************************************************\
|Routine: my_sscanf
|Callby: calculate cfg command help_get_kw init_term parse_fnm restore_par set_param store_param wincom
|Purpose: Does what sscanf() does.
|Arguments:
|	inbuf is a string containing formatted data.
|	fmt is the format string.
\******************************************************************************/
#ifdef VMS
Int my_sscanf(inbuf,fmt)
Char *inbuf,*fmt;
#else
Int my_sscanf(va_alist)
va_dcl
#endif
{
	va_list ap;
#ifndef VMS
	Char *inbuf,*fmt;
#endif
	Uchar *b,*f,*save;
	unsigned long ival;
	double dval;
	Int digcount,i,j,c;
	Int suppress,width,size,sign,unsg,not,cnvcount,decimals,pastpoint,esign,edigcount,signpresent,pointpresent;
	Char *cp;
	short *sp;
	Int *ip;
	long *lp;
	float *fp;
	double *dp;
	void **vp;

#ifdef VMS
	va_start(ap,fmt);
#else
	va_start(ap);
	inbuf = va_arg(ap,Char *);
	fmt = va_arg(ap,Char *);
#endif

	f = (Uchar *)fmt;
	b = (Uchar *)inbuf;
	cnvcount = 0;
	while((c = *f++))
		switch(c)
		{
			case '%':
				if(*f != '[' && *f != 'n' && *f != 'c' && *f != '%')
					while(isspace(*b))
						b++;
				suppress = 0;	/* suppression of assignment */
				width = 0;	/* enforced field maximum width (zero if no limit) */
				size = 0;	/* -1 = short, 0 = int, +1 = long, (or -1 or 0 = float, 1 = double, 2 = long double) */
				sign = 1;	/* sign of result */
				unsg = 0;	/* require unsigned int value */
redo:
				switch((c = *f++))
				{
					case '*':
						suppress = 1;
						goto redo;
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						width = c - '0';
						while(isdigit((c = *f++)))
							width = 10 * width + c - '0';
						f--;
						goto redo;
					case 'h':
						size = -1;
						goto redo;
					case 'l':
						size = 1;
						goto redo;
					case 'L':
						size = 2;
						goto redo;
					case 'd':
dec:
						ival = 0;
						digcount = 0;
						if(*b == '-')
						{
							if(unsg)
								return(0);
							sign = -1;
							b++;
						}
						else if(*b == '+')
							b++;
						while(isdigit((i = *b++)))
						{
							ival = 10 * ival + i - '0';
							++digcount;
							if(width)
								if(digcount >= width)
								{
									b++;	/* because of the b-- below */
									break;
								}
						}
integer:
						if(!digcount)
							return(0);
						b--;
						if(!suppress)
						{
							if(sign < 0)
								ival = -ival;
							switch(size)
							{
								case -1:
									sp = va_arg(ap,short *);
									*sp = ival;
									break;
								case 0:
									ip = va_arg(ap,Int *);
									*ip = ival;
									break;
								case 1:
								case 2:
									lp = va_arg(ap,long *);
									*lp = ival;
									break;
							}
							cnvcount++;
						}
						break;
					case 'i':
						if((i = *b++) == '0')
							if(*b == 'x' || *b == 'X')
							{
								b++;
								goto hex;
							}
							else
								goto oct;
						b--;
						goto dec;
					case 'o':
oct:
						ival = 0;
						digcount = 0;
						while(isdigit((i = *b++)))
						{
							if(i == '8' || i == '9')
								return(0);
							ival = (ival << 3) + i - '0';
							digcount++;
							if(width)
								if(digcount >= width)
								{
									b++;	/* because of the b-- below integer: */
									break;
								}
						}
						goto integer;
					case 'u':
						unsg = 1;
						goto dec;
					case 'x':
hex:
						ival = 0;
						digcount = 0;
						while(isxdigit((i = *b++)))
						{
							ival <<= 4;
							if(i >= '0' && i <= '9')
								ival += i - '0';
							else if(i >= 'A' && i <= 'F')
								ival += i - 'A' + 10;
							else if(i >= 'a' && i <= 'f')
								ival += i - 'a' + 10;
							digcount++;
							if(width)
								if(digcount >= width)
								{
									b++;	/* because of the b-- below integer: */
									break;
								}
						}
						goto integer;
					case 'c':
						cp = va_arg(ap,Char *);
						*cp = *b++;
						cnvcount++;
						break;
					case 's':
						cp = va_arg(ap,Char *);
						while(!isspace((i = *b++)))
							*cp++ = i;
						b--;
						*cp = '\0';
						cnvcount++;
						break;
					case 'e':
					case 'f':
					case 'g':
						ival = 0;	/* this is the exponent */
						dval = 0.0;
						digcount = 0;
						signpresent = 0;
						pointpresent = 0;
						if(*b == '-')
						{
							sign = -1;
							b++;
							digcount++;
							signpresent = 1;
						}
						else if(*b == '+')
						{
							b++;
							digcount++;
							signpresent = 1;
						}
						decimals = 0;
						pastpoint = 0;
floatdigits:
						while(isdigit((i = *b++)))
						{
							if(pastpoint)
								decimals++;
							dval = 10.0 * dval + (double)(i - '0');
							++digcount;
							if(width)
								if(digcount >= width)
								{
									b++;	/* because of the b-- below */
									goto floating;
								}
						}
						if(i == '.')
						{
							pointpresent = 1;
							digcount++;
							if(pastpoint)
								goto floating;
							pastpoint = 1;
							if(width)
								if(digcount >= width)
								{
									b++;	/* because of the b-- below */
									goto floating;
								}
							goto floatdigits;
						}
						if(!(digcount - signpresent - pointpresent))
							return(0);
						if(i == 'e' || i == 'E')
						{
							edigcount = 0;
							esign = 1;
							if(*b == '-')
							{
								esign = -1;
								b++;
							}
							else if(*b == '+')
								b++;
							while(isdigit((i = *b++)))
							{
								ival = 10 * ival + i - '0';
								edigcount++;
							}
							if(!edigcount)	/* assume the e or E is part of a string */
							{
								b--;
								goto floating;
							}
						}
floating:
						b--;	/* back up to char that terminated number */
						if(!suppress)
						{
							if(pastpoint)
								dval /= tentothe(decimals);
							if(esign < 0)
								ival = -ival;
							if(ival)
								dval *= tentothe(ival);
							if(sign < 0)
								dval = -dval;
							switch(size)
							{
								case -1:
								case 0:
									fp = va_arg(ap,float *);
									*fp = (float)dval;
									break;
								case 1:
								case 2:
									dp = va_arg(ap,double *);
									*dp = (double)dval;
									break;
							}
							cnvcount++;
						}
						break;
					case 'p':
						if(*b++ != '0')
							return(0);
						i = *b++;
						if(i != 'x' && i != 'X')
							return(0);
						ival = 0;
						for(j = (sizeof(void *) << 1);j--;)
						{
							if(!isxdigit((i = *b++)))
								return(0);
							ival <<= 4;
							if(i >= '0' && i <= '9')
								ival += i - '0';
							else if(i >= 'A' && i <= 'F')
								ival += i - 'A' + 10;
							else if(i >= 'a' && i <= 'f')
								ival += i - 'a' + 10;
						}
						vp = va_arg(ap,void **);	/* this assumes that char * and void * are the same size */
						*vp = (void *)ival;
						cnvcount++;
						break;
					case 'n':
						if(suppress || width || size)
							return(0);
						ip = va_arg(ap,Int *);
						*ip = b - (Uchar *)inbuf;
						break;
					case '[':
						save = f;
						not = 0;
						if(*f == '^')
						{
							not = 1;
							f++;
						}
						if(*f == ']')
						{
							list[']'] = 1;
							f++;
						}
						while((i = *f++) != ']')
							list[i] = 1;
						cp = va_arg(ap,Char *);
						if(not)
						{
							while(list[(int)*b++]);	/* find a non-member of the set */
							while(!list[(i = *b++)])
								*cp++ = i;
						}
						else
						{
							while(!list[*b++]);	/* find a member of the set */
							while(list[(i = *b++)])
								*cp++ = i;
						}
						*cp = '\0';
						cnvcount++;
						b--;
						f = save;	/* restore list[] to all zeroes (faster than memset(list,0,256)) */
						if(*f == '^')
							f++;
						if(*f == ']')
						{
							list[']'] = 0;
							f++;
						}
						while((i = *f++) != ']')
							list[i] = 0;
						break;
					case '%':
						if(*b++ != '%')
							return(0);
						break;
					default:
						return(0);
				}
				break;
			default:
				if(c != ' ' && c != '\t')
					if(*b++ != c)
						return(0);
		}
	return(cnvcount);
}

static FILE *my_fp;
static Uchar ungetbuf[8];
static Uchar *ungetp;
static Int getcount;

/******************************************************************************\
|Routine: my_fgetc
|Callby: libc
|Purpose: Does what fgetc does.
|Arguments:
|	fp is the file pointer.
\******************************************************************************/
Int my_fgetc()
{
	getcount++;
	if(ungetp != ungetbuf)
		return(*--ungetp);
	return(fgetc(my_fp));
}

/******************************************************************************\
|Routine: my_ungetc
|Callby: libc
|Purpose: Does what ungetc does.
|Arguments:
|	c is the ungot character.
\******************************************************************************/
void my_ungetc(c)
Int c;
{
	getcount--;
	*ungetp++ = c;
}

/******************************************************************************\
|Routine: my_fscanf
|Callby: help_get_kw init_term journal restore_par store_param
|Purpose: Does what fscanf does.
|Arguments:
|	pfp is the file pointer.
|	fmt is the format string.
\******************************************************************************/
#ifdef VMS
Int my_fscanf(pfp,fmt)
FILE *pfp;
Char *fmt;
#else
Int my_fscanf(va_alist)
va_dcl
#endif
{
	va_list ap;
#ifndef VMS
	FILE *pfp;
	Char *fmt;
#endif
	Uchar *f,*save;
	unsigned long ival;
	double dval;
	Int digcount,i,j,c;
	Int suppress,width,size,sign,unsg,not,cnvcount,decimals,pastpoint,esign,edigcount,signpresent,pointpresent,save_e;
	Char *cp;
	short *sp;
	Int *ip;
	long *lp;
	float *fp;
	double *dp;
	void **vp;

#ifdef VMS
	va_start(ap,fmt);
#else
	va_start(ap);
	pfp = va_arg(ap,FILE *);
	fmt = va_arg(ap,Char *);
#endif
	getcount = 0;
	my_fp = pfp;
	ungetp = ungetbuf;
	f = (Uchar *)fmt;
	cnvcount = 0;
	while((c = *f++))
		switch(c)
		{
			case '%':
				if(*f != '[' && *f != 'n' && *f != 'c' && *f != '%')
				{
					while(1)
					{
						if((i = my_fgetc()) == EOF)
							return(EOF);
						if(!isspace(i))
							break;
					}
					my_ungetc(i);
				}
				suppress = 0;	/* suppression of assignment */
				width = 0;	/* enforced field maximum width (zero if no limit) */
				size = 0;	/* -1 = short, 0 = int, +1 = long, (or -1 or 0 = float, 1 = double, 2 = long double) */
				sign = 1;	/* sign of result */
				unsg = 0;	/* require unsigned int value */
redo:
				switch((c = *f++))
				{
					case '*':
						suppress = 1;
						goto redo;
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						width = c - '0';
						while(isdigit((c = *f++)))
							width = 10 * width + c - '0';
						f--;
						goto redo;
					case 'h':
						size = -1;
						goto redo;
					case 'l':
						size = 1;
						goto redo;
					case 'L':
						size = 2;
						goto redo;
					case 'd':
dec:
						ival = 0;
						digcount = 0;
						if((i = my_fgetc()) == EOF)
							return(EOF);
						else if(i == '-')
						{
							if(unsg)
								return(0);
							sign = -1;
						}
						else if(i != '+')
							my_ungetc(i);
						while(1)
						{
							if((i = my_fgetc()) == EOF)
								return(EOF);
							if(!isdigit(i))
								break;
							ival = 10 * ival + i - '0';
							++digcount;
							if(width)
								if(digcount >= width)
								{
									if((i = my_fgetc()) == EOF)	/* because of the my_ungetc() below */
										return(EOF);
									break;
								}
						}
integer:
						if(!digcount)
							return(0);
						my_ungetc(i);
						if(!suppress)
						{
							if(sign < 0)
								ival = -ival;
							switch(size)
							{
								case -1:
									sp = va_arg(ap,short *);
									*sp = ival;
									break;
								case 0:
									ip = va_arg(ap,Int *);
									*ip = ival;
									break;
								case 1:
								case 2:
									lp = va_arg(ap,long *);
									*lp = ival;
									break;
							}
							cnvcount++;
						}
						break;
					case 'i':
						if((i = my_fgetc()) == EOF)
							return(EOF);
						else if(i == '0')
						{
							if((i = my_fgetc()) == EOF)
								return(EOF);
							if(i == 'x' || i == 'X')
								goto hex;
							else
							{
								my_ungetc(i);
								goto oct;
							}
						}
						my_ungetc(i);
						goto dec;
					case 'o':
oct:
						ival = 0;
						digcount = 0;
						while(1)
						{
							if((i = my_fgetc()) == EOF)
								return(EOF);
							if(!isdigit(i))
								break;
							if(i == '8' || i == '9')
								return(0);
							ival = (ival << 3) + i - '0';
							digcount++;
							if(width)
								if(digcount >= width)
								{
									if((i = my_fgetc()) == EOF)	/* because of the my_ungetc() below integer: */
										return(EOF);
									break;
								}
						}
						goto integer;
					case 'u':
						unsg = 1;
						goto dec;
					case 'x':
hex:
						ival = 0;
						digcount = 0;
						while(1)
						{
							if((i = my_fgetc()) == EOF)
								return(EOF);
							if(!isxdigit(i))
								break;
							ival <<= 4;
							if(i >= '0' && i <= '9')
								ival += i - '0';
							else if(i >= 'A' && i <= 'F')
								ival += i - 'A' + 10;
							else if(i >= 'a' && i <= 'f')
								ival += i - 'a' + 10;
							digcount++;
							if(width)
								if(digcount >= width)
								{
									if((i = my_fgetc()) == EOF)	/* because of the my_ungetc() below integer: */
										return(EOF);
									break;
								}
						}
						goto integer;
					case 'c':
						cp = va_arg(ap,Char *);
						if((i = my_fgetc()) == EOF)
							return(EOF);
						*cp = i;
						cnvcount++;
						break;
					case 's':
						cp = va_arg(ap,Char *);
						while(1)
						{
							if((i = my_fgetc()) == EOF)
								return(EOF);
							if(isspace(i))
								break;
							*cp++ = i;
						}
						*cp = '\0';
						my_ungetc(i);
						cnvcount++;
						break;
					case 'e':
					case 'f':
					case 'g':
						ival = 0;	/* this is the exponent */
						dval = 0.0;
						digcount = 0;
						signpresent = 0;
						pointpresent = 0;
						if((i = my_fgetc()) == EOF)
							return(EOF);
						else if(i == '-')
						{
							sign = -1;
							digcount++;
							signpresent = 1;
						}
						else if(i == '+')
						{
							digcount++;
							signpresent = 1;
						}
						else
							my_ungetc(i);
						decimals = 0;
						pastpoint = 0;
floatdigits:
						while(1)
						{
							if((i = my_fgetc()) == EOF)
								return(EOF);
							if(!isdigit(i))
								break;
							if(pastpoint)
								decimals++;
							dval = 10.0 * dval + (double)(i - '0');
							++digcount;
							if(width)
								if(digcount >= width)
								{
									if((i = my_fgetc()) == EOF)	/* because of the my_ungetc() below */
										return(EOF);
									goto floating;
								}
						}
						if(i == '.')
						{
							pointpresent = 1;
							digcount++;
							if(pastpoint)
								goto floating;
							pastpoint = 1;
							if(width)
								if(digcount >= width)
								{
									if((i = my_fgetc()) == EOF)	/* because of the my_ungetc() below */
										return(EOF);
									goto floating;
								}
							goto floatdigits;
						}
						if(!(digcount - signpresent - pointpresent))
							return(0);
						if(i == 'e' || i == 'E')
						{
							save_e = i;
							edigcount = 0;
							esign = 1;
							if((i = my_fgetc()) == EOF)
								return(EOF);
							else if(i == '-')
								esign = -1;
							else if(i != '+')
								my_ungetc(i);
							while(1)
							{
								if((i = my_fgetc()) == EOF)
									return(EOF);
								if(!isdigit(i))
									break;
								ival = 10 * ival + i - '0';
								edigcount++;
							}
							if(!edigcount)	/* assume the e or E is part of a string */
							{
								my_ungetc(save_e);
								goto floating;
							}
						}
floating:
						my_ungetc(i);	/* back up to char that terminated number */
						if(!suppress)
						{
							if(pastpoint)
								dval /= tentothe(decimals);
							if(esign < 0)
								ival = -ival;
							if(ival)
								dval *= tentothe(ival);
							if(sign < 0)
								dval = -dval;
							switch(size)
							{
								case -1:
								case 0:
									fp = va_arg(ap,float *);
									*fp = (float)dval;
									break;
								case 1:
								case 2:
									dp = va_arg(ap,double *);
									*dp = (double)dval;
									break;
							}
							cnvcount++;
						}
						break;
					case 'p':
						if((i = my_fgetc()) == EOF)
							return(EOF);
						if(i != '0')
							return(0);
						if((i = my_fgetc()) == EOF)
							return(EOF);
						if(i != 'x' && i != 'X')
							return(0);
						ival = 0;
						for(j = (sizeof(void *) << 1);j--;)
						{
							if((i = my_fgetc()) == EOF)
								return(EOF);
							if(!isxdigit((i)))
								return(0);
							ival <<= 4;
							if(i >= '0' && i <= '9')
								ival += i - '0';
							else if(i >= 'A' && i <= 'F')
								ival += i - 'A' + 10;
							else if(i >= 'a' && i <= 'f')
								ival += i - 'a' + 10;
						}
						vp = va_arg(ap,void **);	/* this assumes that char * and void * are the same size */
						*vp = (void *)ival;
						cnvcount++;
						break;
					case 'n':
						if(suppress || width || size)
							return(0);
						ip = va_arg(ap,Int *);
						*ip = getcount;
						break;
					case '[':
						save = f;
						not = 0;
						if(*f == '^')
						{
							not = 1;
							f++;
						}
						if(*f == ']')
						{
							list[']'] = 1;
							f++;
						}
						while((i = *f++) != ']')
							list[i] = 1;
						cp = va_arg(ap,Char *);
						if(not)
						{
							while(1)
							{
								if((i = my_fgetc()) == EOF)
									return(EOF);
								if(!list[i])	/* find a non-member of the set */
									break;
							}
							while(1)
							{
								if((i = my_fgetc()) == EOF)
									return(EOF);
								if(list[i])
									break;
								*cp++ = i;
							}
						}
						else
						{
							while(1)
							{
								if((i = my_fgetc()) == EOF)
									return(EOF);
								if(list[i])	/* find a member of the set */
									break;
							}
							while(1)
							{
								if((i = my_fgetc()) == EOF)
									return(EOF);
								if(!list[i])
									break;
								*cp++ = i;
							}
						}
						*cp = '\0';
						cnvcount++;
						my_ungetc(i);
						f = save;	/* restore list[] to all zeroes (faster than memset(list,0,256)) */
						if(*f == '^')
							f++;
						if(*f == ']')
						{
							list[']'] = 0;
							f++;
						}
						while((i = *f++) != ']')
							list[i] = 0;
						break;
					case '%':
						if((i = my_fgetc()) == EOF)
							return(EOF);
						if(i != '%')
							return(0);
						break;
					default:
						return(0);
				}
				break;
			default:
				if(c != ' ' && c != '\t')
				{
					if((i = my_fgetc()) == EOF)
						return(EOF);
					if(i != c)
						return(0);
				}
		}
	return(cnvcount);
}

