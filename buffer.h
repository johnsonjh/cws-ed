/* Included by: BUFFER_APPEND BUFFER_EMPTY CARRIAGE_RETURN CHANGE_CASE COMMAND EDIT FIND_STRING INCLUDE_FILE INQUIRE 
                INSERT KILLER LOAD_BUFFER MATCH_SEARCH OPENLINE OUTPUT_FILE WINCOM WORD_FILL 
Requires: rec.h */
typedef struct buf_str *buf_ptr;
typedef struct buf_str {int nrecs,direction;rec_ptr first,last;} buf_node;
