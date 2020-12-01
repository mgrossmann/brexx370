#define __BMEM_H__

#include "printf.h"
#include "lstring.h"
#include "variable.h"
#include "irx.h"
#include "bintree.h"

int __libc_arch = 0;

// this is needed to enable the linker to find printf
#define printf printf_
#define uintptr_t unsigned long

#define MALLOC(s, d)    malloc_or_die(s)
#define REALLOC(p, s)   realloc_or_die(p,s)
#define	FREE		    free_or_die

#define LFREESTR(s)	{if ((s).pstr) FREE((s).pstr); }

static char line[80];
static int  linePos = 0;

#ifndef SVC
#define SVC
struct SVCREGS {
    int R0;
    int R1;
    int R15;
};
#endif

#ifndef MIN
# define MIN(a,b)	(((a)<(b))?(a):(b))
#endif
#ifndef MAX
# define MAX(a,b)	(((a)>(b))?(a):(b))
#endif

typedef struct shvblock     SHVBLOCK;
typedef struct envblock     ENVBLOCK;
typedef struct workblok_ext WRKBLKEXT;

typedef struct tidentinfo {
    int	id;
    int	stem;
    PBinLeaf leaf[1];
} IdentInfo;

void BRXSVC(int svc, struct SVCREGS *regs);

int fetch (ENVBLOCK *envblock, SHVBLOCK *shvblock);
int set   (ENVBLOCK *envblock, SHVBLOCK *shvblock);
int drop  (ENVBLOCK *envblock, SHVBLOCK *shvblock);
int next  (ENVBLOCK *envblock, SHVBLOCK *shbblock);

/* internal helper functions */
void *getEnvBlock ();
void _tput        (const char *data);
void _clearBuffer ();

/* needed brexx function */
void *malloc_or_die  (size_t size);
void *realloc_or_die (void *ptr, size_t size);
void  free_or_die    (void *ptr);

void _freeVars (void *var);

void Lsccpy (const PLstr to, unsigned char *from);

int IRXEXCOM(char *irxid, void *parm2, void *parm3, SHVBLOCK *shvblock, ENVBLOCK *envblock, int *retVal) {

    int rc = 0;

    // first parameter must be 'IRXEXCOM'
    if (MEMCMP(irxid, "IRXEXCOM", 8) != 0) {
        rc = -1;
    }

    // second and third parameter must be equal
    if (rc == 0) {
        if (MEMCMP(parm2, parm3, 4) != 0) {
            rc = -1;
        }
    }

    // fourth parameter must not be NULL
    if (rc == 0) {
        if (shvblock == NULL) {
            rc = -1;
        }
    }

    // get ENVBLOCK
    if (rc == 0) {
        if (envblock == NULL) {
            envblock = getEnvBlock();
        }

        if (envblock == NULL) {
            rc = -1;
        }
    }

    // check
    if (rc == 0) {
        switch (shvblock->_shvblock_union1._shvblock_struct1._shvcode) {
            case 'F':
                rc = fetch(envblock, shvblock);
                break;

            case 'S':
                rc = set(envblock, shvblock);
                break;

            case 'D':
                rc = drop(envblock, shvblock);
                break;

            case 'N':
                rc = next(envblock, shvblock);
                break;

            default:
                printf("ERR> UNKNOWN SHVCODE GIVEN. => %c \n", shvblock->_shvblock_union1._shvblock_struct1._shvcode);
                rc = -1;
        }
    }

    _clearBuffer();

    return rc;
}

int fetch (ENVBLOCK *envblock, SHVBLOCK *shvblock) {
    int rc;
    int found;

    BinTree *vars;
    PBinLeaf varLeaf;

    Lstr lName;

    if (envblock != NULL) {
        vars      = ((WRKBLKEXT *) envblock->envblock_workblok_ext)->workext_userfield;
    } else {
        vars = NULL;
    }

    if (vars != NULL) {
        lName.pstr = shvblock->shvnama;
        lName.len = shvblock->shvnaml;
        lName.maxlen = shvblock->shvnaml;
        lName.type = LSTRING_TY;

        LASCIIZ(lName)

        varLeaf = BinFind(vars, &lName);
        found = (varLeaf != NULL);
        if (found) {
            MEMCPY(shvblock->shvvala, (LEAFVAL(varLeaf))->pstr, MIN(shvblock->shvbufl, (LEAFVAL(varLeaf))->len));
            shvblock->shvvall = MIN(shvblock->shvbufl, (LEAFVAL(varLeaf))->len);

            rc = 0;
        } else {
            rc = 8;
        }
    } else {
        rc = 12;
    }

    _clearBuffer();

    return rc;
}

