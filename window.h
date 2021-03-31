/* Included by: many, many routines */
typedef struct win_str *win_ptr;
typedef struct win_str
{
	rec_ptr base,currec,toprec,botrec;
	int curbyt,toprow,botrow,toplim,botlim,currow,curcol,firstcol,wantcol,modified,diredit,binary,news;
	Char *filebuf,*filename,*bookmark;
} win_node;
