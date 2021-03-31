/* Included by: HELP HELP_GET_KEYWORD HELP_LOAD */
typedef struct subtopic_str *subtopic_ptr;
typedef struct topic_str *topic_ptr;
typedef struct topic_str
{
	subtopic_ptr first;
	topic_ptr parent;
	Char *topic,*text;
} topic_node;
typedef struct subtopic_str
{
	subtopic_ptr next;
	topic_ptr child;
	Char *keyword;
} subtopic_node;
