/* Included in: many, many routines
Requires: rec.h window.h */
#define MAX_TAB_SETUPS 16
#define MAX_WINDOWS 8
#define MAX_MARKS 16
#define BINARY_RECSIZE 32

extern int NROW,NCOL;	/* size of screen */

extern rec_ptr BASE;	/* base of record queue */
extern rec_ptr CURREC;	/* current record */
extern int CURBYT;	/* position in that record */
extern rec_ptr TOPREC,BOTREC;	/* records at top and bottom of window */
extern rec_ptr NEWTOPREC;	/* new top record during kills */
extern rec_ptr SELREC;	/* record that contains the select marker */
extern int SELBYT;	/* byte within that record for select marker */
extern int SELDIR;	/* which direction to the select marker */

extern int TOPROW,BOTROW;	/* window region */
extern int TOPLIM,BOTLIM;	/* free movement region */
extern int CURROW,CURCOL;	/* position on screen */
extern int FIRSTCOL;	/* column at extreme left of screen */
extern int WANTCOL;	/* desired column number */
extern int DIRECTION;	/* movement direction */

extern int NMARK;	/* number of marked spots */
extern int MARKWINDOW[MAX_MARKS];
extern rec_ptr MARKREC[MAX_MARKS];
extern int MARKBYT[MAX_MARKS];

extern int TAB_SETUPS;	/* number of tab strings defined */
extern int CUR_TAB_SETUP;	/* index to current tab setup */
extern Char *TAB_NAME[MAX_TAB_SETUPS];	/* names of tab setups */
extern Char *TAB_STRING[MAX_TAB_SETUPS];	/* tab setup strings */
extern int TAB_LENGTH[MAX_TAB_SETUPS];	/* lengths of tab setups */

extern int WRAP_MARGIN;
extern int SECTION_LINES;
extern Char WORD_DELIMITERS[256];
extern int NWORD_DELIMITERS;
extern Char PAGE_BREAK[16];
extern int PAGE_BREAK_LENGTH;
extern Char PARAGRAPH_BREAK[16];
extern int PARAGRAPH_BREAK_LENGTH;
extern Char SEARCH_FLAGS[6];	/* General/Exact,Beginning/End,Nowild/Wild,Notable/Table,Regex/Noregex,Char/Word */
extern Char SEARCH_TABLE[256];
extern Char CLOSE_PARENS;
extern Char PAREN_STRING[16];
extern int PAREN_STRING_LENGTH;
extern Char DEFAULT_EXT[256];
extern int DEFAULT_EXT_LENGTH;
extern Char KEYFILE[256];
extern Char JOURNAL_FILE[256];
extern Char USERSHELL[256];
extern int TAB_AUTO;	/* flag to insert matching tabs at beginning of records */
extern int CFRIENDLY;	/* flag to make closing } chars seek matching tab level */
extern int CASECHANGE;	/* 0=upcase,1=lowcase,2=changecase,3=capitalize */

extern int NWINDOWS;
extern int CURWINDOW;
extern win_node WINDOW[MAX_WINDOWS];
extern Char HEXMODE;	/* editor is in hex mode */
extern Char REPORTSTATUS;	/* display status line for each buffer */
extern Char BOXCUT;	/* editor is in box cut mode */
extern Char AUTOWRAP;	/* do autowrapping */
extern Char OVERSTRIKE;	/* 0=insert mode, 1=overstrike mode */
extern Char GREPMODE;	/* 0=verbose, 1=silent */
extern Uchar WILDCARD;	/* wildcard for search */
extern rec_ptr NETRC;	/* record queue for .netrc image */
