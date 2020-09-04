RACAUTH TITLE 'RAC User Authentication Processor'
***********************************************************************
***                                                                 ***
*** Program:  RACAUTH                                               ***
***                                                                 ***
*** Purpose:  C function to authenticate user against RAC           ***
***                                                                 ***
*** Usage:    unsigned int rac_user_auth(char * user, char * pass); ***
***                                                                 ***
***           where user and pass each point to a null terminated   ***
***           string of up to eight characters, the username and    ***
***           the password of the user to be logged in.             ***
***                                                                 ***
***           The following return values are defined:              ***
***                                                                 ***
***                              | return value                     ***
***           -------------------+--------------                    ***
***            RAC not available |       4                          ***
***           -------------------+--------------                    ***
***            auth successful   |       1                          ***
***           -------------------+--------------                    ***
***            auth failed       |       0                          ***
***                                                                 ***
*** Function: 1. Check for resource access control (RAC) being      ***
***              installed and active; return RC(4)                 ***
***                                                                 ***
***           2. Convert user and pass to RAC format (eight         ***
***              characters preceeded by a one byte length field).  ***
***                                                                 ***
***           3. Authenticate user;                                 ***
***                                                                 ***
***           4. Return RC(1) when authentication succeeded         ***
***                                                                 ***
*** Comment:  This code was originally written by Juergen Winkelmann **
***           to extend the FTP server (written by Jason Winter) with *
***           RAC based security functions.                         ***
***                                                                 ***
*** Updates:  2015/03/08 original implementation.                   ***
***           2015/03/14 return ACEE address.                       ***
***           2020/06/18 Tailored for general use                   ***
***                                                                 ***
*** Author:   Juergen Winkelmann, ETH Zuerich.                      ***
***                                                                 ***
*** Maintainer: Peter Jacob, Mike Grossmann                         ***
***                                                                 ***
***********************************************************************
         PRINT NOGEN            no expansions please
RACAUTH CSECT ,                 start of program
         STM   R14,R12,12(R13)  save registers
         L     R2,8(,R13)       \
         LA    R14,96(,R2)       \
         L     R12,0(,R13)        \
         CL    R14,4(,R12)         \
         BL    F1-RACAUTH+4(,R15)   \
         L     R10,0(,R12)           \ save area chaining
         BALR  R11,R10               / and JCC prologue
         CNOP  0,4                  /
F1       DS    0H                  /
         DC    F'96'              /
         STM   R12,R14,0(R2)     /
         LR    R13,R2           /
         LR    R12,R15          establish module addressability
         USING RACAUTH,R12      tell assembler of base
         LR    R11,R1           parameter list
*
* verify RAC availability
*
         LA    R7,4             return code if RAC unavailable
         L     R1,CVTPTR        get CVT address
         ICM   R1,B'1111',CVTSAF(R1) SAFV defined ?
         BZ    LOGNOK           no RAC, allow login
         USING SAFV,R1          addressability of SAFV
         CLC   SAFVIDEN(4),SAFVID SAFV initialized ?
         BNE   LOGNOK           no RAC, allow login
         DROP  R1               SAFV no longer needed
*
* convert C null terminated strings to RAC format
*
         L     R3,0(,R11)       username address
         TRT   0(9,R3),EOS      find end of string
         CR    R1,R3            null string?
         BE    LOGNFAIL         yes -> fail
         SR    R1,R3            length of string
         STC   R1,USER          store length in RAC username field
         BCTR  R1,0             decrement for execute
         EX    R1,MOVEUSER      get username
         L     R3,4(,R11)       password address
         TRT   0(9,R3),EOS      find end of string
         CR    R1,R3            null string?
         BE    LOGNFAIL         yes -> fail
         SR    R1,R3            length of string
         STC   R1,PASS          store length in RAC password field
         BCTR  R1,0             decrement for execute
         EX    R1,MOVEPASS      get password
         OC    USER+1(8),UPPER  translate username to upper case
         OC    PASS+1(8),UPPER  translate password to upper case
*
* enter supervisor state
*
         BSPAUTH ON             become authorized
         MODESET KEY=ZERO,MODE=SUP enter supervisor state
         BSPAUTH OFF            no longer authorized
*
* authenticate user
*
         RACINIT ENVIR=CREATE,USERID=USER,PASSWRD=PASS,ACEE=ACEE
         LTR   R5,R15           authentication successful?
         BNZ   PROB             no -> return to problem state
         RACINIT ENVIR=DELETE,ACEE=ACEE destroy acee
         LA    R5,0
*
* return to problem state
*
PROB     MODESET KEY=NZERO,MODE=PROB back to problem state
         LTR   R5,R5            authentication ok?
         BNZ   LOGNFAIL         no -> signal failure
LOGNOK   LA    R15,1            get return code
         B     RETURN           return to caller
LOGNFAIL LA    R15,0            return (0);
*
* Return to caller
*
RETURN   L     R13,4(,R13)      caller's save area pointer
         L     R14,12(,R13)     restore R14
         LM    R1,R12,24(R13)   restore registers
         BR    R14              return to caller
*
* Executed instructions
*
MOVEUSER MVC   USER+1(0),0(R3)  get username
MOVEPASS MVC   PASS+1(0),0(R3)  get password
*
* Data area
*
ACEE     DS    F                ACEE for authentication
USER     DS    CL9              username
PASS     DS    CL9              password
UPPER    DC    C'        '      for uppercase translation
EOS      DC    X'01',255X'00'   table to find end of string delimiter
SAFVID   DC    CL4'SAFV'        SAFV eye catcher
         YREGS ,                register equates
         CVT   DSECT=YES        map CVT
         IHAPSA ,               map PSA
         IHAASCB ,              map ASCB
         IHAASXB ,              map ASXB
CVTSAF   EQU   248 CVTSAF doesn't exist but is a reserved field in 3.8J
         ICHSAFV  DSECT=YES     map SAFV
         END   RACAUTH          end of RACAUTH
