         MACRO
         RXPGNAME &VAR,&INDEX
         LCLA  &VARLEN
&VARLEN  SETA  K'&VAR                 LENGTH OF VARIABLE NAME
* .... SET VARIABLE NAME .............................................
         XC    VARNAME,VARNAME        SET ADDRESS OF VARIABLE NAME
         MVC   VARNAME(&VARLEN),=C'&VAR' SET ADDRESS OF VARIABLE NAME
         MVA   SHVNAMA,VARNAME        SET ADDRESS OF VARIABLE NAME
         AIF   ('&INDEX' NE '').ISINDX
         MVA   SHVNAML,&VARLEN        LENGTH OF VARIABLE NAME
         MEXIT
.ISINDX  ANOP
         BIN2CHR STRNUM,&INDEX        CONVERT BINARY NUMBER TO CHAR
         LA    R1,IRXBLK              RE-ESTABLISH SHVBLOCK
         LA    RE,&VARLEN             LOAD LENGTH OF STEM ROOT
         LA    RF,6(RE)               ADD 6 FOR INDEX
         ST    RF,SHVNAML             SAVE LENGTH OF VARIABLE NAME
         L     RF,SHVNAMA             LOAD VARIABLE NAME OFFSET
         LA    RF,0(RE,RF)            LOAD OFFSET IN VARIABLE NAME
         MVC   0(6,RF),STRNUM+10      ADD 6 BYTES INDEX
         MEND