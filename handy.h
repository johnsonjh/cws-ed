/* Included by: CALC_LIMITS CHANGE_CASE EDIT INIT_TERIMINAL INSERT_WINDOW */
#define max(x,y) (((x)>=(y))?(x):(y))	/* maximum of two things */
#define min(x,y) (((x)<=(y))?(x):(y))	/* minimum of two things */
#ifndef _AIX
#define abs(x)   (((x)>=0)?(x):(-(x)))	/* absolute value (assumes correct promotion of 0 by compiler) */
#endif
#define roundup(n,b) ((((n)+(b)-1)/(b))*(b))	/* round n up to the next multiple of b */
#define truncate(n,b) (((n)/(b))*(b))	/* round n down to a multiple of b */
