$ set noon
$ say:=write sys$output
$ tcpip_type=99
$ if(f$sear("sys$library:ucx$ipc.olb").nes."")then tcpip_type=1
$ if(f$sear("multinet:multinet_socket_library.*").nes."")then tcpip_type=2
$ if(f$sear("sys$library:twg_rtl.exe").nes."")then tcpip_type=4
$ if(f$sear("twg$tcp:[netdist.lib]twglib.olb").nes."")then tcpip_type=3
$ if(tcpip_type.eq.1)then say "This system appears to have UCX TCP/IP."
$ if(tcpip_type.eq.2)then say "This system appears to have MULTINET TCP/IP."
$ if(tcpip_type.eq.3)then say "This system appears to have WOLLONGONG TCP/IP."
$ if(tcpip_type.eq.4)then say "This system appears to have WOLLONGONG TCP/IP."
$ if(tcpip_type.eq.99)then say "This system doesn't appear to have TCP/IP."
$ vaxsys = 1
$ if(f$sear("sys$library:vaxcrtl.exe").eqs."")then vaxsys = 0 ! alpha?
$ if(vaxsys.eq.0)then say "This appears to be an Alpha or is missing a required file."
$ if(vaxsys.eq.0)then ed_cc_opts == "/stan=vaxc"
$ say " "
$ say "Determining whether your compiler understands the 'signed' keyword..."
$ open/write outfile test.c
$ write outfile "main(){signed char a;sub(a);}"
$ close outfile
$ defi/user sys$output nl:
$ defi/user sys$error test.out
$ cc/nolist/noobje 'ed_cc_opts test
$ signed=1
$ if(f$sear("test.out").nes."")
$  then
$   if(f$file("test.out","alq").gt.0)then signed=0
$   dele test.out;
$  endif
$ dele test.c;
$ say " "
$ say "Determining whether your C library includes the 'mmap' and 'munmap' routines..."
$ open/write outfile test.c
$ write outfile "main(){mmap();}"
$ close outfile
$ cc/nolist 'ed_cc_opts test
$ defi/user sys$output nl:
$ defi/user sys$error test.out
$ if(vaxsys.eq.1)
$  then
$   link/noexec/nomap test,sys$input:/opt
    sys$share:vaxcrtl/share
$  else
$   link/noexec/nomap test
$  endif
$ mmap=1
$ if(f$file("test.out","alq").gt.0)then mmap=0
$ dele test.obj;,test.out;,test.c;
$ say " "
$ open/write outfile config.h
$ say "Creating 'config.h'..."
$ write outfile "/* This file was created by the 'configure.com' procedure.*/"
$ if(tcpip_type.eq.1)then write outfile "#define UCX"
$ if(tcpip_type.eq.2)then write outfile "#define MULTINET"
$ if(tcpip_type.eq.3)then write outfile "#define WOLLONGONG"
$ if(tcpip_type.eq.4)then write outfile "#define WOLLONGONG"
$ if(tcpip_type.eq.99)then write outfile "#define NO_FTP"
$ if(tcpip_type.eq.99)then write outfile "#define NO_NEWS"
$ if(.not.signed)then write outfile "#define NO_SIGNED_CHAR"
$ if(.not.mmap)then write outfile "#define NO_MMAP"
$ close outfile
$ type config.h
$ open/write outfile vmslink.com
$ say "Creating 'vmslink.com'..."
$ write outfile "$! This file was created by the 'configure.com' procedure."
$ if(tcpip_type.eq.1)
$ then
$	write outfile "$ link/nomap/notrac/exec=ed.exe ed.olb/libr/incl=main,sys$library:ucx$ipc/library,sys$input:/opt"
$	if(vaxsys.eq.1)then write outfile "sys$share:vaxcrtl/share"
$ endif
$ if(tcpip_type.eq.2)
$ then
$	write outfile "$ link/nomap/notrac/exec=ed.exe ed.olb/libr/incl=main,sys$input:/opt"
$	write outfile "multinet:multinet_socket_library/share"
$	if(vaxsys.eq.1)then write outfile "sys$share:vaxcrtl/share"
$ endif
$ if(tcpip_type.eq.3)
$ then
$	write outfile "$ link/nomap/notrac/exec=ed.exe ed.olb/libr/incl=main,twg$tcp:[netdist.lib]twglib/library,sys$input:/opt"
$	if(vaxsys.eq.1)then write outfile "sys$share:vaxcrtl/share"
$ endif
$ if(tcpip_type.eq.4)
$ then
$	write outfile "$ link/nomap/notrac/exec=ed.exe ed.olb/libr/incl=main,sys$input:/opt"
$	write outfile "sys$share:twg_rtl/share"
$	if(vaxsys.eq.1)then write outfile "sys$share:vaxcrtl/share"
$ endif
$ if(tcpip_type.eq.99)
$ then
$	write outfile "$ link/nomap/notrac/exec=ed.exe ed.olb/libr/incl=main,sys$input:/opt"
$	if(vaxsys.eq.1)then write outfile "sys$share:vaxcrtl/share"
$ endif
$ write outfile "$ exit"
$ close outfile
