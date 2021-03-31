@rem This file require DJGPP and event library from LIBGRX
@rem
@echo #define GNUDOS > config.h
@echo #define NO_MMAP >> config.h
@echo #define NO_FTP >> config.h
@echo #define NO_NEWS >> config.h
@echo #define USE_NCS >> config.h
@REM echo #define NO_EVENTS
@rem
@echo CC=gcc > compiler
@echo CFLAGS=-O2 >> compiler
@rem
@rem set gcccmd=gcc -c -O2 -m486 (for 486 machine, larger but faster)
set gcccmd=gcc -c -O2
@rem This is broken up due to gccrm limitations
%gcccmd% autowrp.c backspac.c bigger_w.c buffer_a.c buffer_e.c calculat.c calc_lim.c carriage.c scroll_d.c 
%gcccmd% cfg.c cfrendly.c change_c.c command.c copier.c copy_rec.c cqsort.c down_arr.c do_grep.c edit.c ftp.c
%gcccmd% envir_su.c express.c extensio.c file_lis.c find_cha.c find_col.c load_fil.c str_to_b.c dir.c
%gcccmd% find_fil.c find_str.c fixup_re.c fix_botr.c fix_disp.c fix_scro.c get_colu.c get_colb.c  include_.c 
%gcccmd% get_mark.c get_offs.c get_posi.c get_seld.c get_toke.c help.c help_get.c help_loa.c imalloc.c
%gcccmd% init_ter.c inquire.c insert.c insert_w.c insq.c journal.c killer.c left_arr.c libc.c load_buf.c 
%gcccmd% load_key.c main.c match_pa.c match_se.c move_eol.c move_lin.c move_wor.c new_botr.c new_wind.c 
%gcccmd% output_f.c openline.c paint.c paint_wi.c parse_co.c parse_fn.c put.c rec_chgc.c rec_copy.c rec_inse.c
%gcccmd% rec_merg.c rec_spli.c rec_trim.c ref_disp.c ref_wind.c remove_w.c restore_.c right_ar.c save_win.c 
%gcccmd% scroll_u.c select.c set_para.c set_wind.c show_par.c slip_mes.c smaller_.c sort_rec.c store_pa.c 
%gcccmd% tabstop.c toss_dat.c trim.c ttyput.c unselect.c up_arrow.c version.c wincom.c word_fil.c 
%gcccmd% regex.c
gcc *.o -lpc -lqueue -lm -lc -o edc.e
copy edc.e edcdebug.e
strip edc.e
@rem coff2exe -s \dj110\go32\go32ed.exe edc.e
aout2exe edc.e
