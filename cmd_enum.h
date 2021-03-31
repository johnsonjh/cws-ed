/* Included by: COMMAND RESTORE_PARAM SET_PARAM SHOW_PARAM STORE_PARAM */
/* NOTE: changes to this file must have corresponding changes in cmd_def.h */
enum commands
{
COM_EXIT,
COM_QUIT,
COM_HELP,
COM_SET,
COM_SHOW,
COM_STORE,
COM_RESTORE,
COM_INCLUDE,
COM_WRITE,
COM_CALCULATE,
COM_DATE,
COM_TIME,
COM_ABORT,
COM_GREP,
COM_BYTE,
COM_FILE,
COM_TRIM,
COM_CD,
COM_PWD,
COM_BIGGER,
COM_SMALLER,
COM_LOAD,
COM_UNLOAD,
COM_SORT,
COM_BOOKMARK,
COM_DELETE,
COM_PERG,
COM_SUBSTITUTE	/* add new commands following this one */
};
#define NUM_COMMANDS 28

enum params
{
PAR_SHELL,
PAR_TABS,
PAR_WRAP,
PAR_SECTION,
PAR_WORD,
PAR_PAGE,
PAR_PARAGRAPH,
PAR_SEARCH,
PAR_STABLE,
PAR_CLOSE_PARENS,
PAR_PARENS,
PAR_DEFAULT,
PAR_AUTO_TABS,
PAR_CFRIENDLY,
PAR_BOXCUT,
PAR_AUTOWRAP,
PAR_CASE,
PAR_OVERSTRIKE,
PAR_GREPMODE,
PAR_WILDCARD,	/* add new SETable params following this one */
PAR_POSITION,
PAR_MODIFIED,
PAR_VERSION,	/* add new non-SETable params following this one */
PAR_KEYS	/* leave KEYS as the last one. */
};
#define NUM_PARAMS 24
#define NUM_NOTSETTABLE 4	/* this means that the last 4 params cannot be SET */

enum smodes
{
SMODE_GENERAL,
SMODE_EXACT,
SMODE_BEGINNING,
SMODE_END,
SMODE_NO_WILDCARDS,
SMODE_WILDCARDS,
SMODE_NOT_TABLE_DRIVEN,
SMODE_TABLE_DRIVEN,
SMODE_REGEXP,
SMODE_NOREGEXP,
SMODE_CHAR,
SMODE_WORD	/* add new search modes following this one */
};
#define NUM_SMODES 12

enum cases
{
CASE_UPPER,
CASE_LOWER,
CASE_OPPOSITE,
CASE_CAPITALIZE
};
#define NUM_CASES 4
