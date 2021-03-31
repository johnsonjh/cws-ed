/* Included by: COMMAND */
/* NOTE: changes to this file must have corresponding changes in cmd_enum.h */
Char *commands[] =
{
"exit",
"quit",
"help",
"set",
"show",
"stor",
"rest",
"incl",
"writ",
"calc",
"date",
"time",
"abor",
"grep",
"byte",
"file",
"trim",
"cd",
"pwd",
"bigg",
"smal",
"load",
"unlo",
"sort",
"book",
"dele",
"perg",
"subs"
};

Char *params[] =
{
"shel",
"tabs",
"wrap",
"sect",
"word",
"page",
"para",
"sear",
"stab",
"clos",
"pare",
"defa",
"auto",
"cfri",
"box ",
"awrp",
"case",
"over",
"grep",
"wild",
"posi",	/* from this point onward, reserved for nonsettable params (see NUM_NONSETTABLE above) */
"modi",
"vers",
"keys"
};

Char *prompts[] =
{
"Shell",
"Tab setup",
"Wrap margin",
"Section lines",
"Word delimiters",
"Page delimiter",
"Paragraph delimiter",
"Search mode",
"Search table",
"Paren matching",
"Parenthesis pairs",
"Default extensions",
"Auto tabs",
"C-friendly mode",
"Box cut mode",
"Auto wrap",
"Case change",
"Overstrike",
"GREP mode",
"Search wildcard",
"Current position",
"Buffer modified",
"ED version number"
};

Char *smodes[] =
{
"gene",
"exac",
"begi",
"end",
"nowi",
"wild",
"nota",
"tabl",
"rege",
"nore",
"char",
"word"
};

Char *sprompts[] =
{
"General",
"Exact",
"Beginning",
"End",
"No wild",
"Wildcard",
"No table",
"Table",
"Regex",
"No Regex",
"Character",
"Word"
};

Char *offon[] =
{
"off",
"on",
"toggle"
};

Char *cases[] = 
{
"uppe",
"lowe",
"oppo",
"capi"
};

Char *caseprompts[] =
{
"Upper",
"Lower",
"Opposite",
"Capitalize"
};

Char *grepmodes[] =
{
"verbose",
"silent",
"toggle"
};
/* end */
