/*  Copyright 1992, 1994 John Bovey, University of Kent at Canterbury.
 *
 *  Redistribution and use in source code and/or executable forms, with
 *  or without modification, are permitted provided that the following
 *  condition is met:
 *
 *  Any redistribution must retain the above copyright notice, this
 *  condition and the following disclaimer, either as part of the
 *  program source code included in the redistribution or in human-
 *  readable materials provided with the redistribution.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS".  Any express or implied
 *  warranties concerning this software are disclaimed by the copyright
 *  holder to the fullest extent permitted by applicable law.  In no
 *  event shall the copyright-holder be liable for any damages of any
 *  kind, however caused and on any theory of liability, arising in any
 *  way out of the use of, or inability to use, this software.
 *
 *  -------------------------------------------------------------------
 *
 *  In other words, do not misrepresent my work as your own work, and
 *  do not sue me if it causes problems.  Feel free to do anything else
 *  you wish with it.
 */

char xvt_xvt_c_sccsid[] = "@(#)xvt.c	1.4 11/1/94 (UKC)";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "rvt.h"
#include "command.h"
#include "ttyinit.h"
#include "xsetup.h"
#include "screen.h"
#include "sbar.h"
#include "token.h"
#include "../common/common.h"

#ifdef UKC_LOCATIONS
#define LOCTMPFILE "/etc/loctmp"
#endif /* UKC_LOCATIONS */

#ifdef __STDC__
int main(int, char **);
#else
int main();
#endif

extern int debug;

static int size_set = 0;	/* flag set once the window size has been set */
char *progname;

/*  Malloc that checks for NULL return.
 */
void *
cmalloc(size)
int size;
{
	char *s;

	if ((s = malloc((unsigned int)size)) == NULL)
		abort();
	return((void *)s);
}

/*  Run the command in a subprocess and return a file descriptor for the
 *  master end of the pseudo-teletype pair with the command talking to
 *  the slave.
 */
