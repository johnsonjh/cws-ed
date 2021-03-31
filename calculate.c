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
#include <stddef.h>
#include <string.h>
#include <math.h>

#include "ctyp_dec.h"

#ifndef PI
#define PI 3.14159265
#endif
#define CONV (PI / 180.0)

#ifndef HUGE_VAL           /* ANSI type definition conversion */
#define HUGE_VAL HUGE
#endif

/******************************************************************************\
|Routine: calculate
|Callby: command
|Purpose: Implements a basic calculator.
|Arguments:
|    buf is a buffer containing an algebraic expression.
|    result is the returned buffer containing the answer, or an error message.
\******************************************************************************/
Int calculate(pbuf,result)
Char *pbuf;
Char *result;
{
	static Char *buf = NULL;
	static Int bufl = 0;
	static Int maxtokens = 0;
	static Int *token = NULL;
	static double *value,*real_value;
	static Char *intrinsic[] =
	{
		"sinh","cosh","tanh",
		"sind","cosd","tand",
		"cotd","secd","cscd",
		"sin","cos","tan",
		"cot","sec","csc",
		"arcsind","arccosd","arctand",
		"arcsin","arccos","arctan",
		"sqrt","fact","exp","ln","log","int","abs",
	};

	Char *p,*q,c;
	double val;
	Int i,j,ival;
	Int ntokens,nintr,depth,lowest,first,second,operator;

/* ignore empty expressions */
	if(!pbuf)
		goto empty;
/* expand the local buffer if necessary */
	if((i = strlen(pbuf) + 2) > bufl)
	{
		if(buf)
			ifree(buf);
		buf = (Char *)imalloc((bufl = i) * sizeof(Char));
	}
/* copy to local buffer, making it lowercase */
	for(p = pbuf,q = buf;(c = *p) && c != '=';p++)
		*q++ = tolower(c);
	*q++ = ' ';
	*q = '\0';
/* get the number of tokens */
	nintr = sizeof(intrinsic)/sizeof(intrinsic[0]);	/* number of intrinsic functions */
	for(ntokens = 0,p = buf;(c = *p);p++)
		switch(c)
		{
			case '(': case ')': case '+': case '-': case '*': case '/': case '^':
				ntokens++;
				break;
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '.':
				if(my_sscanf(p,"%Lf%n",&val,&i) != 1)
				{
					strcpy(result,"Bad number.");
					return(0);
				}
				p += i - 1;
				ntokens++;
				break;
			case ' ': case '\t':
				break;
			default:
				for(i = 0;i < nintr;i++)
				{
					if(!strncmp(p,intrinsic[i],strlen(intrinsic[i])))
					{
						ntokens++;
						break;
					}
				}
				if(i == nintr)
				{
					strcpy(result,"Bad function.");
					return(0);
				}
				p += strlen(intrinsic[i]) - 1;
				ntokens++;
		}
/* this is an entry point for a spot below where closing parens need to be added to the token array */
expand_tokens:
	if((ntokens += 2) > maxtokens)	/* add two for enclosing parens */
	{
		if(token)
		{
			ifree(token);
			ifree(real_value);
		}
		maxtokens = ntokens;
		token = (Int *)imalloc(maxtokens * sizeof(int));
/* value is a double-aligned version of real_value. real_value is kept around for later freeing */
		real_value = (double *)imalloc((maxtokens + 1) * sizeof(double));
		value = (double *)((((size_t)real_value) + 7) & ~7);
	}
	token[0] = '(';	/* enclose entire expression in parens */
	ntokens = 1;	/* first paren is first token */
/*
	convert string to tokens.
	token[i] = 0 if token is a number. In this case, value[i] is the numeric value.
	token[i] = <character> if token is an operator <+ - * / ( )>.
	token[i] = -n if token is intrinsic function #n (starting with #1).
*/
	for(p = buf;(c = *p);p++)
		switch(c)
		{
			case '(': case ')': case '+': case '-': case '*': case '/': case '^':
				token[ntokens++] = c;
				break;
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '.':
				token[ntokens] = 0;
				my_sscanf(p,"%Lf%n",value + ntokens++,&i);
				p += i - 1;
				break;
			case ' ': case '\t':
				break;
			default:
				for(i = 0;i < nintr;i++)
				{
					if(!strncmp(p,intrinsic[i],strlen(intrinsic[i])))
					{
						token[ntokens++] = -1 - i;
						break;
					}
				}
				p += strlen(intrinsic[i]) - 1;
		}
/* finish with enclosing parenthesis */
	token[ntokens++] = ')';
/* now process the tokens, always going to deepest paren depth */
	while(ntokens > 1)
	{
		for(lowest = depth = i = 0;i < ntokens;i++)
		{
			if(token[i] == '(')
			{
				if(++depth > lowest)
				{
					lowest = depth;
					first = i;
				}
			}
			else if(token[i] == ')')
			{
				if(--depth < 0)
				{
					strcpy(result,"Bad parentheses.");
					return(0);
				}
			}
		}
/* make parentheses balance if they don't */
		if(depth)
		{
			if(ntokens + depth > maxtokens)
			{
				ntokens += depth;
				goto expand_tokens;
			}
			while(depth-- > 0)
				token[ntokens++] = ')';
			continue;
		}
/* find closure of lowest expression */
		for(second = first + 1;token[second] != ')';second++);
/* evaluate the expression */
		if((i = first + 1) == second)
			value[first] = 0;
		else
		{
			if(token[i] == '-')
			{
				val = -value[++i];
				i++;
			}
			else
				val = value[i++];
			while(i < second)
			{
				switch(token[i++])
				{
					case 0:
						strcpy(result,"Two numbers with no operator between.");
						return(0);
					case '+':
						operator = 0;
						break;
					case '-':
						operator = 1;
						break;
					case '*':
						operator = 2;
						break;
					case '/':
						operator = 3;
						break;
					case '^':
						operator = 4;
						break;
					default:
						operator = token[i-1];
						break;
				}
				if(i == second)
				{
					strcpy(result,"Operator not followed by number.");
					return(0);
				}
				switch(token[i])
				{
					case 0:
						switch(operator)
						{
							case 0:
								val += value[i];
								break;
							case 1:
								val -= value[i];
								break;
							case 2:
								val *= value[i];
								break;
							case 3:
								if(!value[i])
								{
									strcpy(result,"Divide by zero.");
									return(0);
								}
								val /= value[i];
								break;
							case 4:
								if((val == 0 && value[i] <= 0) || (val < 0 && floor(value[i]) != value[i]))
								{
									strcpy(result,"Illegal exponentiation.");
									return(0);
								}
								val = pow(val,value[i]);
								break;
						}
						break;
					default:
						strcpy(result,"Two operators with no number between.");
						return(0);
				}
				i++;
			}
/* check for preceeding intrinsic function(s) */
			i = first;
			while(i > 0)
			{
				if(token[i - 1] >= 0)
					break;
				switch(-token[--i])
				{
					case 1:
						if(fabs(val) > log(HUGE_VAL))
						{
							strcpy(result,"Bad hyperbolic sine.");
							return(0);
						}
						val = sinh(val);
						break;
					case 2:
						if(fabs(val) > log(HUGE_VAL))
						{
							strcpy(result,"Bad hyperbolic cosine.");
							return(0);
						}
						val = cosh(val);
						break;
					case 3:
						val = tanh(val);
						break;
					case 4:
						val = sin(CONV * val);
						break;
					case 5:
						val = cos(CONV * val);
						break;
					case 6:
						if(val - (double)(90 * (int)(val/90.0)) == 0 && (int)(val/90.0) & 1)
						{
							strcpy(result,"Bad tangent.");
							return(0);
						}
						val = tan(CONV * val);
						break;
					case 7:
						if(val - (double)(90 * (int)(val/90.0)) == 0 && !((int)(val/90.0) & 1))
						{
							strcpy(result,"Bad cotangent.");
							return(0);
						}
						val = 1.0/tan(CONV * val);
						break;
					case 8:
						if(val - (double)(90 * (int)(val/90.0)) == 0 && (int)(val/90.0) & 1)
						{
							strcpy(result,"Bad secant.");
							return(0);
						}
						val = 1.0/cos(CONV * val);
						break;
					case 9:
						if(val - (double)(90 * (int)(val/90.0)) == 0 && !((int)(val/90.0) & 1))
						{
							strcpy(result,"Bad cosecant.");
							return(0);
						}
						val = 1.0/sin(CONV * val);
						break;
					case 10:
						val = sin(val);
						break;
					case 11:
						val = cos(val);
						break;
					case 12:
						if(val - (double)(90 * (int)(val/90.0)) == 0 && (int)(val/90.0) & 1)
						{
							strcpy(result,"Bad tangent.");
							return(0);
						}
						val = tan(val);
						break;
					case 13:
						if(val - (double)(90 * (int)(val/90.0)) == 0 && !((int)(val/90.0) & 1))
						{
							strcpy(result,"Bad cotangent.");
							return(0);
						}
						val = 1.0/tan(val);
						break;
					case 14:
						if(val - (double)(90 * (int)(val/90.0)) == 0 && (int)(val/90.0) & 1)
						{
							strcpy(result,"Bad secant.");
							return(0);
						}
						val = 1.0/cos(val);
						break;
					case 15:
						if(val - (double)(90 * (int)(val/90.0)) == 0 && !((int)(val/90.0) & 1))
						{
							strcpy(result,"Bad cosecant.");
							return(0);
						}
						val = 1.0/sin(val);
						break;
					case 16:
						if(val < -1.0 || val > 1.0)
						{
							strcpy(result,"Bad arcsin.");
							return(0);
						}
						val = asin(val) / CONV;
						break;
					case 17:
						if(val < -1.0 || val > 1.0)
						{
							strcpy(result,"Bad arccos.");
							return(0);
						}
						val = acos(val) / CONV;
						break;
					case 18:
						val = atan(val) / CONV;
						break;
					case 19:
						if(val < -1.0 || val > 1.0)
						{
							strcpy(result,"Bad arcsin.");
							return(0);
						}
						val = asin(val);
						break;
					case 20:
						if(val < -1.0 || val > 1.0)
						{
							strcpy(result,"Bad arccos.");
							return(0);
						}
						val = acos(val);
						break;
					case 21:
						val = atan(val);
						break;
					case 22:
						if(val < 0.0)
						{
							strcpy(result,"Square root of negative value.");
							return(0);
						}
						val = sqrt(val);
						break;
					case 23:
						if(val <= -1.0)
						{
							strcpy(result,"Factorial of negative value.");
							return(0);
						}
						else if(val >= 34.0)
						{
							strcpy(result,"Factorial of value that is too large.");
							return(0);
						}
						for(ival = (int)val,val = 1.0;ival > 0;ival--)
							val *= (double)ival;
						break;
					case 24:
						if(fabs(val) > log(HUGE_VAL))
						{
							strcpy(result,"Bad exponentiation.");
							return(0);
						}
						val = exp(val);
						break;
					case 25:
						if(val <= 0.0)
						{
							strcpy(result,"Log of non-positive value.");
							return(0);
						}
						val = log(val);
						break;
					case 26:
						if(val <= 0.0)
						{
							strcpy(result,"Log(base 10) of non-positive value.");
							return(0);
						}
						val = log10(val);
						break;
					case 27:
						val = (double)((int)val);
						break;
					case 28:
						val = (val < 0.0)? -val : val;
						break;
				}	/* end of intrinsic switch */
			}	/* end of while */
			first = i;
		}	/* end of if(second = first + 1) */
/* replace token[first] with 0, value[first] with val, squeeze out everything up to second inclusive */
		token[first] = 0;
		value[first] = val;
		for(i = second + 1,j = first + 1;i < ntokens;i++,j++)
		{
			token[i - second + first] = token[i];
			value[i - second + first] = value[i];
		}
		ntokens = j;
/* go for another pass */
	}	/* end of while(ntokens > 1) */
/* report results */
	sprintf(result,"%f",value[0]);
	return(1);
empty:
	strcpy(result,"Nothing to calculate.");
	return(0);
}