int set   (ENVBLOCK *envblock, SHVBLOCK *shvblock) {
    int rc;
    int found;

    BinTree *vars;
    PBinLeaf varLeaf;

    Lstr lName;
    Lstr lValue;
    Lstr aux;

    Variable *var;

    if (envblock != NULL) {
        vars      = ((WRKBLKEXT *) envblock->envblock_workblok_ext)->workext_userfield;
    } else {
        vars = NULL;
    }

    if (vars != NULL) {

        lName.pstr = shvblock->shvnama;
        lName.len = shvblock->shvnaml;
        lName.maxlen = shvblock->shvnaml;
        lName.type = LSTRING_TY;

        lValue.pstr = shvblock->shvvala;
        lValue.len = shvblock->shvvall;
        lValue.maxlen = shvblock->shvvall;
        lValue.type = LSTRING_TY;

        varLeaf = BinFind(vars, &lName);
        found = (varLeaf != NULL);

        if (found) {
            /* Just copy the new value */
            Lstrcpy(LEAFVAL(varLeaf), &lValue);

            rc = 0;
        } else {
            /* added it to the tree */
            /* create memory */
            LINITSTR(aux)
            Lsccpy(&aux, shvblock->shvnama);
            LASCIIZ(aux)
            var = (Variable *) malloc_or_die(sizeof(Variable));
            LINITSTR(var->value)
            Lfx(&(var->value), lValue.len);
            var->exposed = NO_PROC;
            var->stem = NULL;

            varLeaf = BinAdd(vars, &aux, var);
            Lstrcpy(LEAFVAL(varLeaf), &lValue);

            rc = 0;
        }
    } else {
        rc = 12;
    }

    return rc;
}

int drop  (ENVBLOCK *envblock, SHVBLOCK *shvblock) {
    int rc = 0;
    int found = FALSE;

    BinTree *vars;
    PBinLeaf varLeaf;

    BinTree *literals;
    PBinLeaf litLeaf;

    IdentInfo *info = NULL;

    Lstr lName;

    if (envblock != NULL) {
        vars      = ((WRKBLKEXT *) envblock->envblock_workblok_ext)->workext_userfield;
        literals  = envblock->envblock_userfield;
    } else {
        rc = 28;
    }

    if (rc == 0) {
        if (vars != NULL) {
            printf("FOO> variable pool found\n");
            lName.pstr = shvblock->shvnama;
            lName.len = shvblock->shvnaml;
            lName.maxlen = shvblock->shvnaml;
            lName.type = LSTRING_TY;

            LASCIIZ(lName)

            printf("FOO> searching literal  %s\n", LSTR(lName));
            if (literals != NULL) {
                litLeaf = BinFind(literals, &lName);
                if (litLeaf != NULL) {
                    printf("FOO> literal %s found - getting IdentInfo\n", LSTR(lName));
                    info = (IdentInfo*)(litLeaf->value);
                }
            }

            if (info != NULL) {
                printf("FOO> IdentInfo found\n");
                //TODO: Rx_id must be saved in a control block
                //if (info->id == Rx_id) {
                if (info->id == 1) {
                    printf ("FOO> IdentInfo marked as cached\n");
                    varLeaf = info->leaf[0];
                    if (varLeaf != NULL) {
                        printf("FOO> variable found in cache \n");
                        found = TRUE;
                    }
                } else {
                    varLeaf = BinFind(vars, &lName);
                    found = (varLeaf != NULL);
                }
            }

            if (found) {
                Variable *var = (Variable *)(varLeaf->value);

                printf("FOO> Variable %s=%s found\n", LSTR(lName), LSTR(var->value));

                if (!info->stem) {	/* ==== simple variable ==== */
                    if (var->exposed>=0) {	/* --- free only data --- */
                        if (!LISNULL(var->value)) {
                            LFREESTR(var->value);
                            LINITSTR(var->value);
                            Lstrcpy(&(var->value), &lName);
                        }
                        if (var->stem)
                            RxScopeFree(var->stem);
                    } else {
                        printf("FOO> variable %s will be deleted\n", LSTR(lName));
                        BinDel(vars, &lName, _freeVars);

                    }
                }
                printf ("FOO> IdentInfo will be marked as no_cached, now\n");
                info->id = NO_CACHE;
            } else {
                printf ("FOO> no variable could be found\n");
            }

        } else {
            printf ("FOO> no variable pool found\n");
            rc = -1;
        }
    }

    _clearBuffer();

    return rc;
}

