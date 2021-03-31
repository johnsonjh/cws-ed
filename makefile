# makefile for ED.
#
# To build ed, say:
#
#	make
#
include objects
include compiler
a.out:	config.h main.o $(OBJECTS)
	$(CC) main.o $(OBJECTS) -lm -o $@
	strip $@
%.o:	%.c
	$(CC) $(CFLAGS) -c -o $@ $<
config.h:
	@echo You must run configure or autoconfigure before make !
	@exit 2
