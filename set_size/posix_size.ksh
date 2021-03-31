if [[ ! -n ${firsttime} ]] ;then
	/usr/local/bin/read_size ksh;	# get the terminal's size
	if [[ -O read_size.tmp ]] ;then
		. read_size.tmp;
		rm read_size.tmp;
	else
		ROWS=24;
		COLS=80;
	fi
	stty rows ${ROWS} cols ${COLS};
	firsttime=nope;
fi
alias ed='/usr/local/bin/ed /usr/local/bin/${TERM}.ed ~/ed.setup 0 0'
