if ( ! $?firsttime ) then
	/usr/local/bin/set_size csh		# get the terminal's size
	setenv ROWS 24					# in case disk full
	setenv COLS 80
	if ( -e set_size.tmp ) then
		source set_size.tmp
		rm set_size.tmp
	endif
	stty rows $ROWS cols $COLS
	setenv firsttime n
endif
alias ed '/usr/local/bin/ed /usr/local/bin/$TERM.ed ~/ed.setup 0 0'
