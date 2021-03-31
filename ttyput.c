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

extern void cleanup();
static Int restored = 1;	/* flag to control behavior of ttyend(). it does restoration only if restored == 0. */

#ifdef VMS
#include <ssdef.h>
#include <ttdef.h>
#include <iodef.h>

static short chan = 0;
extern void ttyend();
static int desblk[4] = {0,(int)ttyend,0,(int)&desblk[3]};
static Char sysinput[] = {"SYS$COMMAND"};
static struct {unsigned short length,class;Char *string;} desc = {11,0x010e,sysinput};
typedef struct mode_str *mode_ptr;
typedef struct mode_str
{
	Uchar class,type;
	unsigned short buffersize;
	union
	{
		unsigned int characteristics;
		Uchar page[4];
	} u;
	unsigned int extended;
} mode_node;
static mode_node old_mode;
static unsigned int newmask = 0x02100000,oldmask;	/* disable interrupts */
static Char ctrlc_typed = 0;

/******************************************************************************\
|Routine: ttysetsize
|Callby: init_term
|Purpose: Handles defaults for terminal size.
|Arguments:
|    NROW,NCOL - the number of rows and columns on the screen.
\******************************************************************************/
void ttysetsize(nrow,ncol)
Int *nrow,*ncol;
{
	unsigned short tmprow, tmpcol;

	if(!*nrow || !*ncol)
	{
		tmprow = 24;
		tmpcol = 80;
		if(old_mode.class == 66) /* Terminal */
		{
			if(old_mode.u.page[3] > 1)
				tmprow = old_mode.u.page[3];
			tmpcol = old_mode.buffersize;
		}
		if(!*nrow)
			*nrow = tmprow;
		if(!*ncol)
			*ncol = tmpcol;
	}

}

/******************************************************************************\
|Routine: ttygetsb
|Callby: put
|Purpose: Null routine.
|Arguments:
|    ptr - physical video frame buffer base pointer.
\******************************************************************************/
void ttygetsb(ptr)
short **ptr;
{
	printf("You can't use this terminal configuration file with this image.\r\n");
	cleanup(-1);
}

/******************************************************************************\
|Routine: ttyscreen
|Callby: put
|Purpose: Null routine.
|Arguments:
|    row, col - where to position the cursor on the screen, zero based.
\******************************************************************************/
void ttyscreen(row,col)
Int row,col;
{
}

/******************************************************************************\
|Routine: ring_bell
|Callby: init_term
|Purpose: Handles special bell-ringing for brain-dead terminals.
|Arguments:
|    none
\******************************************************************************/
void ring_bell()
{
}

/******************************************************************************\
|Routine: ttyend
|Callby: init_term ttyput
|Purpose: Terminates connection with the terminal.
|Arguments:
|    none
\******************************************************************************/
void ttyend()
{
	unsigned short iosb[4],temp;
	
	if(temp = chan)
	{
		chan = 0;
		SYS$CANEXH(desblk);
		SYS$QIOW(0,temp,IO$_SETMODE,iosb,0,0,&old_mode,12,0,0,0,0);
		SYS$DASSGN(temp);
		LIB$ENABLE_CTRL(&oldmask);
	}
}

/******************************************************************************\
|Routine: ttyioerr
|Callby: ttyput
|Purpose: Handles i/o errors with the terminal.
|Arguments:
|    status is the VMS status of the i/o.
|    iosb is the VMS i/o status block.
\******************************************************************************/
void ttyioerr(status,iosb)
Int status;
unsigned short iosb;
{
	if(!(status & 1))
	{
		if(status != SS$_TIMEOUT)
			goto report_error;
	}
	else
	{
		if(!((status = iosb) & 1))
		{
			if(status != SS$_TIMEOUT)
				goto report_error;
		}
	}
	return;
report_error: 
	printf("An I/O error occurred (error #%08X).\r\n",status);
	cleanup(-1);
}

/******************************************************************************\
|Routine: ttyput
|Callby: put
|Purpose: Puts characters to the terminal.
|Arguments:
|    c is the string to send to the terminal.
\******************************************************************************/
void ttyput(c)
Char *c;
{
	Int i,status;
	unsigned short iosb[4];

	if(!(i = strlen(c)))
		return;
	while(i > 64)
	{
		status = SYS$QIOW(0,chan,IO$_WRITEVBLK | IO$M_NOFORMAT,iosb,0,0,c,64,0,0,0,0);
		ttyioerr(status,iosb[0]);
		c += 64;
		i -= 64;
	}
	if(i)
	{
		status = SYS$QIOW(0,chan,IO$_WRITEVBLK | IO$M_NOFORMAT,iosb,0,0,c,i,0,0,0,0);
		ttyioerr(status,iosb[0]);
	}
}

/******************************************************************************\
|Routine: ttychk
|Callby: load_file
|Purpose: Null routine.
|Arguments:
|    none
\******************************************************************************/
Int ttychk()
{
	if(ctrlc_typed)
	{
		ctrlc_typed = 0;
		return(3);
	}
	return(0);
}