int next  (ENVBLOCK *envblock, SHVBLOCK *shvblock) {
    int rc;
    int found;

    BinTree *tree;
    PBinLeaf leaf;

    Lstr lName;

    if (envblock != NULL) {
        tree = envblock->envblock_userfield;
    } else {
        tree = NULL;
    }

    if (tree != NULL) {
        lName.pstr = shvblock->shvnama;
        lName.len = shvblock->shvnaml;
        lName.maxlen = shvblock->shvnaml;
        lName.type = LSTRING_TY;

        LASCIIZ(lName)

        leaf = BinFind(tree, &lName);
        found = (leaf != NULL);
        if (found) {
            MEMCPY(shvblock->shvvala, (LEAFVAL(leaf))->pstr, MIN(shvblock->shvbufl, (LEAFVAL(leaf))->len));
            shvblock->shvvall = MIN(shvblock->shvbufl, (LEAFVAL(leaf))->len);

            rc = 0;
        } else {
            rc = 8;
        }
    } else {
        rc = 12;
    }

    _clearBuffer();

    return rc;
}

void *_getEctEnvBk() {
    void **psa;           // PAS      =>   0 / 0x00
    void **ascb;          // PSAAOLD  => 548 / 0x224
    void **asxb;          // ASCBASXB => 108 / 0x6C
    void **lwa;           // ASXBLWA  =>  20 / 0x14
    void **ect;           // LWAPECT  =>  32 / 0x20
    void **ectenvbk;      // ECTENVBK =>  48 / 0x30

    psa = 0;
    ascb = psa[137];
    asxb = ascb[27];
    lwa = asxb[5];
    ect = lwa[8];

    ectenvbk = ect + 12;   // 12 * 4 = 48

    return ectenvbk;
}

