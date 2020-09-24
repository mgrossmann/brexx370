#include <stdio.h>
#include <string.h>
#include "lstring.h"

#include "rexx.h"
#include "rxtcp.h"
#include "jccdummy.h"
#include "util.h"

/* ------- Includes for any other external library ------- */
#if defined(__MVS__) || defined(__CROSS__)
extern int  __CDECL RxMvsInitialize();
extern void __CDECL RxMvsRegFunctions();
extern int  __CDECL isTSO();
#endif

#ifndef __CROSS__
extern int __libc_arch;
#else
int __libc_arch = 0;
#endif

void term();

/* --------------------- main ---------------------- */
int __CDECL
main(int ac, char *av[])
{
	Lstr	args[MAXARGS], tracestr, file;
	int	ia,ir,iaa,rc,staeret;
	bool	input, loop_over_stdin, parse_args, interactive;
    jmp_buf b;
    
    char systemCompletionCode[3 + 1];
    char userCompletionCode[3 + 1];

    SDWA sdwa;

	input           = FALSE;
	loop_over_stdin = FALSE;
	parse_args      = FALSE;
	interactive     = FALSE;

    atexit(term);
    if (strcasecmp(av[ac - 1], "NOSTAE") == 0)
	{
        staeret = 0;
        ac--;
    } else
    {
        staeret = _setjmp_stae(b, (char *) &sdwa); // We don't want 104 bytes of abend data
    }

    if (staeret == 0) { // Normal return
        rc = RxMvsInitialize();
        if (rc != 0) {
            printf("\nBRX0001E - ERROR IN INITIALIZATION OF THE BREXX/370 ENVIRONMENT: %d\n",rc);
            return rc;
        }

        for (ia=0; ia<MAXARGS; ia++) LINITSTR(args[ia]);
        LINITSTR(tracestr);
        LINITSTR(file);

        if (ac<2) {
            puts(VERSIONSTR);

            return 0;
        }
#ifdef __DEBUG__
        __debug__ = FALSE;
#endif

        RxInitialize(av[0]);

        /* --- Register functions of external libraries --- */
#if defined(__MVS__) || defined(__CROSS__)
        RxMvsRegFunctions();
#endif

        /* --- scan arguments --- */
        ia = 1;
        if (av[ia][0]=='-') {
            if (av[ia][1]==0)
                input = TRUE;
            else
            if (av[ia][1]=='F')
                loop_over_stdin = input = TRUE;
            else
            if (av[ia][1]=='a')
                parse_args = TRUE;
            else
            if (av[ia][1]=='i')
                interactive = TRUE;
#ifndef __CROSS__
                else
		if (av[ia][1]=='m')
			__libc_arch = atoi(av[ia]+2);
#endif
            else
                Lscpy(&tracestr,av[ia]+1);
            ia++;
        } else
        if (av[ia][0]=='?' || av[ia][0]=='!') {
            Lscpy(&tracestr,av[ia]);
            ia++;
        }

        /* --- let's read a normal file --- */
        if (!input && !interactive && ia<ac) {
            /* prepare arguments for program */
            iaa = 0;
            for (ir=ia+1; ir<ac; ir++) {
                if (parse_args) {
                    Lscpy(&args[iaa], av[ir]);
                    if (++iaa >= MAXARGS) break;
                } else {
                    Lcat(&args[0], av[ir]);
                    if (ir<ac-1) Lcat(&args[0]," ");
                }
            }

            if(isTSO())
                RxRun(av[ia],NULL,args,&tracestr,"TSO");
            else
                RxRun(av[ia],NULL,args,&tracestr,NULL);

        } else {
            if (interactive)
                Lcat(&file,
                     "signal on syntax;"
                     "signal on error;"
                     "signal on halt;"
                     "start:do forever;"
                     "call write ,\">>> \";"
                     " parse pull _;"
                     " result=@r;"
                     " interpret _;"
                     " @r=result;"
                     "end;"
                     "signal start;"
                     "syntax:;error: say \"+++ Error\" RC\":\" errortext(RC);"
                     "signal start;"
                     "halt:");
            else
            if (ia>=ac) {
                Lread(STDIN,&file,LREADFILE);
            } else {
                /* Copy a small header */
                if (loop_over_stdin)
                    Lcat(&file,"do forever;"
                               "linein=read();"
                               "if eof(0) then exit;");
                for (;ia<ac; ia++) {
                    Lcat(&file,av[ia]);
                    if (ia<ac-1) Lcat(&file," ");
                }
                /* and a footer */
                if (loop_over_stdin)
                    Lcat(&file,";end");
            }
            if(isTSO())
                RxRun(NULL,&file,args,&tracestr,"TSO");
            else
                RxRun(NULL,&file,args,&tracestr,NULL);
        }
    } else if (staeret == 1) { // Something was caught - the STAE has been cleaned up.

        bzero(systemCompletionCode,4);
        bzero(userCompletionCode,4);

        sprintf(systemCompletionCode,"%02X%1X", sdwa.SDWACMPC[0], (sdwa.SDWACMPC[1] >> 4));
        sprintf(userCompletionCode,  "%1X%02X", sdwa.SDWACMPC[1] & 0xF, sdwa.SDWACMPC[2]);

        /*
        USER HERC01    TRANSMIT  ABEND S013
        PSW   075C1000 00E014BE  ILC 02  INTC 000D
        DATA NEAR PSW  00E014B6  16104100 37860A0D 45E0372A 58204238
        GR 0-3   00E01620  A0013000  000B178C  40E00E9A
        GR 4-7   009A8BF8  019A8F2C  009A8EE4  019A8F2C
        GR 8-11  009A8F04  00014008  58003398  009A8A7C
        GR 12-15 0002AEA0  00000052  80E00F8A  00000018
         */
        fprintf(STDERR, "\nBRX0003E - SYSTEM COMPLETION CODE = %s / USER COMPLETION CODE = %s\n", systemCompletionCode
                                                                                               , userCompletionCode);

        DumpHex((const unsigned char *) &sdwa, 104);

        rxReturnCode = 8;       

        goto TERMINATE;

    } else { // can only be -1 = OS failure
        fprintf(STDERR, "\nBRX0002E - ERROR IN INITIALIZATION OF THE BREXX/370 STAE ROUTINE\n");
    }

TERMINATE:

	/* --- Free everything --- */
	RxFinalize();
    // TODO: call brxterm
    ResetTcpIp();
	for (ia=0; ia<MAXARGS; ia++) LFREESTR(args[ia]);
	LFREESTR(tracestr);
	LFREESTR(file);

#ifdef __DEBUG__
	if (mem_allocated()!=0) {
		fprintf(STDERR,"\nMemory left allocated: %ld\n",mem_allocated());
		mem_list();
	}
#endif

	return rxReturnCode;
} /* main */

void term() {
#ifdef __DEBUG__
    fprintf(STDOUT, "\nBRX0001I - BREXX/370 TERMINATION ROUTINE STARTED\n");
#endif

    setEnvBlock(0);
}