/******************************************************************************\
|Routine: setctrlc
|Callby: ttystart
|Purpose: AST routine to set the ctrlc-has-been-typed flag.
|Arguments:
|    none
\******************************************************************************/
void setctrlc()
{
	unsigned short iosb[4];
	
	ctrlc_typed = 1;
	SYS$QIOW(0,chan,IO$_SETMODE | IO$M_CTRLCAST,iosb,0,0,setctrlc,0,0,0,0,0);
}

/******************************************************************************\
|Routine: ttyget
|Callby: cfg init_term
|Purpose: Gets a character from the terminal.
|Arguments:
|    c is the returned character.
\******************************************************************************/
void ttyget(c)
Schar *c;
{
	static Char buf[128],*next = buf;
	static int terms[8] = {0,0,0,0,0,0,0,0};
	static struct {int nbytes,*terminators;} term_desc = {0,terms};
	static Int remain = 0;
	Int status;
	unsigned short iosb[4];

	if(!remain)
	{
		status = SYS$QIOW(0,chan,IO$_TTYREADALL | IO$M_NOECHO | IO$M_NOFILTR | IO$M_TIMED,iosb,0,0,buf,sizeof(buf) - 3,0,&term_desc,0,0);
		ttyioerr(status,iosb[0]);
		if(!(remain = iosb[1]))
		{
			status = SYS$QIOW(0,chan,IO$_TTYREADALL | IO$M_NOECHO,iosb,0,0,c,1,0,0,0,0);
			ttyioerr(status,iosb[0]);
			return;
		}
		else
			next = buf;
	}
	*c = *((Schar *)next++);
	remain--;
}

/******************************************************************************\
|Routine: ttystart
|Callby: init_term
|Purpose: Initializes communication with the terminal.
|Arguments:
|    none
\******************************************************************************/
void ttystart()
{
	mode_node new_mode;
	Int status;
	unsigned short iosb[4];

	if(!chan)
	{
		LIB$DISABLE_CTRL(&newmask,&oldmask);
		status = SYS$ASSIGN(&desc,&chan,0,0);
		if(!(status & 1))
		{
			printf("Unable to assign a channel to SYS$COMMAND (error #%08X).\r\n",status);
			exit(-1);
		}
		old_mode.class = 0;			/* Just in case it fails */
		SYS$QIOW(0,chan,IO$_SENSEMODE,iosb,0,0,&old_mode,12,0,0,0,0);
		new_mode = old_mode;
		new_mode.u.characteristics |= TT$M_HOSTSYNC|TT$M_TTSYNC|TT$M_NOBRDCST|TT$M_NOECHO;
		SYS$QIOW(0,chan,IO$_SETMODE,iosb,0,0,&new_mode,12,0,0,0,0);
		if(!((status = SYS$DCLEXH(desblk)) & 1))
		{
			ttyend();
			printf("Unable to declare exit handler, status = %08X\n",status);
			cleanup(0);
		}
		SYS$QIOW(0,chan,IO$_SETMODE | IO$M_CTRLCAST,iosb,0,0,setctrlc,0,0,0,0,0);
	}
}

#else	/* VMS */

#ifdef WIN32	/* Windows NT */
#include <windows.h>

static short *screen_base = 0;		/* pointer to PC screen area */
static CHAR_INFO *pchar_base = 0;	/* pointer to pchiSrcBuffer */
static unsigned short lenLVB = 0;	/* length of screen */
static COORD coordScreen;			/* Size of Screen */
static HANDLE hOut,hIn;				/* Screen & Keyboard Handles */
static DWORD dwMode;				/* terminal input settings */

/******************************************************************************\
|Routine: ttygetsb
|Callby: put
|Purpose: Returns video buffer base.
|Arguments:
|    ptr - logical video frame buffer base pointer.
\******************************************************************************/
void ttygetsb(ptr)
short **ptr;
{
	*ptr = screen_base;
}

/******************************************************************************\
|Routine: ttysetsize
|Callby: init_term
|Purpose: Handles defaults for terminal size.
|Arguments:
|    nrow, ncol - the number of rows and columns on the screen.
\******************************************************************************/
void ttysetsize(nrow,ncol)
Int *nrow,*ncol;
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	BOOL bSuccess;

	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if(hOut == INVALID_HANDLE_VALUE)
	{
		printf("GetStdHandle failed %x\n",GetLastError());
		cleanup(-1);
	}

