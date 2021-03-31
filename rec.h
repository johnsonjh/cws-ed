/* Included by: many, many routines */
typedef struct rec_str *rec_ptr;
typedef struct rec_str {rec_ptr next,prev;int length;Char *data;int recflags;} rec_node;
