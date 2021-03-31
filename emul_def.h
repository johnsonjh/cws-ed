/* Included by: PUT */
Char *EMU_SCREEN,*EMU_ATTRIB;	/* screen image and attribute mask */
int EMU_NROW,EMU_NCOL;	/* size of screen and attrib */
int EMU_CURROW,EMU_CURCOL;	/* cursor position (zero-based) */
int EMU_CURATT;	/* current attribute setting, <0> = reverse, <1> = bold, <2> = underline, <3> = flashing */
Char BRAINDEAD = 0;	/* terminal has no capabilities, use special mode */