/* Screen resizing not implemented - to be done */
/*	bSuccess = GetConsoleScreenBufferInfo(hOut,&csbi);
	change = 0;
	if(*ncol > csbi.dwSize.X)
	{
		change = 1;
		coordScreen.X = *ncol;
	} else
		coordScreen.X = csbi.dwSize.X;
	if(*nrow > csbi.dwSize.Y)
	{
		change = 1;
		coordScreen.Y = *nrow;
	} else
		coordScreen.Y = csbi.dwSize.Y;
	if(change)
	{
		bSuccess = SetConsoleScreenBufferInfo(hOut,&coordScreen);
	}
	*/
	bSuccess = GetConsoleScreenBufferInfo(hOut,&csbi);
	if(!bSuccess)
	{
		printf("GetConsoleScreenBufferInfo failed %x\n",GetLastError());
		csbi.dwSize.X = 80;
		csbi.dwSize.Y = 25;
	}
	*ncol = coordScreen.X = csbi.dwSize.X;
	*nrow = coordScreen.Y = csbi.dwSize.Y;

	lenLVB = *nrow * *ncol;
	screen_base = (short *)imalloc(lenLVB * sizeof(short));
	pchar_base = (CHAR_INFO *)imalloc(lenLVB * sizeof(CHAR_INFO));
}

/******************************************************************************\
|Routine: ttyscreen
|Callby: put
|Purpose: Updates screen buffer and positions cursor (BRAINDEAD only).
|Arguments:
|    row, col - where to position the cursor on the screen, zero based.
\******************************************************************************/
void ttyscreen(row,col)
Int row,col;
{
	int i;
	COORD coord;
	SMALL_RECT srec;

	for(i = 0;i < lenLVB;i++)
	{
		pchar_base[i].Char.AsciiChar = screen_base[i] & 0xff;
		pchar_base[i].Attributes = screen_base[i] >> 8;
	}
	coord.X = coord.Y = 0;
	srec.Left = srec.Top = 0;
	srec.Right = coordScreen.X - 1;
	srec.Bottom = coordScreen.Y - 1;
	if(!WriteConsoleOutput(hOut,pchar_base,coordScreen,coord,&srec))
	{
		printf("WriteConsoleOutput failed %x\n",GetLastError());
		cleanup(-1);
	}
	coord.X = col;
	coord.Y = row;
	if(!SetConsoleCursorPosition(hOut,coord))
	{
		printf("SetConsoleCursorPosition failed %x\n",GetLastError());
		cleanup(-1);
	}
}

/******************************************************************************\
|Routine: ring_bell
|Callby: init_term
|Purpose: Handles special bell-ringing for BRAINDEAD terminals.
|Arguments:
|    none
\******************************************************************************/
void ring_bell()
{
	Beep(320,100);
}

/******************************************************************************\
|Routine: ttyput
|Callby: put
|Purpose: Puts characters to the terminal.
|Arguments:
|    c is the string to send to the terminal.
\******************************************************************************/
void ttyput(c)
Char *c;
{
}

/******************************************************************************\
|Routine: ttychk
|Callby: load_file
|Purpose: Null routine.
|Arguments:
|    none
\******************************************************************************/
Int ttychk()
{
	return(0);
}

/******************************************************************************\
|Routine: ttyget
|Callby: cfg init_term
|Purpose: Gets a character from the terminal.
|Arguments:
|    c is the returned character.
\******************************************************************************/
void ttyget(c)
Schar *c;
{
	static Char buf[2],*next = buf;
	static Int remain = 0;
	short shift;
	Int evtcode,scan,asci;
	Uchar thischar;
	INPUT_RECORD pirBuf;
	DWORD nr;	/* number of events actually read */

	while(!remain)
	{
		next = buf;

		do
			if(!ReadConsoleInput(hIn,&pirBuf,1,&nr))
			{
				printf("ReadConsoleInput failed %x\n",GetLastError());
				cleanup(-1);
			}
		while(pirBuf.EventType != KEY_EVENT || !pirBuf.Event.KeyEvent.bKeyDown);

		shift = pirBuf.Event.KeyEvent.dwControlKeyState;
		scan = pirBuf.Event.KeyEvent.wVirtualScanCode;
		asci = pirBuf.Event.KeyEvent.uChar.AsciiChar;

		evtcode = (scan << 8) | asci;
		if(evtcode == 0x0e08)
			thischar = 127;		/* Rubout */
		else if(evtcode == 0x352f && shift & ENHANCED_KEY)
			thischar = 0xbe;	/* PF2 fixup */
		else if(evtcode == 0x1c0d && shift & ENHANCED_KEY)
			thischar = 0xbd;	/* KpEnter */
		else if( (asci > 0) && (scan <= 0x35) )
		{
			/* If Caps Lock on then it is Caps! */
			if((CAPSLOCK_ON & shift) && 
				(asci >= 'a') && (asci <= 'z') )
					asci = asci - 0x20;
			thischar = asci;
		}
		else if(scan == 0x39)
			thischar = ' ';		/* Space */
		/* 13 characters between 0x00 and 0x0c (Keypad & Arrows) */
		else if( (scan >= 0x47) && (scan <= 0x53) )
		{
			if( shift & ENHANCED_KEY )
				thischar = 0xa0 + (scan - 0x47);
			else
				thischar = 0xb0 + (scan - 0x47);
		}
		else if( evtcode == 0x372a )
			thischar = 0xbf;	/* PF3 */
		else if( evtcode == 0x0300 )
			thischar = 0;		/* Null */
		/* 10 characters between 0x00 and 0x09 (F1 to F10) */
		else if( (scan >= 0x3b) && (scan <= 0x44) )
			thischar = 0xc0 + (scan - 0x3b);
		else if( evtcode == 0x5700 ) /* 8500 */
			thischar = 0xca;	/* F11 */
		else if( evtcode == 0x5800 ) /* 8600 */
			thischar = 0xcb;	/* F12 */
		else if( evtcode == 0x4500 )
			thischar = 0xaf;	/* NumLock = GOLD */
		else
			thischar = 0xff;		/* indicate not set */

		if(thischar >= 0xb0 && thischar <= 0xbf) /* shift special */
			if(SHIFT_PRESSED & shift) thischar -= 0x10;

		if(thischar != 0xff)
		{
			*next++ = thischar;
			remain++;
		}
		next = buf;
	}
	*c = *((Schar *)next++);
	remain--;
	return;
}

