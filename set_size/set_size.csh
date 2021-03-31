if ( ! $?firsttime ) then
	/usr/local/bin/read_size 		# get the terminal's size
	setenv firsttime n
endif
alias ed '/usr/local/bin/ed /usr/local/bin/$TERM.ed ~/ed.setup 0 0'