int
main(argc,argv)
int argc;
char **argv;
{
	int i, n, x, y;
	int mode;
	int iargc;
	int nrofargs;
	struct tokenst token;
	char *command = NULL;
	char *remote_username = NULL;
	char *remote_hostname = NULL;
	char *remote_port = NULL;
	char **com_argv = NULL;
	char **rsh;
	char buf[MAXBUF], pbuf[MAXBUF];
	static char **iargv;
	extern char *version;

	/* Check for a -V or -help option.
	 */
	progname = strdup(basename(argv[0]));

	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i],"-V") == 0 ||
		    strcmp(argv[i],"-v") == 0) {
		    (void)printf("%s: %s\n", progname, version);
		    (void)printf("based on xvt version %s\n", XVT_VERSION);
		    exit(0);
		}
		if (strcmp(argv[i],"-help") == 0) {
			usage(1);
			exit(1);
		}
	}

	/* Make a copy of the command line argument array
	 */
	iargv = (char **)cmalloc((argc + 1) * sizeof(char *));
	for (i = 0; i < argc; i++) {
		iargv[i] = (char *)cmalloc(strlen(argv[i]) + 1);
		strcpy(iargv[i],argv[i]);
	}
	iargv[i] = NULL;
	iargc = argc;
	com_argv = NULL;

	/*  Look for a -e flag and if it is there use it to initialise
	 *  the command and its arguments.
	 */
	/*
	for (i = 0; i < argc; i++)
		if (strcmp(argv[i],"-e") == 0)
			break;
	if (i < argc - 1) {
		argv[i] = NULL;
		com_argv = argv + i + 1;
		command = argv[i + 1];
		argc = i;
	}
	*/
	
	/* Find and remove any -l flag for the username */
	for( i = 0; i < argc; i++ )
		if (strcmp(argv[i], "-l") == 0)
			break;
	if( i < argc - 1)
	{
		remote_username = strdup(argv[i+1]);
		for(i+=2; i < argc; i++)
			argv[i-2] = argv[i];
		argv[argc-1] = NULL;
		argv[argc-2] = NULL;
		argc -= 2;
	}

	/* Pull out the last argument as the hostname */
	if(argc < 2)
	{
		usage(1);
		exit(1);
	}
	remote_hostname = argv[--argc];
	argv[argc] = NULL;
		
	init_display(argc,argv,iargc,iargv,command);
	map_window();

	rsh = parse_rcmd("RLOGIN_CMD", "RLOGIN_CMD_ARGS", &nrofargs);
	
	/* Build the rsh command line */
	remote_port = (char *)strtok(remote_hostname, ":");
	if( remote_port != NULL) {
		remote_port = (char *)strtok( NULL, ":");
	}
	if( remote_port != NULL)
		(void)snprintf(pbuf, MAXBUF, "-p%s", remote_port);
	else
		if( getenv("RCMD_PORT") )
			(void)snprintf(pbuf, MAXBUF, "-p%s", getenv("RCMD_PORT"));
		else
			(void)snprintf(pbuf, MAXBUF, "-p22");

	if (remote_username != NULL)
		(void)snprintf(buf, MAXBUF, "%s@%s", remote_username,
		    remote_hostname);
	else
		(void)snprintf(buf, MAXBUF, "%s", remote_hostname);
	com_argv = (char **)calloc(nrofargs + 2, sizeof(char *));
	i = 0;
	while (rsh[i] != NULL) {
		com_argv[i] = rsh[i];
		i++;
	}
	if( pbuf ) com_argv[i++] = pbuf;
	com_argv[i++] = buf;
	com_argv[i] = (char *)0;
	
	fix_environment();

	init_command(com_argv[0], com_argv);

	for (;;) {
		get_token(&token);
		switch (token.tk_type) {
		    case TK_STRING :
			scr_string(token.tk_string,token.tk_length,token.tk_nlcount);
			break;
		    case TK_CHAR :
			switch (token.tk_char) {
			    case '\n' :
				scr_index();
				break;
			    case '\r' :
				scr_move(0,0,ROW_RELATIVE);
				break;
			    case '\b' :
				scr_backspace();
				break;
			    case '\t' :
				scr_tab();
				break;
			    case '\007' :	/* bell */
				scr_bell();
				break;
			    case '\016' :	/* change to char set G1 */
				scr_shift(1);
				break;
			    case '\017' :	/* change to char set G0 */
				scr_shift(0);
				break;
			}
			break;
		    case TK_EOF :
			quit(0);
			break;
		    case TK_ENTRY :	/* keyboard focus changed */
			scr_focus(1,token.tk_arg[0]);
			break;
		    case TK_FOCUS :
			scr_focus(2,token.tk_arg[0]);
			break;
		    case TK_EXPOSE :	/* window exposed */
			switch (token.tk_region) {
			    case SCREEN :
				if (!size_set) {

					/*  Force a full reset if an exposure event
					 *  arrives after a resize.
					 */
					scr_reset();
					size_set = 1;
				} else {
					scr_refresh(token.tk_arg[0],token.tk_arg[1],
						token.tk_arg[2],token.tk_arg[3]);
				}
				break;
			    case SCROLLBAR :
				sbar_reset();
				break;
			}
			break;
		    case TK_RESIZE :
			if (resize_window() != 0)
				size_set = 0;
			break;
		    case TK_TXTPAR :		/* change title or icon name */
			switch (token.tk_arg[0]) {
			    case 0 :
				change_window_name(token.tk_string);
				change_icon_name(token.tk_string);
				break;
			    case 1 :
				change_icon_name(token.tk_string);
				break;
			    case 2 :
				change_window_name(token.tk_string);
				break;
			}
			break;
		    case TK_SBSWITCH :
			switch_scrollbar();
			break;
		    case TK_SBGOTO :
			scr_move_to(token.tk_arg[0]);
			break;
		    case TK_SBUP :
			scr_move_by(token.tk_arg[0]);
			break;
		    case TK_SBDOWN :
			scr_move_by(-token.tk_arg[0]);
			break;
		    case TK_SELSTART :
			scr_start_selection(token.tk_arg[0],token.tk_arg[1],CHAR);
			break;
		    case TK_SELEXTND :
			scr_extend_selection(token.tk_arg[0],token.tk_arg[1],0);
			break;
		    case TK_SELDRAG :
			scr_extend_selection(token.tk_arg[0],token.tk_arg[1],1);
			break;
		    case TK_SELWORD :
			scr_start_selection(token.tk_arg[0],token.tk_arg[1],WORD);
			break;
		    case TK_SELLINE :
			scr_start_selection(token.tk_arg[0],token.tk_arg[1],LINE);
			break;
		    case TK_SELECT :
			scr_make_selection(token.tk_arg[0]);
			break;
		    case TK_SELCLEAR :
			scr_clear_selection();
			break;
		    case TK_SELREQUEST :
			scr_send_selection(token.tk_arg[0],token.tk_arg[1],
					   token.tk_arg[2],token.tk_arg[3]);
			break;
		    case TK_SELINSRT :
			scr_request_selection(token.tk_arg[0],token.tk_arg[1],token.tk_arg[2]);
			break;
		    case TK_SELNOTIFY :
			scr_paste_primary(token.tk_arg[0],token.tk_arg[1],token.tk_arg[2]);
			break;
		    case TK_CUU :	/* cursor up */
			n = token.tk_arg[0];
			n = n == 0 ? -1 : -n;
			scr_move(0,n,ROW_RELATIVE | COL_RELATIVE);
			break;
		    case TK_CUD :	/* cursor down */
			n = token.tk_arg[0];
			n = n == 0 ? 1 : n;
			scr_move(0,n,ROW_RELATIVE | COL_RELATIVE);
			break;
		    case TK_CUF :	/* cursor forward */
			n = token.tk_arg[0];
			n = n == 0 ? 1 : n;
			scr_move(n,0,ROW_RELATIVE | COL_RELATIVE);
			break;
		    case TK_CUB :	/* cursor back */
			n = token.tk_arg[0];
			n = n == 0 ? -1 : -n;
			scr_move(n,0,ROW_RELATIVE | COL_RELATIVE);
			break;
		    case TK_HVP :
		    case TK_CUP :	/* position cursor */
			if (token.tk_nargs == 1)
				if (token.tk_arg[0] == 0) {
					x = 0;
					y = 0;
				} else {
					x = 0;
					y = token.tk_arg[0] - 1;
				}
			else {
				y = token.tk_arg[0] - 1;
				x = token.tk_arg[1] - 1;
			}
			scr_move(x,y,0);
			break;
		    case TK_ED :
			scr_erase_screen(token.tk_arg[0]);
			break;
		    case TK_EL :
			scr_erase_line(token.tk_arg[0]);
			break;
		    case TK_IL :
			n = token.tk_arg[0];
			if (n == 0)
				n = 1;
			scr_insert_lines(n);
			break;
		    case TK_DL :
			n = token.tk_arg[0];
			if (n == 0)
				n = 1;
			scr_delete_lines(n);
			break;
		    case TK_DCH :
			n = token.tk_arg[0];
			if (n == 0)
				n = 1;
			scr_delete_characters(n);
			break;
		    case TK_ICH :
			n = token.tk_arg[0];
			if (n == 0)
				n = 1;
			scr_insert_characters(n);
			break;
		    case TK_DA :
			cprintf("\033[?6c");	/* I am a VT102 */
			break;
		    case TK_TBC :
			break;
		    case TK_SET :
		    case TK_RESET :
			mode = (token.tk_type == TK_SET) ? HIGH : LOW;
			if (token.tk_private == '?') {
				switch (token.tk_arg[0]) {
				    case 1 :
					set_cur_keys(mode);
					break;
				    case 6 :
					scr_set_decom(mode);
					break;
				    case 7 :
					scr_set_wrap(mode);
					break;
				    case 47 :		/* switch to main screen */
					scr_change_screen(mode);
					break;
				}
			} else if (token.tk_private == 0) {
				switch (token.tk_arg[0]) {
				    case 4 :
					scr_set_insert(mode);
					break;
				}
			}
			break;
		    case TK_SGR :
			if (token.tk_nargs == 0)
				scr_change_rendition(RS_NONE);
			else {
				for (i = 0; i < token.tk_nargs; i++) {
					switch (token.tk_arg[i]) {
					    case 0 :
						scr_change_rendition(RS_NONE);
						break;
					    case 1 :
						scr_change_rendition(RS_BOLD);
						break;
					    case 4 :
						scr_change_rendition(RS_ULINE);
						break;
					    case 5 :
						scr_change_rendition(RS_BLINK);
						break;
					    case 7 :
						scr_change_rendition(RS_RVID);
						break;
					}
				}
			}
			break;
		    case TK_DSR :		/* request for information */
			switch (token.tk_arg[0]) {
			    case 6 :
				scr_report_position();
				break;
			    case 7 :	/* display name */
				scr_report_display();
				break;
			    case 8 :	/* send magic cookie */
				send_auth();
				break;
			}
			break;
		    case TK_DECSTBM :		/* set top and bottom margins */
			if (token.tk_private == '?')
				/*  xterm uses this combination to reset parameters.
				 */
				break;
			if (token.tk_nargs < 2 || token.tk_arg[0] >= token.tk_arg[1])
				scr_set_margins(0,10000);
			else
				scr_set_margins(token.tk_arg[0] - 1,token.tk_arg[1] - 1);
			break;
		    case TK_DECSWH :		/* ESC # digit */
			if (token.tk_arg[0] == '8')
				scr_efill();	/* fill screen with Es */
			break;
		    case TK_SCS0 :
			switch (token.tk_arg[0]) {
			    case 'A' :
				scr_set_char_set(0,CS_UKASCII);
				break;
			    case 'B' :
				scr_set_char_set(0,CS_USASCII);
				break;
			    case '0' :
				scr_set_char_set(0,CS_SPECIAL);
				break;
			}
			break;
		    case TK_SCS1 :
			switch (token.tk_arg[0]) {
			    case 'A' :
				scr_set_char_set(1,CS_UKASCII);
				break;
			    case 'B' :
				scr_set_char_set(1,CS_USASCII);
				break;
			    case '0' :
				scr_set_char_set(1,CS_SPECIAL);
				break;
			}
			break;
		    case TK_DECSC :
			scr_save_cursor();
			break;
		    case TK_DECRC :
			scr_restore_cursor();
			break;
		    case TK_DECPAM :
			set_kp_keys(HIGH);
			break;
		    case TK_DECPNM :
			set_kp_keys(LOW);
			break;
		    case TK_IND :		/* Index (same as \n) */
			scr_index();
			break;
		    case TK_NEL :
			break;
		    case TK_HTS :
			break;
		    case TK_RI :		/* Reverse index */
			scr_rindex();
			break;
		    case TK_SS2 :
			break;
		    case TK_SS3 :
			break;
		    case TK_DECID :
			cprintf("\033[?6c");	/* I am a VT102 */
			break;
		}
#ifdef DEBUG
		if (debug)
			show_token(&token);
#endif /* DEBUG */
	}
	return 0;
}