/******************************************************************************\
|Routine: ttyend
|Callby: init_term
|Purpose: Terminates connection with the terminal.
|Arguments:
|    none
\******************************************************************************/
void ttyend()	/* this is a handler that restores the terminal settings at image exit */
{
	if(restored++)
		return;
	if(!SetConsoleMode(hIn,dwMode))
	{
		printf("SetConsoleMode failed %x\n",GetLastError());
	}
	return;
}

/******************************************************************************\
|Routine: ttystart
|Callby: init_term
|Purpose: Initializes communication with the terminal.
|Arguments:
|    none
\******************************************************************************/
void ttystart()
{
	hIn = GetStdHandle(STD_INPUT_HANDLE);
	if(hIn == INVALID_HANDLE_VALUE)
	{
		printf("GetStdHandle failed %x\n",GetLastError());
		cleanup(-1);
	}
	if(!GetConsoleMode(hIn,&dwMode))
	{
		printf("GetConsoleMode failed %x\n",GetLastError());
		cleanup(-1);
	}
	if(!SetConsoleMode(hIn,(dwMode & ~(ENABLE_LINE_INPUT |
		ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT |
		ENABLE_PROCESSED_INPUT |ENABLE_WINDOW_INPUT) )) )
	{
		printf("SetConsoleMode failed %x\n",GetLastError());
		cleanup(-1);
	}
	restored = 0;
}

#else		/* !WIN32 */

#ifdef GNUOS2
#define  INCL_KBD		/* Define Kbd routines in os2.h */
#define  INCL_VIO		/* Define Vio routines in os2.h */
#define  INCL_NOPM		/* Don't define PM routines in os2.h */
#include <os2.h>
static short *screen_base = 0;	/* pointer to PC screen area */
static short saveshift = 0;	/* shift flags from previous keystroke */
static unsigned short lenLVB = 0; /* length of screen */

/******************************************************************************\
|Routine: ttygetsb
|Callby: put
|Purpose: Returns video buffer base.
|Arguments:
|    ptr - logical video frame buffer base pointer.
\******************************************************************************/
void ttygetsb(ptr)
short **ptr;
{
	*ptr = screen_base;
}

/******************************************************************************\
|Routine: ttysetsize
|Callby: init_term
|Purpose: Handles defaults for terminal size.
|Arguments:
|    nrow, ncol - the number of rows and columns on the screen.
\******************************************************************************/
void ttysetsize(nrow,ncol)
Int *nrow,*ncol;
{
	VIOMODEINFO viomi;

	viomi.cb = sizeof(viomi);
	VioGetMode(&viomi, 0);

	if(*ncol > viomi.col)		/* User wants more columns, set to 132 (max) */
	{
/*		viomi.col = 132; 
		VioSetMode(&viomi, 0);
		VioGetMode(&viomi, 0); */
	}

	if(*nrow > viomi.row)		/* User wants more rows, use 8x8 font */
	{
/*		viomi.row = *nrow; */
		viomi.row = 50;
		VioSetMode(&viomi, 0);
		VioGetMode(&viomi, 0);
	}

	/* Indicate final resolution (may or may not change from above) */
	*nrow = viomi.row;
	*ncol = viomi.col;
	lenLVB = viomi.row * viomi.col * sizeof(short);
	screen_base = (short *)imalloc(lenLVB);
}

/******************************************************************************\
|Routine: ttyscreen
|Callby: put
|Purpose: Updates screen buffer and positions cursor (DOS & OS2 only).
|Arguments:
|    row, col - where to position the cursor on the screen, zero based.
\******************************************************************************/
void ttyscreen(row,col)
Int row,col;
{
	short i;
	
	if((i = VioWrtCellStr((Char *)screen_base, lenLVB, 0, 0, 0)))
	{
		printf("VioWrtCellStr failure %d.\n",i);
		cleanup(-1);
	}
		
	if((i = VioSetCurPos((unsigned short)row, (unsigned short)col, 0)))
	{
		printf("VioSetCurPos failure %d.\n",i);
		cleanup(-1);
	}
}

