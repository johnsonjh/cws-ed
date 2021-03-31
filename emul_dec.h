/* Included by: INIT_TERMINAL */
extern Char *EMU_SCREEN,*EMU_ATTRIB;	/* screen image and attribute mask */
extern int EMU_NROW,EMU_NCOL;	/* size of screen and attrib */
extern int EMU_CURROW,EMU_CURCOL;	/* cursor position (zero-based) */
extern int EMU_CURATT;	/* current attribute setting, <0> = reverse, <1> = bold, <2> = underline, <3> = flashing */
extern Char BRAINDEAD;	/* terminal has no capabilities, use special mode */