void *getEnvBlock() {
    void **ectenvbk;
    ENVBLOCK *envblock;

    ectenvbk = _getEctEnvBk();
    envblock = *ectenvbk;

    if (envblock != NULL) {
        if (strncmp((char *) envblock->envblock_id, "ENVBLOCK", 8) == 0) {
            return envblock;
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

/* needed LSTRING functions */
void Lfx(const PLstr s, const size_t len) {
    size_t max;

    if (LISNULL(*s)) {
        LSTR(*s) = (unsigned char *) malloc_or_die((max = LNORMALISE(len)) + LEXTRA);
        memset(LSTR(*s), 0, max);
        LLEN(*s) = 0;
        LMAXLEN(*s) = max;
        LTYPE(*s) = LSTRING_TY;
    } else if (LMAXLEN(*s) < len) {
        LSTR(*s) = (unsigned char *) realloc_or_die(LSTR(*s), (max = LNORMALISE(len)) + LEXTRA);
        LMAXLEN(*s) = max;
    }
}

void Lsccpy(const PLstr to, unsigned char *from) {
    size_t len;

    if (!from)
        Lfx(to, len = 0);
    else {
        Lfx(to, len = strlen((const char *) from));
        MEMCPY(LSTR(*to), from, len);
    }
    LLEN(*to) = len;
    LTYPE(*to) = LSTRING_TY;
}

void Lstrcpy(const PLstr to, const PLstr from) {
    if (LISNULL(*to)) {
        Lfx(to, 31);
    }

    if (LLEN(*from) == 0) {
        LLEN(*to) = 0;
        LTYPE(*to) = LSTRING_TY;
    } else {
        if (LMAXLEN(*to) <= LLEN(*from)) Lfx(to, LLEN(*from));
        switch (LTYPE(*from)) {
            case LSTRING_TY:
                MEMCPY(LSTR(*to), LSTR(*from), LLEN(*from));
                break;

            case LINTEGER_TY:
                LINT(*to) = LINT(*from);
                break;

            case LREAL_TY:
                LREAL(*to) = LREAL(*from);
                break;
        }
        LTYPE(*to) = LTYPE(*from);
        LLEN(*to) = LLEN(*from);
    }
}

int _Lstrcmp(const PLstr a, const PLstr b) {
    int r;

    if ((r = MEMCMP(LSTR(*a), LSTR(*b), MIN(LLEN(*a), LLEN(*b)))) != 0)
        return r;
    else {
        if (LLEN(*a) > LLEN(*b))
            return 1;
        else if (LLEN(*a) == LLEN(*b)) {
            if (LTYPE(*a) > LTYPE(*b))
                return 1;
            else if (LTYPE(*a) < LTYPE(*b))
                return -1;
            return 0;
        } else
            return -1;
    }
}

/* mvs memory allocation */
void * _getmain       (size_t length) {
    long *ptr;

    struct SVCREGS registers;
    registers.R0 = ((int)length) + 12;
    registers.R1 = -1;
    registers.R15 = 0;

    BRXSVC(10, &registers);

    if (registers.R15 == 0) {
        ptr = (void *) (uintptr_t) registers.R1;
        ptr[0] = 0xDEADBEAF;
        ptr[1] = (((long) (ptr)) + 12);
        ptr[2] = length;
    } else {
        ptr = NULL;
    }

    return (void *) (((uintptr_t) (ptr)) + 12);
}

void   _freemain      (void *ptr) {
    struct SVCREGS registers;
    registers.R0 = 0;
    registers.R1 = (int) (uintptr_t) ptr;
    registers.R15 = -1;

    BRXSVC(10, &registers);
}

void * malloc_or_die  (size_t size) {
    void *nPtr;

    nPtr = _getmain(size);
    if (!nPtr) {
        _tput("ERR>   GETMAIN FAILED");
        return NULL;
    }

    return nPtr;
}

void * realloc_or_die (void *oPtr, size_t size) {
    void *nPtr;

    nPtr = _getmain(size);

    if (!nPtr) {
        _tput("ERR>   GETMAIN FAILED");
        return NULL;
    }

    return nPtr;

    /* TODO: added beacause get rid of failed realloc's */
    /*
    size++;

    ptr = realloc(ptr,size);

    if (!ptr) {
        Lstr lerrno;

        LINITSTR(lerrno)
        Lfx(&lerrno,31);

        Lscpy(&lerrno,strerror(errno));

        Lerror(ERR_REALLOC_FAILED,0);
        fprintf(stderr, "errno: %s\n",strerror(errno));

        LFREESTR(lerrno);
        raise(SIGSEGV);
    }

    return ptr;
    */
}

void   free_or_die    (void *ptr) {
    if (ptr != NULL) {
        _freemain(ptr);
    }
}

/* internal terminal i/o */
void _tput(const char *data) {
    struct SVCREGS registers;

    registers.R0 = strlen(data);
    registers.R1 = (unsigned int) data & 0x00FFFFFF;
    registers.R15 = 0;

    BRXSVC(93, &registers);
}

void _putchar(char character) {
    line[linePos] = character;
    linePos++;

    if (character ==  '\n' || linePos == 80) {
        _tput(line);
        bzero(line, 80);
        linePos = 0;
    }
}

void _clearBuffer() {
    if (linePos > 0) {
        _tput(line);
        bzero(line, 80);
        linePos = 0;
    }
}

/* other stuff to be checked TODO: try to ged rid of this stuff */
void RxScopeFree(Scope scope) {
    printf("FOO> RxScopeFree \n");
    if (scope) {
        printf("FOO> RxScopeFree -  Scope is NULL\n");
        BinDisposeLeaf(&(scope[0]),scope[0].parent,_freeVars);
    }
}

void _freeVars(void *var) {

    Variable	*v;

    v = (Variable*)var;
    if (v->exposed==NO_PROC) {
        if (v->stem) {
            printf("FOO> freeing a STEM");
            RxScopeFree(v->stem);
            FREE(v->stem);
        }
        LFREESTR(v->value);
        FREE(var);
    } else {
        printf("FOO> RxVarFree PROC\n");
        /*
        if (v->exposed == _rx_proc-1) {
            v->exposed = NO_PROC;
        }
        */
    }

}

/* dummy impls */
int  Lstrbeg (const PLstr str, const PLstr pre) {
    _tput("FOO> DUMMY LSTRBEG CALLED");
    return 0;
}

long Lwords  (const PLstr from) {
    _tput("FOO> DUMMY LWORDSCALLED");
    return 0;
}

void Lword   (const PLstr to, const PLstr from, long n) {
    _tput("FOO> DUMMY LWORD CALLED");
}

void L2str   (const PLstr s )
{
    _tput("FOO> DUMMY L2STR CALLED");
}

int  Lstrcmp (const PLstr a, const PLstr b) {
    _tput("FOO> DUMMY LSTRCMP CALLED");
    return 0;
}

void Lcat    (const PLstr to, const char *from) {
    _tput("FOO> DUMMY LCAT CALLED");
}

void Lscpy   (const PLstr to, const char *from) {
    _tput("FOO> DUMMY Lscpy CALLED");
}

#ifdef __CROSS__
void BRXSVC(int svc, struct SVCREGS *regs) {

}
#endif

/*
*
*     Return Code Flags (Stored in SHVRET):
*
SHVCLEAN EQU   X'00' Execution was OK
SHVNEWV  EQU   X'01' Variable did not exist
SHVLVAR  EQU   X'02' Last variable transferred (for "N")
SHVTRUNC EQU   X'04' Truncation occurred during "Fetch"
SHVBADN  EQU   X'08' Invalid variable name
SHVBADV  EQU   X'10' Value too long
SHVBADF  EQU   X'80' Invalid function code (SHVCODE)
*/