/******************************************************************************\
|Routine: ring_bell
|Callby: init_term
|Purpose: Handles special bell-ringing for non-terminal PC terminals.
|Arguments:
|    none
\******************************************************************************/
void ring_bell()
{
	DosBeep(320,100);
}

/******************************************************************************\
|Routine: ttyput
|Callby: put
|Purpose: Puts characters to the terminal.
|Arguments:
|    c is the string to send to the terminal.
\******************************************************************************/
void ttyput(c)
Char *c;
{
}

/******************************************************************************\
|Routine: ttychk
|Callby: load_file
|Purpose: Null routine.
|Arguments:
|    none
\******************************************************************************/
Int ttychk()
{
	return(0);
}

/******************************************************************************\
|Routine: ttyget
|Callby: cfg init_term
|Purpose: Gets a character from the terminal.
|Arguments:
|    c is the returned character.
\******************************************************************************/
void ttyget(c)
Schar *c;
{
	static Char buf[2],*next = buf;
	static Int remain = 0;
	short shift;
	Int evtcode,scan,asci;
	Uchar thischar;
	KBDKEYINFO keyinf;

	while(!remain)
	{
		next = buf;
		if(KbdCharIn(&keyinf, IO_WAIT, 0))
		{
			printf("KbdCharIn failed!\n");
			cleanup(-1);
		}
		shift = keyinf.fsState;
		if(NUMLOCK_ON & (shift ^ saveshift))
		{
			*next++ = 0xaf;		/* NumLock */
			remain++;
		}
		saveshift = shift;
		evtcode = (keyinf.chScan << 8) | keyinf.chChar;
		scan = keyinf.chScan;
		asci = keyinf.chChar;
		if( evtcode == 0x0e08 )
			thischar = 127;		/* Rubout */
		else if( (asci > 0) && (scan <= 0x35) )
		{
			/* If Caps Lock on then it is Caps! */
			if((CAPSLOCK_ON & shift) && 
				(asci >= 'a') && (asci <= 'z') )
					asci = asci - 0x20;
			thischar = asci;
		}
		else if( scan == 0x39)
			thischar = ' ';		/* Space */
		/* 13 characters between 0x00 and 0x0c (Keypad & Arrows) */
		else if( (scan >= 0x47) && (scan <= 0x53) )
		{
			if( asci == 0xe0 )
				thischar = 0xa0 + (scan - 0x47);
			else
				thischar = 0xb0 + (scan - 0x47);
		}
		else if( evtcode == 0xe00d )
			thischar = 0xbd;	/* KpEnter */
		else if( evtcode == 0xe02f )
			thischar = 0xbe;	/* PF2 */
		else if( evtcode == 0x372a )
			thischar = 0xbf;	/* PF3 */
		else if( evtcode == 0x0300 )
			thischar = 0;		/* Null */
		/* 10 characters between 0x00 and 0x09 (F1 to F10) */
		else if( (scan >= 0x3b) && (scan <= 0x44) )
			thischar = 0xc0 + (scan - 0x3b);
		else if( evtcode == 0x8500 )
			thischar = 0xca;	/* F11 */
		else if( evtcode == 0x8600 )
			thischar = 0xcb;	/* F12 */
		else
			thischar = 0xff;		/* indicate not set */

		if(thischar >= 0xb0 && thischar <= 0xbf) /* shift special */
			if(KB_SHIFT & shift) thischar -= 0x10;

		if(thischar != 0xff)
		{
			*next++ = thischar;
			remain++;
		}
		next = buf;
	}
	*c = *((Schar *)next++);
	remain--;
	return;
}

/******************************************************************************\
|Routine: ttyend
|Callby: init_term ttyput
|Purpose: Terminates connection with the terminal.
|Arguments:
|    none
\******************************************************************************/
void ttyend()	/* this is a handler that restores the terminal settings at image exit */
{
	if(restored++)
		return;
	return;
}

void ttystart()
{
	KBDINFO kbstInfo;

	kbstInfo.cb = sizeof(kbstInfo);		/* Length of status buffer */
	if(KbdGetStatus(&kbstInfo, 0))
	{
		printf("KbdGetStatus failed!\n");
		cleanup(-1);
	}
	saveshift = kbstInfo.fsState;
	restored = 0;
}

#else		/* !GNUOS2 */

#ifdef GNUDOS
#include <stdlib.h>
#include <pc.h>
#include <dos.h>
#ifndef NO_EVENTS		/* Define this if you don't have cbgrx*.zip */
#include <eventque.h>

#define Qsize 100		/* number of interrupt events to queue */
EventQueue *q = 0;
#else
#define KB_NUMLOCK	0x20		/* NUM LOCK active */
#define KB_CAPSLOCK	0x40		/* CAPS LOCK active */
#define KB_SHIFT	0x03		/* Either shift key depressed */
#endif

static short *screen_base = NULL;
static short saveshift = 0;	/* shift flags from previous keystroke */
static char setnumlk = 1;	/* reset NumLock */
static int direct_access = 0;	/* Can we directly touch hardware? */

