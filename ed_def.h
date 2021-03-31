/* Included in: ED
Requires: rec.h window.h */
#define MAX_TAB_SETUPS 16
#define MAX_WINDOWS 8
#define MAX_MARKS 8
#define BINARY_RECSIZE 32

int NROW,NCOL;	/* size of screen */

rec_ptr BASE;	/* base of record queue */
rec_ptr CURREC;	/* current record */
int CURBYT;	/* position in that record */
rec_ptr TOPREC,BOTREC;	/* records at top and bottom of window */
rec_ptr NEWTOPREC;	/* new top record during kills */
rec_ptr SELREC;	/* record that contains the select marker */
int SELBYT;	/* byte within that record for select marker */
int SELDIR;	/* which direction to the select marker */

int TOPROW,BOTROW;	/* window region */
int TOPLIM,BOTLIM;	/* free movement region */
int CURROW,CURCOL;	/* position on screen */
int FIRSTCOL;	/* column at extreme left of screen */
int WANTCOL;	/* desired column number */
int DIRECTION;	/* movement direction */

int NMARK;	/* number of marked spots */
int MARKWINDOW[MAX_MARKS];
rec_ptr MARKREC[MAX_MARKS];
int MARKBYT[MAX_MARKS];

int TAB_SETUPS;	/* number of tab strings defined */
int CUR_TAB_SETUP;	/* index to current tab setup */
Char *TAB_NAME[MAX_TAB_SETUPS];	/* names of tab setups */
Char *TAB_STRING[MAX_TAB_SETUPS];	/* tab setup strings */
int TAB_LENGTH[MAX_TAB_SETUPS];	/* lengths of tab setups */

int WRAP_MARGIN;
int SECTION_LINES;
Char WORD_DELIMITERS[256];
int NWORD_DELIMITERS;
Char PAGE_BREAK[16];
int PAGE_BREAK_LENGTH;
Char PARAGRAPH_BREAK[16];
int PARAGRAPH_BREAK_LENGTH;
Char SEARCH_FLAGS[6];
Char SEARCH_TABLE[256];
Char CLOSE_PARENS;
Char PAREN_STRING[16];
int PAREN_STRING_LENGTH;
Char DEFAULT_EXT[256];
int DEFAULT_EXT_LENGTH;
Char KEYFILE[256];
Char JOURNAL_FILE[256];
Char USERSHELL[256];
int TAB_AUTO;	/* flag to insert matching tabs at beginning of records */
int CFRIENDLY;	/* flag to make closing } chars seek matching tab level */
int CASECHANGE;	/* 0=upcase,1=lowcase,2=changecase,3=capitalize */

int NWINDOWS;
int CURWINDOW;
win_node WINDOW[MAX_WINDOWS];
Char HEXMODE = 0;	/* editor is in hex mode */
Char REPORTSTATUS = 0;	/* display status line for each buffer */
Char BOXCUT;	/* editor is in box cut mode */
Char AUTOWRAP;	/* do autowrapping */
Char OVERSTRIKE;	/* 0=insert mode, 1=overstrike mode */
Char GREPMODE;	/* 0=verbose, 1=silent */
Uchar WILDCARD;	/* wildcard for search */
rec_ptr NETRC;	/* record queue for .netrc image */