/******************************************************************************\
|Routine: ttygetsb
|Callby: put
|Purpose: Returns video buffer base.
|Arguments:
|    ptr - physical video frame buffer base pointer.
\******************************************************************************/
void ttygetsb(ptr)
short **ptr;
{
	*ptr = screen_base;
}

/******************************************************************************\
|Routine: ttyscreen
|Callby: put
|Purpose: Updates screen buffer and positions cursor (DOS & OS2 only).
|Arguments:
|    row, col - where to position the cursor on the screen, zero based.
\******************************************************************************/
void ttyscreen(row,col)
Int row,col;
{
	if(!direct_access)		/* then protected mode, copy LVB */
		ScreenUpdate(screen_base);
	ScreenSetCursor(row,col);
}

/******************************************************************************\
|Routine: ttysetsize
|Callby: init_term
|Purpose: Handles defaults for terminal size.
|Arguments:
|    NROW,NCOL - the number of rows and columns on the screen.
\******************************************************************************/
void ttysetsize(nrow,ncol)
Int *nrow,*ncol;
{
	Uchar pcrow,pccol;
	union REGS regs;
	char *numlkt;

	pcrow = ScreenRows();
	pccol = ScreenCols();

	if(*ncol > pccol)		/* User wants more columns, set to 132 (max) */
	{
		regs.x.ax = 0x23;		/* Try Pro II 132x25 mode */
		int86(0x10, &regs, &regs);
	}

	if(*nrow > pcrow)		/* User wants more rows, use 8x8 font */
	{
		regs.x.ax = 0x1202;
		regs.x.bx = 0x30;
		int86(0x10, &regs, &regs);	/* Set 400 scan lines */

		regs.x.ax = 0x1112;
		regs.x.bx = 0;
		int86(0x10, &regs, &regs);	/* Set font and reprogram */
	}

	/* Indicate final resolution (may or may not change from above) */
	*nrow = ScreenRows();
	*ncol = ScreenCols();

	/* Check for DPMI (ESP will be small) */
	asm volatile(						       "\n\
	    movl   %%esp,%%eax						\n\
	    andl   $0x40000000,%%eax					\n\
	    movl   %%eax,_direct_access					"
	    : : : "ax"
	);

	if(direct_access)		/* direct access OK */
		screen_base = ScreenPrimary;	/* From pc.h (short *)0xe00b8000L; */
	else						/* Protected mode need buffer */
		screen_base = (short *)imalloc( *nrow * *ncol * sizeof(short) );

	if((numlkt = getenv("ED_NUMLK")) && *numlkt == 'N') setnumlk = 0;
}

/******************************************************************************\
|Routine: ring_bell
|Callby: init_term
|Purpose: Handles special bell-ringing for non-terminal PC terminals.
|Arguments:
|    none
\******************************************************************************/
void ring_bell()
{
	sound(320);			/* Frequency 320 Hz */
	delay(100);			/* 100 ms duration */
	nosound();
}

/******************************************************************************\
|Routine: ttyput
|Callby: put
|Purpose: Null routine.
|Arguments:
|    c is the string to send to the terminal.
\******************************************************************************/
void ttyput(c)
Char *c;
{
}

/******************************************************************************\
|Routine: ttychk
|Callby: load_file
|Purpose: Null routine.
|Arguments:
|    none
\******************************************************************************/
Int ttychk()
{
	return(0);
}

/******************************************************************************\
|Routine: ttyget
|Callby: cfg init_term
|Purpose: Gets a character from the terminal.
|Arguments:
|    c is the returned character.
\******************************************************************************/
void ttyget(c)
Schar *c;
{
	static Char buf[2],*next = buf;
	static Int remain = 0;

	short shift, bshift, keyqptr[2];
	unsigned evtcode,scan,asci;
	Uchar thischar;

	while(!remain)
	{
		next = buf;
#ifndef NO_EVENTS
		if(q) {
			EventRecord e;
			while(!EventQueueNextEvent(q,&e) || e.evt_type != EVENT_KEYBD);	/* Another event */
			shift = e.evt_kbstat;
			if(KB_NUMLOCK & (shift ^ saveshift))		/* NumLock */
			{
				*next++ = 0xaf;
				remain++;
			}
			dosmemget(0x417,2,&bshift);
			if(setnumlk && !q->evq_cursize && !(KB_NUMLOCK & bshift)) {
				saveshift = KB_NUMLOCK | bshift;
				dosmemput(&saveshift,2,0x417);
			} else
				saveshift = shift;
			evtcode = (e.evt_scancode & 0xffff);
		}
		else
#endif
		{
			union REGS regs;
			
			regs.x.ax = 0x1000;
			int86(0x16, &regs, &regs);	/* Read a character, wait */
			evtcode = (regs.x.ax & 0xffff);
			dosmemget(0x417,2,&shift);	/* Assumes curr shift state for press */
			if(KB_NUMLOCK & (shift ^ saveshift))		/* NumLock */
			{
				*next++ = 0xaf;
				remain++;
			}
			dosmemget(0x41a,4,keyqptr);
			if(setnumlk && (keyqptr[0] == keyqptr[1])&& !(KB_NUMLOCK & shift)) {
				saveshift = KB_NUMLOCK | shift;
				dosmemput(&saveshift,2,0x417);
			} else
				saveshift = shift;
		}
		scan = evtcode >> 8;
		asci = evtcode & 0xff;
		if(evtcode == 0x0e08)		/* Rubout */
			thischar = 127;
		else if(asci > 0 && scan <= 0x35)	/* If Caps Lock on then it is Caps! */
		{
			if((KB_CAPSLOCK & shift) && asci >= 'a' && asci <= 'z')
				asci -= 0x20;
			thischar = asci;
		}
		else if(scan == 0x39)		/* Space */
			thischar = ' ';
		else if(scan >= 0x47 && scan <= 0x53)	/* 13 characters between 0x00 and 0x0c (Keypad & Arrows) */
			thischar = ((asci == 0xe0)? 0xa0 : 0xb0) + (scan - 0x47);
		else if(evtcode == 0xe00d)	/* KpEnter */
			thischar = 0xbd;
		else if(evtcode == 0xe02f)	/* PF2 */
			thischar = 0xbe;
		else if(evtcode == 0x372a)	/* PF3 */
			thischar = 0xbf;
		else if(evtcode == 0x0300)	/* Null */
			thischar = 0;
/* 10 characters between 0x00 and 0x09 (F1 to F10) */
		else if(scan >= 0x3b && scan <= 0x44)
			thischar = 0xc0 + (scan - 0x3b);
		else if(evtcode == 0x8500)	/* F11 */
			thischar = 0xca;
		else if(evtcode == 0x8600)	/* F12 */
			thischar = 0xcb;
		else
			thischar = 0xff;		/* indicate not set */

		if(thischar >= 0xb0 && thischar <= 0xbf) /* shift special */
			if(KB_SHIFT & shift) thischar -= 0x10;

		if(thischar != 0xff)
		{
			*next++ = thischar;
			remain++;
		}
		next = buf;
	}
	*c = *((Schar *)next++);
	remain--;
	return;
}

/******************************************************************************\
|Routine: ttyend
|Callby: init_term ttyput
|Purpose: Terminates connection with the terminal.
|Arguments:
|    none
\******************************************************************************/
void ttyend()	/* this is a handler that restores the terminal settings at image exit */
{
	if(restored++)
		return;
	EventQueueDeInit();
	return;
}

void ttystart()
{
#ifndef NO_EVENTS
	q = EventQueueInit(Qsize,320,NULL);
	if(q) q->evq_drawmouse = 0;
#endif
	dosmemget(0x417,2,&saveshift);
	restored = 0;
}

#else		/* !GNUDOS */

#include "unistd.h"
#ifdef NeXT
#include <sgtty.h>
static struct sgttyb savetty;
#else
#include <termios.h>
#ifndef NO_TERMSIZE					/* Posix doesn't have terminal size */
#ifndef sparc
#include <sys/ioctl.h>				/* Only for terminal size IOCTL */
#endif
#endif
static struct termios savetty;
#endif
#include <fcntl.h>
#include <signal.h>
#ifndef O_NDELAY					/* Posix */
#define O_NDELAY O_NONBLOCK
#endif

static Int ttyfd;

/******************************************************************************\
|Routine: ttygetsb
|Callby: put
|Purpose: Null routine.
|Arguments:
|    ptr - physical video frame buffer base pointer.
\******************************************************************************/
void ttygetsb(ptr)
short **ptr;
{
	printf("You can't use this terminal configuration file with this image.\r\n");
	cleanup(-1);
}

/******************************************************************************\
|Routine: ttyscreen
|Callby: put
|Purpose: Null routine.
|Arguments:
|    row, col - where to position the cursor on the screen, zero based.
\******************************************************************************/
void ttyscreen(row,col)
Int row,col;
{
}

/******************************************************************************\
|Routine: ttysetsize
|Callby: init_term
|Purpose: Handles defaults for terminal size.
|Arguments:
|    nrow, ncol - the number of rows and columns on the screen.
\******************************************************************************/
void ttysetsize(nrow,ncol)
Int *nrow,*ncol;
{
	Int tmprow, tmpcol;
#ifndef NO_TERMSIZE
	struct winsize ttysiz;
#endif

	if(!*nrow || !*ncol)
	{
		tmprow = 24;
		tmpcol = 80;
#ifndef NO_TERMSIZE
		ttysiz.ws_row = 0;
		ttysiz.ws_col = 0;
		ioctl(ttyfd,TIOCGWINSZ,&ttysiz);
		if(ttysiz.ws_row > 0)
			tmprow = ttysiz.ws_row;
		if(ttysiz.ws_col > 0)
			tmpcol = ttysiz.ws_col;
#endif
		if(!*nrow)
			*nrow = tmprow;
		if(!*ncol)
			*ncol = tmpcol;
	}
}

/******************************************************************************\
|Routine: ring_bell
|Callby: init_term
|Purpose: Handles special bell-ringing for brain-dead terminals.
|Arguments:
|    none
\******************************************************************************/
void ring_bell()
{
}

/******************************************************************************\
|Routine: ttyput
|Callby: put
|Purpose: Puts characters to the terminal.
|Arguments:
|    c is the string to send to the terminal.
\******************************************************************************/
void ttyput(c)
Char *c;
{
	Int i;

	if(!(i = strlen(c)))
		return;
	while(i > 64)
	{
		write(ttyfd,c,64);
		c += 64;
		i -= 64;
	}
	if(i)
		write(ttyfd,c,i);
}

/******************************************************************************\
|Routine: ttyget
|Callby: cfg init_term
|Purpose: Gets a character from the terminal.
|Arguments:
|    c is the returned character.
\******************************************************************************/
void ttyget(c)
Schar *c;
{
	static Char buf[8],*next = buf;
	static Int remain = 0;

	if(!remain)
	{
		remain = read(ttyfd,buf,8);
		next = buf;
	}
	*c = *((Schar *)next++);
	remain--;
}

/******************************************************************************\
|Routine: ttyend
|Callby: init_term ttyput
|Purpose: Terminates connection with the terminal.
|Arguments:
|    none
\******************************************************************************/
void ttyend()	/* this is a handler that restores the terminal settings at image exit */
{
	if(restored++)
		return;
#ifdef NeXT
	ioctl(ttyfd,TIOCSETP,&savetty);
#else
	tcsetattr(ttyfd,TCSANOW,&savetty);
#endif
	close(ttyfd);
	signal(SIGINT,SIG_DFL);
	return;
}

/******************************************************************************\
|Routine: ttystart
|Callby: init_term ttyput
|Purpose: Initiates connection with the terminal.
|Arguments:
|    none
\******************************************************************************/
void ttystart()
{
#ifdef NeXT
	struct sgttyb newtty;
#else
	struct termios newtty;
	Char termname[L_ctermid];
#endif
	Int i;

#ifdef NeXT
	if((ttyfd = open("/dev/tty",O_RDWR)) < 0)
	{
		printf("Unable to open terminal /dev/tty.\n");
#else
	if((ttyfd = open(ctermid(termname),O_RDWR)) < 0)
	{
		printf("Unable to open terminal %s.\n",termname);
#endif
		exit(1);
	}
#ifdef NeXT
	if(ioctl(ttyfd,TIOCGETP,&savetty) == -1)
#else
	if(tcgetattr(ttyfd,&savetty) == -1)
#endif
	{
		printf("Unable to read terminal characteristics.\n");
		exit(1);
	}
	newtty = savetty;
#ifdef NeXT
	newtty.sg_flags |= RAW | CBREAK;
	newtty.sg_flags &= ~(CRMOD | ECHO);
	if(ioctl(ttyfd,TIOCSETP,&newtty) == -1)
#else
	newtty.c_lflag &= ~(ICANON | ISIG | ECHO);
	newtty.c_oflag &= ~OPOST;
	newtty.c_iflag &= ~(INLCR | ICRNL | ISTRIP | BRKINT); /* IUCLC */
	newtty.c_cflag |= HUPCL;
/*	newtty.c_cflag &= ~CLOCAL;*/
	for(i = 0;i < 16;i++)
		newtty.c_cc[i] = 0;
	newtty.c_cc[VMIN] = 5;	/* terminate io if 5 chars arrive... */
	newtty.c_cc[VTIME] = 1;	/* ...or 10 ms elapse */
	if(tcsetattr(ttyfd,TCSANOW,&newtty) == -1)
#endif
	{
		printf("Unable to set terminal to raw mode.\r\n");
		perror("ttyput");
		exit(1);
	}
	signal(SIGINT,ttyend);
	restored = 0;
}

/******************************************************************************\
|Routine: ttychk
|Callby: load_file
|Purpose: Checks for a character waiting, and returns it.
|Arguments:
|    none
\******************************************************************************/
Int ttychk()
{
	Int flags,retval;
	Char buf;
	
	retval = -1;
	if((flags = fcntl(ttyfd,F_GETFL,0)) != -1)
		if(fcntl(ttyfd,F_SETFL,flags | O_NDELAY) != -1)
		{
			if(read(ttyfd,&buf,1) == 1)
				retval = buf;
			fcntl(ttyfd,F_SETFL,flags);
		}
	return(retval);
}

#endif		/* GNUDOS */

#endif		/* GNUOS2 */

#endif		/* WIN32 */

#endif		/* VMS */

/******************************************************************************\
|Routine: cleanup
|Callby: cfg imalloc init_term journal main parse_fnm restore_par slip_message
|Purpose: Does all cleanup on image exit.
|Arguments:
|    STATUS - the value to pass to exit().
\******************************************************************************/
void cleanup(status)
Int status;
{
	ttyend();
#ifdef VMS
	exit((status < 0)? 0 : 1);
#else
	exit(status);
#endif
}

