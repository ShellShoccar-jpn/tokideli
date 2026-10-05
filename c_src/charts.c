/*####################################################################
#
# CHARTS - Print the Current Timestamp at the Top of Each Character
#
#          This command attaches the current time to the top of each
#          locale-aware character (a single byte in the "C" locale, or
#          possibly several bytes in a multibyte locale such as UTF-8)
#          read from the input, then writes that character on a record
#          of its own. Strictly speaking, "the current time" means the
#          instant when this command starts writing the character which
#          has been read.
#
# USAGE   : charts [-0|-3|-6|-9] [-c|-e|-I|-z|-Z] [-1bdu] [file [...]]
# Args    : file ...... Filepath to be attached the current timestamp
#                       ("-" means STDIN)
# Options : -0,-3,-6,-9 Specify resolution unit of the time. For instance,
#                       timestamp becomes "YYYYMMDDhhmmss.nnn" when
#                       "-3" option is set.
#                       You have to set one of them.
#                         -0 ... second (default)
#                         -3 ... millisecond
#                         -6 ... microsecond
#                         -9 ... nanosecond
#           -c,-e,-I,   Specify the format for timestamp. You can choose
#           -z,-Z       one of them.
#                         -c ... "YYYYMMDDhhmmss[.n]" (default)
#                                Calendar-time (standard time) in your
#                                timezone (".n" is the digits under
#                                second. It will be attached when -3 or
#                                -6 or -9 option is specified)
#                         -e ... "n[.n]"
#                                The number of seconds since the UNIX
#                                epoch (".n" is the same as -c)
#                         -I ... "YYYY-MM-DDThh:mm:ss[,n]{+|-}hh:mm"
#                                The extended ISO 8601 format
#                                (",n" is the same as -c)
#                         -z ... "n[.n]"
#                                The number of seconds since this command
#                                started (".n" is the same as -c)
#                         -Z ... "n[.n]"
#                                The number of seconds since the first
#                                character came (".n" is the same as -c)
#           -1 ........ * Output one LF character at first before
#                         outputting the incoming data.
#                       * This option might work as a starter of the
#                         system embedding this command.
#           -b ........ Force byte mode: always treat each single byte
#                       of the input as one character, regardless of the
#                       current locale. This also skips calling
#                       setlocale()/mbrtowc() entirely, so it works as a
#                       faster path, too.
#           -d ........ Insert "delta-t" (the number of seconds since
#                       started writing the previous character) into the
#                       next to the current timestamp. So, two fields
#                       will be attatched when using this option.
#           -u ........ Set the date in UTC when -c option is set
#                       (same as that of date command)
# Retuen  : Return 0 only when finished successfully
#
# How to compile : cc -O3 -std=c99 -o __CMDNAME__ __SRCNAME__ -lrt
#                  (if it doesn't work)
# How to compile : cc -O3 -std=c99 -o __CMDNAME__ __SRCNAME__
#
# Written by Shell-Shoccar Japan (@shellshoccarjpn) on 2026-10-06
#
# This is a public-domain software (CC0). It means that all of the
# people can use this for any purposes with no restrictions at all.
# By the way, We are fed up with the side effects which are brought
# about by the major licenses.
#
# The latest version is distributed at the following page.
# https://github.com/ShellShoccar-jpn/tokideli
#
####################################################################*/



/*####################################################################
# Initial Configuration
####################################################################*/

/*=== Initial Setting ==============================================*/

/*--- headers ------------------------------------------------------*/
/* Solaris 11.3's <sys/feature_tests.h> only recognizes the exact
 * values _XOPEN_SOURCE==600 / _POSIX_C_SOURCE==200112L for its UNIX 03
 * detection and has no notion of POSIX.1-2008/SUSv4 at all; requesting
 * 700/200809L there trips its strict conformance-level check and
 * aborts the build, so __EXTENSIONS__ (which sidesteps that check
 * entirely and exposes every POSIX/XSI/BSD interface regardless of C
 * standard level) is used there instead. Everywhere else, we ask for
 * _XOPEN_SOURCE 700 rather than _POSIX_C_SOURCE 200809L alone: on
 * FreeBSD, _POSIX_C_SOURCE alone leaves __XSI_VISIBLE unset, hiding
 * XSI interfaces (e.g. SA_SIGINFO/sa_sigaction, S_IFMT/S_IFREG) that
 * some of these commands need; _XOPEN_SOURCE 700 enables both.       */
#if defined(__sun) || defined(__SVR4)
  #define __EXTENSIONS__
#else
  #define _XOPEN_SOURCE 700 /* for setenv() */
#endif
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

/*--- macro constants ----------------------------------------------*/
/* Buffer size for a timestamp string */
#define LINE_BUF         80
#ifndef CLOCK_MONOTONIC
  #define CLOCK_MONOTONIC CLOCK_REALTIME /* for HP-UX */
#endif

/*--- data type definitions ----------------------------------------*/
typedef struct timespec tmsp;

/*--- prototype functions ------------------------------------------*/
int  get_1byte(FILE *fp);
void unget_1byte(int iChar);
int  fetch_1char(FILE *fp, char *pszUnit, int *piLen);
void write_1char(const char *pszUnit, int iLen);
int  read_1char_unit(FILE *fp);
int  read_c1st_1unit(FILE *fp);
int  read_e1st_1unit(FILE *fp);
int  read_I1st_1unit(FILE *fp);
int  read_Z1st_1unit(FILE *fp);
void print_cur_timestamp(void);

/*--- global variables ---------------------------------------------*/
char*     gpszCmdname      ; /* The name of this command                      */
int       giVerbose   =  0 ; /* speaks more verbosely by the greater number   */
int       giFmtType   = 'c'; /* 'c':calendar-time (default)
                                 'e':UNIX-epoch-time
                                 'z':command-running-sec                      */
int       giTimeResol =  0 ; /* 0:second(def) 3:millisec 6:microsec 9:nanosec */
int       giDeltaMode =  0 ; /* attach the number of seconds since printing
                                 the previous character after the timestamp
                                 when >0                                      */
int       giByteMode  =  0 ; /* 1 if -b (force byte mode) is given            */
tmsp      gtsZero     = {0}; /* Time this command booted                      */
tmsp      gtsPrev     = {0}; /* Time the previous character has come         */
int       giHold      =  0 ; /* for read_1char_unit(): 1 if the next
                                 character has already been fetched           */
char      gszNextchar[MB_LEN_MAX]; /* for read_1char_unit(): raw bytes of
                                       the next character                     */
int       giNextlen       ; /* for read_1char_unit(): length of gszNextchar  */
mbstate_t gmbsState    ={0}; /* shift state used by mbrtowc()                 */
char      gszPushback[MB_LEN_MAX]; /* internal byte-level pushback stack     */
int       giPushbacklen= 0 ; /* number of bytes currently pushed back        */

/*=== Define the functions for printing usage and error ============*/

/*--- exit with usage ----------------------------------------------*/
void print_usage_and_exit(void) {
  fprintf(stderr,
    "USAGE   : %s [-0|-3|-6|-9] [-c|-e|-I|-z|-Z] [-1bdu] [file [...]]\n"
    "Args    : file ...... Filepath to be attached the current timestamp\n"
    "                      (\"-\" means STDIN)\n"
    "Options : -0,-3,-6,-9 Specify resolution unit of the time. For instance,\n"
    "                      timestamp becomes \"YYYYMMDDhhmmss.nnn\" when\n"
    "                      \"-3\" option is set. \n"
    "                      You have to set one of them.\n"
    "                        -0 ... second (default)\n"
    "                        -3 ... millisecond\n"
    "                        -6 ... microsecond\n"
    "                        -9 ... nanosecond\n"
    "          -c,-e,-I,   Specify the format for timestamp. You can choose\n"
    "          -z,-Z       one of them.\n"
    "                        -c ... \"YYYYMMDDhhmmss[.n]\" (default)\n"
    "                               Calendar-time (standard time) in your\n"
    "                               timezone (\".n\" is the digits under\n"
    "                               second. It will be attached when -3 or\n"
    "                               -6 or -9 option is specified)\n"
    "                        -e ... \"n[.n]\"\n"
    "                               The number of seconds since the UNIX\n"
    "                               epoch (\".n\" is the same as -c)\n"
    "                        -I ... \"YYYY-MM-DDThh:mm:ss[,n]{+|-}hh:mm\"\n"
    "                               The extended ISO 8601 format\n"
    "                               (\",n\" is the same as -c)\n"
    "                        -z ... \"n[.n]\"\n"
    "                               The number of seconds since this command\n"
    "                               started (\".n\" is the same as -c)\n"
    "                        -Z ... \"n[.n]\"\n"
    "                               The number of seconds since the first\n"
    "                               character came (\".n\" is the same as -c)\n"
    "          -1 ........ * Output one LF character at first before\n"
    "                        outputting the incoming data.\n"
    "                      * This option might work as a starter of the\n"
    "                        system embedding this command.\n"
    "          -b ........ Force byte mode: always treat each single byte\n"
    "                      of the input as one character, regardless of\n"
    "                      the current locale. This also skips calling\n"
    "                      setlocale()/mbrtowc() entirely, so it works as\n"
    "                      a faster path, too.\n"
    "          -d ........ Insert \"delta-t\" (the number of seconds since\n"
    "                      started writing the previous character) into\n"
    "                      the next to the current timestamp. So, two\n"
    "                      fields will be attatched when using this option.\n"
    "          -u ........ Set the date in UTC when -c option is set\n"
    "                      (same as that of date command)\n"
    "Retuen  : Return 0 only when finished successfully\n"
    "Version      : 1.0.0\n"
    "Last Updated : 2026-10-06 00:55:00 JST\n"
    "               (POSIX C language)\n"
    "\n"
    "Shell-Shoccar Japan (@shellshoccarjpn), No rights reserved.\n"
    "This is public domain software. (CC0)\n"
    "\n"
    "The latest version is distributed at the following page.\n"
    "https://github.com/ShellShoccar-jpn/tokideli\n"
    ,gpszCmdname);
  exit(1);
}

/*--- print warning message ----------------------------------------*/
void warning(const char* szFormat, ...) {
  va_list va;
  va_start(va, szFormat);
  fprintf(stderr,"%s: ",gpszCmdname);
  vfprintf(stderr,szFormat,va);
  va_end(va);
  return;
}

/*--- exit with error message --------------------------------------*/
void error_exit(int iErrno, const char* szFormat, ...) {
  va_list va;
  va_start(va, szFormat);
  fprintf(stderr,"%s: ",gpszCmdname);
  vfprintf(stderr,szFormat,va);
  va_end(va);
  exit(iErrno);
}


/*####################################################################
# Main
####################################################################*/

/*=== Initialization ===============================================*/
int main(int argc, char *argv[]) {

/*--- Variables ----------------------------------------------------*/
int      iRet;            /* return code                            */
int      iRet_r1u;        /* return value by read_1char_unit()      */
int      iOpt_1=0;        /* -1 option flag (default 0)             */
char    *pszPath;         /* filepath on arguments                  */
char    *pszFilename;     /* filepath (for message)                 */
int      iFileno;         /* file# of filepath                      */
int      iFd;             /* file descriptor                        */
FILE    *fp;              /* file handle                            */
int      iFirstchar='c';  /* >0 when x-opt and no char come yet     */
int      i;               /* all-purpose int                        */

/*--- Initialize ---------------------------------------------------*/
if (clock_gettime(CLOCK_REALTIME,&gtsZero) != 0) {
  error_exit(errno,"clock_gettime() at initialize: %s\n",strerror(errno));
}
gpszCmdname = argv[0];
for (i=0; *(gpszCmdname+i)!='\0'; i++) {
  if (*(gpszCmdname+i)=='/') {gpszCmdname=gpszCmdname+i+1;}
}

if (argc>=2 && strcmp(argv[1],"--version")==0) {
  printf("%s (tokideli) 1.0.0\n", gpszCmdname);
  return 0;
}
if (setenv("POSIXLY_CORRECT","1",1) < 0) {
  error_exit(errno,"setenv() at initialization: \n", strerror(errno));
}

/*=== Parse arguments ==============================================*/

/*--- Parse options which start by "-" -----------------------------*/
while ((i=getopt(argc, argv, "0369ceIzZ1bduvh")) != -1) {
  switch (i) {
    case '0': giTimeResol =  0 ;                 break;
    case '3': giTimeResol =  3 ;                 break;
    case '6': giTimeResol =  6 ;                 break;
    case '9': giTimeResol =  9 ;                 break;
    case 'c': giFmtType   = 'c'; iFirstchar='c'; break;
    case 'e': giFmtType   = 'e'; iFirstchar='e'; break;
    case 'I': giFmtType   = 'I'; iFirstchar='I'; break;
    case 'Z': giFmtType   = 'Z'; iFirstchar='Z'; break;
    case 'z': giFmtType   = 'z'; iFirstchar='z'; break;
    case '1': iOpt_1      =  1 ;                 break;
    case 'b': giByteMode  =  1 ;                 break;
    case 'd': giDeltaMode =  1 ;                 break;
    case 'u': (void)setenv("TZ", "UTC0", 1);     break;
    case 'v': giVerbose++      ;                 break;
    case 'h': print_usage_and_exit();
    default : print_usage_and_exit();
  }
}
argc -= optind-1;
argv += optind  ;
if (giVerbose>0) {warning("verbose mode (level %d)\n",giVerbose);}

/*=== Set up locale handling (skipped entirely when -b is given) ===*/
/* setlocale() fails silently (just returns NULL, leaving the "C" locale
 * in effect) if the locale named by LC_ALL/LC_CTYPE/LANG isn't actually
 * installed on this system. Left unreported, that looks indistinguishable
 * from a real bug: every non-ASCII byte then gets flagged one-by-one as
 * an "invalid byte" by fetch_1char(), because the "C" locale has no
 * multibyte characters to decode them into.                            */
if (! giByteMode && setlocale(LC_CTYPE, "") == NULL) {
  warning("failed to set the locale requested via LC_ALL/LC_CTYPE/LANG; "
          "falling back to the \"C\" locale, so multibyte characters "
          "will be reported as invalid bytes. Install/generate the "
          "requested locale, or specify one that is actually available "
          "(see: locale -a), or use -b if byte-mode processing is what "
          "you actually want.\n");
}

/*=== Switch buffer mode ===========================================*/
if (setvbuf(stdout,NULL,_IOLBF,0)!=0) {
  error_exit(255,"Failed to switch to line-buffered mode\n");
}

/*=== Output the starter charater/line when -1 is enabled ==========*/
if (iOpt_1 && putchar('\n')==EOF) {
  error_exit(errno, "putchar() in main(): %s\n", strerror(errno));
}

/*=== Each file loop ===============================================*/
iRet     =  0;
iFileno  =  0;
iFd      = -1;
iRet_r1u =  0;
while ((pszPath = argv[iFileno]) != NULL || iFileno == 0) {

  /*--- Open one of the input files --------------------------------*/
  if (pszPath == NULL || strcmp(pszPath, "-") == 0) {
    pszFilename = "stdin"                ;
    iFd         = STDIN_FILENO           ;
  } else                                            {
    pszFilename = pszPath                ;
    while ((iFd=open(pszPath, O_RDONLY)) < 0) {
      if (errno == EINTR) {continue;}
      iRet = 1;
      warning("%s: %s\n",pszFilename,strerror(errno));
      iFileno++;
      break;
    }
    if (iFd < 0) {continue;}
  }
  if (iFd == STDIN_FILENO) {
    fp = stdin;
    if (feof(stdin)) {clearerr(stdin);} /* Reset EOF condition when stdin */
  } else                   {
    fp = fdopen(iFd, "r");
  }

  /*--- Reset the per-file multibyte-decoding state -----------------*/
  giPushbacklen = 0;
  memset(&gmbsState, 0, sizeof(gmbsState));

  /*--- Reading and writing loop -----------------------------------*/
  if (! feof(fp)) {
    switch(iFirstchar) {
      case 'c': iRet_r1u=read_c1st_1unit(fp); iFirstchar=0;               break;
      case 'e': iRet_r1u=read_e1st_1unit(fp); iFirstchar=0;               break;
      case 'I': iRet_r1u=read_I1st_1unit(fp); iFirstchar=0;               break;
      case 'z': gtsPrev.tv_sec =gtsZero.tv_sec;
                gtsPrev.tv_nsec=gtsZero.tv_nsec;
                iRet_r1u=0;                   iFirstchar=0;               break;
      case 'Z': iRet_r1u=read_Z1st_1unit(fp); iFirstchar=0;giFmtType='z'; break;
      default : iRet_r1u=0;
    }
    while (iRet_r1u==0) {iRet_r1u=read_1char_unit(fp);}
  }

  /*--- Close the input file ---------------------------------------*/
  if (fp != stdin) {fclose(fp);}

  /*--- End loop ---------------------------------------------------*/
  if (pszPath == NULL) {break;}
  iFileno++;
}

/*=== Finish normally ==============================================*/
return(iRet);}



/*####################################################################
# Functions
####################################################################*/

/*=== Get one byte, honoring the internal pushback stack first =======
 * [in]  gszPushback    : (must be defined as a global variable)
 *       giPushbacklen  : (must be defined as a global variable)
 * [ret] 0-255          : The byte which has been read
 *       EOF            : Reached the end of the file                */
int get_1byte(FILE *fp) {
  if (giPushbacklen > 0) {return (unsigned char)gszPushback[--giPushbacklen];}
  return getc(fp);
}


/*=== Push one byte back, to be returned by the next get_1byte() call =
 * [in]  gszPushback    : (must be defined as a global variable)
 *       giPushbacklen  : (must be defined as a global variable)      */
void unget_1byte(int iChar) {
  gszPushback[giPushbacklen++] = (char)iChar;
  return;
}


/*=== Fetch the raw bytes of exactly one locale-aware character ======
 * [in]  giByteMode     : (must be defined as a global variable)
 *       gmbsState      : (must be defined as a global variable)
 * [out] pszUnit        : the raw bytes of the character fetched
 *       piLen          : the number of bytes written into pszUnit
 * [ret] 0              : Successfully fetched one character
 *       EOF            : There was no more data to fetch at all      */
int fetch_1char(FILE *fp, char *pszUnit, int *piLen) {

  /*--- Variables --------------------------------------------------*/
  int     iC, i, iN=0;
  size_t  sRet;
  wchar_t wc;
  char    szRaw[MB_LEN_MAX];

  /*--- Byte-mode fast path (-b) -------------------------------------*/
  if (giByteMode) {
    iC = get_1byte(fp);
    if (iC == EOF) {return EOF;}
    pszUnit[0] = (char)iC;
    *piLen     = 1;
    return 0;
  }

  /*--- Locale-aware path, one byte at a time ------------------------*/
  while (1) {
    iC = get_1byte(fp);
    if (iC == EOF) {
      if (iN == 0) {return EOF;} /* clean EOF, nothing pending        */
      warning("an incomplete multibyte character (%d byte(s)) was cut "
              "off by EOF; flushing it as the last character\n", iN);
      memcpy(pszUnit, szRaw, (size_t)iN);
      *piLen = iN;
      memset(&gmbsState, 0, sizeof(gmbsState));
      return 0;
    }
    szRaw[iN] = (char)iC;
    /* Feed mbrtowc() exactly the one new byte, not the whole buffer
     * accumulated so far: gmbsState already carries everything it
     * learned from the earlier bytes of this same character, so
     * resubmitting them here would make it consume them twice.      */
    sRet = mbrtowc(&wc, &szRaw[iN], 1, &gmbsState);
    iN++;
    if (sRet == (size_t)-2 && iN < MB_LEN_MAX) {continue;} /* need more */
    if (sRet == (size_t)-1 || sRet == (size_t)-2) {
      /* Invalid byte sequence (or one that still isn't complete even
       * after MB_LEN_MAX bytes, which the locale shouldn't allow but
       * is handled defensively anyway). Recover by emitting only the
       * first byte as its own character, and push the rest back so
       * they get re-examined as a fresh sequence next time.          */
      warning("an invalid byte (0x%02x) was replaced with a 1-byte "
              "character\n", (unsigned char)szRaw[0]);
      for (i=iN-1; i>=1; i--) {unget_1byte(szRaw[i]);}
      pszUnit[0] = szRaw[0];
      *piLen     = 1;
      memset(&gmbsState, 0, sizeof(gmbsState));
      return 0;
    }
    /* sRet==0 (an embedded NUL) or sRet>0 (a complete character)     */
    memcpy(pszUnit, szRaw, (size_t)iN);
    *piLen = iN;
    return 0;
  }
}


/*=== Write the raw bytes of one character, then a trailing newline ==
 * The trailing newline is omitted when the character itself already
 * is a single '\n' byte, since that byte already terminates the
 * record by itself.                                                 */
void write_1char(const char *pszUnit, int iLen) {

  /*--- Variables --------------------------------------------------*/
  int i;

  /*--- Write the character's raw bytes -----------------------------*/
  for (i=0; i<iLen; i++) {
    while (putchar((unsigned char)pszUnit[i])==EOF) {
      if (errno == EINTR) {continue;}
      error_exit(errno,"write_1char(): putchar() #1: %s\n",strerror(errno));
    }
  }

  /*--- Write the forced trailing newline, unless already present --*/
  if (! (iLen==1 && pszUnit[0]=='\n')) {
    while (putchar('\n')==EOF) {
      if (errno == EINTR) {continue;}
      error_exit(errno,"write_1char(): putchar() #2: %s\n",strerror(errno));
    }
  }
  return;
}


/*=== Read and write only one character ===============================
 * [in]  giHold     : (must be defined as a global variable)
 *       gszNextchar: (must be defined as a global variable)
 *       giNextlen  : (must be defined as a global variable)
 * [ret] 0          : Finished reading/writing one character,
 *                    and more data may follow
 *       1          : Finished reading/writing one character,
 *                    which was also the last one of the file
 *       EOF        : There was no character to read at all          */
int read_1char_unit(FILE *fp) {

  /*--- Variables --------------------------------------------------*/
  char szUnit[MB_LEN_MAX];
  int  iLen;

  /*--- Fetch the character (reuse the lookahead if we have one) ---*/
  if (giHold) {
    memcpy(szUnit, gszNextchar, (size_t)giNextlen);
    iLen = giNextlen;
    giHold = 0;
  } else {
    if (fetch_1char(fp,szUnit,&iLen)==EOF) {return EOF;}
  }

  /*--- Print the timestamp, then the character ---------------------*/
  print_cur_timestamp();
  write_1char(szUnit, iLen);

  /*--- Look ahead by one character to decide the return value ------*/
  if (fetch_1char(fp,gszNextchar,&giNextlen)!=EOF) {giHold=1; return 0;}
  else                                             {          return 1;}
}


/*=== Read and write only one character (for c-option and 1st char) ===
 * [in]  giHold     : (must be defined as a global variable)
 *       gszNextchar: (must be defined as a global variable)
 *       giNextlen  : (must be defined as a global variable)
 * [ret] 0          : Finished reading/writing one character,
 *                    and more data may follow
 *       1          : Finished reading/writing one character,
 *                    which was also the last one of the file
 *       EOF        : There was no character to read at all          */
int read_c1st_1unit(FILE *fp) {

  /*--- Variables --------------------------------------------------*/
  tmsp       tsNow, ts;
  struct tm *ptm  ;
  char       szUnit[MB_LEN_MAX];
  int        iLen ;

  /*--- Fetch the first character -----------------------------------*/
  if (fetch_1char(fp,szUnit,&iLen)==EOF) {return EOF;}
  if (clock_gettime(CLOCK_REALTIME,&tsNow) != 0) {
    error_exit(errno,"read_c1st_1unit(): clock_gettime(): %s\n",
                     strerror(errno)                            );
  }
  /* Apply the same half-up-to-the-next-second rounding carry as
   * print_cur_timestamp() does, and BEFORE calling localtime(), so the
   * displayed second and fractional part always agree with each other
   * and with how every later character's timestamp gets rounded. Doing
   * this inconsistently between the first character and the rest is
   * what let the first character's displayed time occasionally look
   * LATER (or, relatively, let a later character look EARLIER) than it
   * should.                                                           */
  ts.tv_sec=tsNow.tv_sec; ts.tv_nsec=tsNow.tv_nsec;
  switch (giTimeResol) {
    case 0 : if (ts.tv_nsec>=500000000L) {ts.tv_sec++;ts.tv_nsec=0;} break;
    case 3 : if (ts.tv_nsec>=999500000L) {ts.tv_sec++;ts.tv_nsec=0;} break;
    case 6 : if (ts.tv_nsec>=999999500L) {ts.tv_sec++;ts.tv_nsec=0;} break;
    default: break; /* case 9: shown at full precision, no rounding */
  }
  if ((ptm=localtime(&ts.tv_sec)) == NULL) {
    error_exit(255,"read_c1st_1unit(): localtime(): returned NULL\n");
  }
  switch (giTimeResol) {
    case 0 : printf("%04d%02d%02d%02d%02d%02d "      ,
               ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
               ptm->tm_hour     , ptm->tm_min  , ptm->tm_sec  ); break;
    case 3 : printf("%04d%02d%02d%02d%02d%02d.%03ld " ,
               ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
               ptm->tm_hour     , ptm->tm_min  , ptm->tm_sec ,
               ts.tv_nsec/1000000                             ); break;
    case 6 : printf("%04d%02d%02d%02d%02d%02d.%06ld " ,
               ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
               ptm->tm_hour     , ptm->tm_min  , ptm->tm_sec ,
               ts.tv_nsec/1000                                ); break;
    case 9 : printf("%04d%02d%02d%02d%02d%02d.%09ld ",
               ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
               ptm->tm_hour     , ptm->tm_min  , ptm->tm_sec ,
               ts.tv_nsec                                     ); break;
    default: error_exit(255,"read_c1st_1unit(): Unknown resolution\n");
  }
  if (giDeltaMode) {
    printf("0 ");
    gtsPrev.tv_sec=tsNow.tv_sec; gtsPrev.tv_nsec=tsNow.tv_nsec;
  }
  write_1char(szUnit, iLen);

  /*--- Look ahead by one character to decide the return value ------*/
  if (fetch_1char(fp,gszNextchar,&giNextlen)!=EOF) {giHold=1; return 0;}
  else                                             {          return 1;}
}


/*=== Read and write only one character (for e-option and 1st char) ===
 * [in]  giHold     : (must be defined as a global variable)
 *       gszNextchar: (must be defined as a global variable)
 *       giNextlen  : (must be defined as a global variable)
 * [ret] 0          : Finished reading/writing one character,
 *                    and more data may follow
 *       1          : Finished reading/writing one character,
 *                    which was also the last one of the file
 *       EOF        : There was no character to read at all          */
int read_e1st_1unit(FILE *fp) {

  /*--- Variables --------------------------------------------------*/
  tmsp       tsNow, ts     ;
  char       szUnit[MB_LEN_MAX];
  int        iLen           ;

  /*--- Fetch the first character -----------------------------------*/
  if (fetch_1char(fp,szUnit,&iLen)==EOF) {return EOF;}
  if (clock_gettime(CLOCK_REALTIME,&tsNow) != 0) {
    error_exit(errno,"read_e1st_1unit(): clock_gettime(): %s\n",
                     strerror(errno)                            );
  }
  /* Same rounding carry as print_cur_timestamp(); see the comment in
   * read_c1st_1unit() for why this must be applied here, too.        */
  ts.tv_sec=tsNow.tv_sec; ts.tv_nsec=tsNow.tv_nsec;
  switch (giTimeResol) {
    case 0 : if (ts.tv_nsec>=500000000L) {ts.tv_sec++;ts.tv_nsec=0;} break;
    case 3 : if (ts.tv_nsec>=999500000L) {ts.tv_sec++;ts.tv_nsec=0;} break;
    case 6 : if (ts.tv_nsec>=999999500L) {ts.tv_sec++;ts.tv_nsec=0;} break;
    default: break; /* case 9: shown at full precision, no rounding */
  }
  switch (giTimeResol) {
    case 0 : printf("%jd "      ,(intmax_t)ts.tv_sec  ); break;
    case 3 : printf("%jd.%03ld ",(intmax_t)ts.tv_sec, ts.tv_nsec/1000000); break;
    case 6 : printf("%jd.%06ld ",(intmax_t)ts.tv_sec, ts.tv_nsec/   1000); break;
    case 9 : printf("%jd.%09ld ",(intmax_t)ts.tv_sec, ts.tv_nsec        ); break;
    default: error_exit(255,"read_e1st_1unit(): Unknown resolution\n");
  }
  if (giDeltaMode) {
    printf("0 ");
    gtsPrev.tv_sec=tsNow.tv_sec; gtsPrev.tv_nsec=tsNow.tv_nsec;
  }
  write_1char(szUnit, iLen);

  /*--- Look ahead by one character to decide the return value ------*/
  if (fetch_1char(fp,gszNextchar,&giNextlen)!=EOF) {giHold=1; return 0;}
  else                                             {          return 1;}
}


/*=== Read and write only one character (for I-option and 1st char) ===
 * [in]  giHold     : (must be defined as a global variable)
 *       gszNextchar: (must be defined as a global variable)
 *       giNextlen  : (must be defined as a global variable)
 * [ret] 0          : Finished reading/writing one character,
 *                    and more data may follow
 *       1          : Finished reading/writing one character,
 *                    which was also the last one of the file
 *       EOF        : There was no character to read at all          */
int read_I1st_1unit(FILE *fp) {

  /*--- Variables --------------------------------------------------*/
  tmsp       tsNow, ts;
  struct tm *ptm      ;
  char       szUnit[MB_LEN_MAX];
  int        iLen     ;
  char       szTmz[ 7]; /* timestamp (timezone) */

  /*--- Fetch the first character -----------------------------------*/
  if (fetch_1char(fp,szUnit,&iLen)==EOF) {return EOF;}
  if (clock_gettime(CLOCK_REALTIME,&tsNow) != 0) {
    error_exit(errno,"read_I1st_1unit(): clock_gettime(): %s\n",
                     strerror(errno)                            );
  }
  /* Same rounding carry as print_cur_timestamp(); see the comment in
   * read_c1st_1unit() for why this must be applied here, too.        */
  ts.tv_sec=tsNow.tv_sec; ts.tv_nsec=tsNow.tv_nsec;
  switch (giTimeResol) {
    case 0 : if (ts.tv_nsec>=500000000L) {ts.tv_sec++;ts.tv_nsec=0;} break;
    case 3 : if (ts.tv_nsec>=999500000L) {ts.tv_sec++;ts.tv_nsec=0;} break;
    case 6 : if (ts.tv_nsec>=999999500L) {ts.tv_sec++;ts.tv_nsec=0;} break;
    default: break; /* case 9: shown at full precision, no rounding */
  }
  if ((ptm=localtime(&ts.tv_sec)) == NULL) {
    error_exit(255,"read_I1st_1unit(): localtime(): returned NULL\n");
  }
  strftime(szTmz, 6, "%z", ptm);
  szTmz[6]=0; szTmz[5]=szTmz[4]; szTmz[4]=szTmz[3]; szTmz[3]=':';
  switch (giTimeResol) {
    case 0 : printf("%04d-%02d-%02dT%02d:%02d:%02d%s ",
               ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
               ptm->tm_hour     , ptm->tm_min  , ptm->tm_sec ,
                                                    szTmz     ); break;
    case 3 : printf("%04d-%02d-%02dT%02d:%02d:%02d,%03ld%s " ,
               ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
               ptm->tm_hour     , ptm->tm_min  , ptm->tm_sec ,
               ts.tv_nsec/1000000, szTmz                      ); break;
    case 6 : printf("%04d-%02d-%02dT%02d:%02d:%02d,%06ld%s " ,
               ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
               ptm->tm_hour     , ptm->tm_min  , ptm->tm_sec ,
               ts.tv_nsec/1000, szTmz                         ); break;
    case 9 : printf("%04d-%02d-%02dT%02d:%02d:%02d,%09ld%s ",
               ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
               ptm->tm_hour     , ptm->tm_min  , ptm->tm_sec ,
               ts.tv_nsec, szTmz                              ); break;
    default: error_exit(255,"read_I1st_1unit(): Unknown resolution\n");
  }
  if (giDeltaMode) {
    printf("0 ");
    gtsPrev.tv_sec=tsNow.tv_sec; gtsPrev.tv_nsec=tsNow.tv_nsec;
  }
  write_1char(szUnit, iLen);

  /*--- Look ahead by one character to decide the return value ------*/
  if (fetch_1char(fp,gszNextchar,&giNextlen)!=EOF) {giHold=1; return 0;}
  else                                             {          return 1;}
}


/*=== Read and write only one character (for Z-option and 1st char) ===
 * [in]  giHold     : (must be defined as a global variable)
 *       gszNextchar: (must be defined as a global variable)
 *       giNextlen  : (must be defined as a global variable)
 * [ret] 0          : Finished reading/writing one character,
 *                    and more data may follow
 *       1          : Finished reading/writing one character,
 *                    which was also the last one of the file
 *       EOF        : There was no character to read at all          */
int read_Z1st_1unit(FILE *fp) {

  /*--- Variables --------------------------------------------------*/
  char szUnit[MB_LEN_MAX];
  int  iLen;

  /*--- Fetch the first character -----------------------------------*/
  if (fetch_1char(fp,szUnit,&iLen)==EOF) {return EOF;}
  if (clock_gettime(CLOCK_REALTIME,&gtsZero) != 0) {
    error_exit(errno,"read_Z1st_1unit(): clock_gettime(): %s\n",
                     strerror(errno)                            );
  }
  if (giDeltaMode) {
    printf("0 0 ");
    gtsPrev.tv_sec=gtsZero.tv_sec; gtsPrev.tv_nsec=gtsZero.tv_nsec;
  } else           {
    printf("0 ");
  }
  write_1char(szUnit, iLen);

  /*--- Look ahead by one character to decide the return value ------*/
  if (fetch_1char(fp,gszNextchar,&giNextlen)!=EOF) {giHold=1; return 0;}
  else                                             {          return 1;}
}


/*=== Write the current timestamp to stdout ==========================
 * [in] giFmtType   : (must be defined as a global variable)
 *      giTimeResol : (must be defined as a global variable)
 *      giDeltaMode : (must be defined as a global variable)
 *      gtsZero     : (must be defined as a global variable)
 *      gtsPrev     : (must be defined as a global variable) */
void print_cur_timestamp(void) {

  /*--- Variables --------------------------------------------------*/
  tmsp        tsNow          ; /* Current time but substructed by gtsZero */
  tmsp        tsDiff         ;
  tmsp        ts             ;
  struct tm  *ptm            ;
  char        szBuf[LINE_BUF];
  char        szDec[21]      ; /* for the Decimal part */
  char        szTmz[ 7]      ; /* timestamp (timezone) */

  /*--- Get the current time ---------------------------------------*/
  if (clock_gettime(CLOCK_REALTIME,&tsNow) != 0) {
    error_exit(errno,"clock_gettime()#1: %s\n",strerror(errno));
  }

  /*--- Print the current timestamp --------------------------------*/
  switch (giFmtType) {
    case 'c':
              ts.tv_sec=tsNow.tv_sec; ts.tv_nsec=tsNow.tv_nsec;
              switch (giTimeResol) {
                case 0 : if (ts.tv_nsec>=500000000L) {ts.tv_sec++;ts.tv_nsec=0;}
                         szDec[0]=0;
                         break;
                case 3 : if (ts.tv_nsec>=999500000L) {ts.tv_sec++;ts.tv_nsec=0;}
                         snprintf(szDec,21,".%03ld",ts.tv_nsec/1000000);
                         break;
                case 6 : if (ts.tv_nsec>=999999500L) {ts.tv_sec++;ts.tv_nsec=0;}
                         snprintf(szDec,21,".%06ld",ts.tv_nsec/   1000);
                         break;
                default: snprintf(szDec,21,".%09ld",ts.tv_nsec        );
                         break;
              }
              ptm = localtime(&ts.tv_sec);
              if (ptm==NULL) {error_exit(255,"localtime(): returned NULL\n");}
              printf("%04d%02d%02d%02d%02d%02d%s ",
                ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
                ptm->tm_hour     , ptm->tm_min  , ptm->tm_sec , szDec);
              break;
    case 'I':
              ts.tv_sec=tsNow.tv_sec; ts.tv_nsec=tsNow.tv_nsec;
              switch (giTimeResol) {
                case 0 : if (ts.tv_nsec>=500000000L) {ts.tv_sec++;ts.tv_nsec=0;}
                         szDec[0]=0;
                         break;
                case 3 : if (ts.tv_nsec>=999500000L) {ts.tv_sec++;ts.tv_nsec=0;}
                         snprintf(szDec,21,",%03ld",ts.tv_nsec/1000000);
                         break;
                case 6 : if (ts.tv_nsec>=999999500L) {ts.tv_sec++;ts.tv_nsec=0;}
                         snprintf(szDec,21,",%06ld",ts.tv_nsec/   1000);
                         break;
                default: snprintf(szDec,21,",%09ld",ts.tv_nsec        );
                         break;
              }
              ptm = localtime(&ts.tv_sec);
              if (ptm==NULL) {error_exit(255,"localtime(): returned NULL\n");}
              strftime(szTmz, 6, "%z", ptm);
              szTmz[6]=0; szTmz[5]=szTmz[4]; szTmz[4]=szTmz[3]; szTmz[3]=':';
              printf("%04d-%02d-%02dT%02d:%02d:%02d%s%s ",
                ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
                ptm->tm_hour     , ptm->tm_min  , ptm->tm_sec , szDec, szTmz);
              break;
    case 'e':
              ts.tv_sec=tsNow.tv_sec; ts.tv_nsec=tsNow.tv_nsec;
              switch (giTimeResol) {
                case 0 : if (ts.tv_nsec>=500000000L) {ts.tv_sec++;ts.tv_nsec=0;}
                         szDec[0]=0;
                         break;
                case 3 : if (ts.tv_nsec>=999500000L) {ts.tv_sec++;ts.tv_nsec=0;}
                         snprintf(szDec,21,".%03ld",ts.tv_nsec/1000000);
                         break;
                case 6 : if (ts.tv_nsec>=999999500L) {ts.tv_sec++;ts.tv_nsec=0;}
                         snprintf(szDec,21,".%06ld",ts.tv_nsec/   1000);
                         break;
                default: snprintf(szDec,21,".%09ld",ts.tv_nsec        );
                         break;
              }
              ptm = localtime(&ts.tv_sec);
              strftime(szBuf, LINE_BUF, "%s", ptm);
              printf("%s%s ", szBuf, szDec);
              break;
    case 'z':
              if ((tsNow.tv_nsec - gtsZero.tv_nsec) < 0) {
                ts.tv_sec  = tsNow.tv_sec  - gtsZero.tv_sec  -          1;
                ts.tv_nsec = tsNow.tv_nsec - gtsZero.tv_nsec + 1000000000;
              } else {
                ts.tv_sec  = tsNow.tv_sec  - gtsZero.tv_sec ;
                ts.tv_nsec = tsNow.tv_nsec - gtsZero.tv_nsec;
              }
              switch (giTimeResol) {
                case 0 : if (ts.tv_nsec>=500000000L) {ts.tv_sec++;ts.tv_nsec=0;}
                         szDec[0]=0;
                         break;
                case 3 : if (ts.tv_nsec>=999500000L) {ts.tv_sec++;ts.tv_nsec=0;}
                         snprintf(szDec,21,".%03ld",ts.tv_nsec/1000000);
                         break;
                case 6 : if (ts.tv_nsec>=999999500L) {ts.tv_sec++;ts.tv_nsec=0;}
                         snprintf(szDec,21,".%06ld",ts.tv_nsec/   1000);
                         break;
                default: snprintf(szDec,21,".%09ld",ts.tv_nsec        );
                         break;
              }
              ptm = localtime(&ts.tv_sec);
              strftime(szBuf, LINE_BUF, "%s", ptm);
              printf("%s%s ", szBuf, szDec);
              break;
    default : error_exit(255,"print_cur_timestamp(): Unknown format\n");
  }

  /*--- Print the delta-t if required ------------------------------*/
  if (giDeltaMode) {
    if ((tsNow.tv_nsec - gtsPrev.tv_nsec) < 0) {
      tsDiff.tv_sec  = tsNow.tv_sec  - gtsPrev.tv_sec  -          1;
      tsDiff.tv_nsec = tsNow.tv_nsec - gtsPrev.tv_nsec + 1000000000;
    } else {
      tsDiff.tv_sec  = tsNow.tv_sec  - gtsPrev.tv_sec ;
      tsDiff.tv_nsec = tsNow.tv_nsec - gtsPrev.tv_nsec;
    }
    gtsPrev.tv_sec=tsNow.tv_sec; gtsPrev.tv_nsec=tsNow.tv_nsec;
    switch (giTimeResol) {
      case 0 : if(tsDiff.tv_nsec>=500000000L){tsDiff.tv_sec++;tsDiff.tv_nsec=0;}
               szDec[0]=0;
               break;
      case 3 : if(tsDiff.tv_nsec>=999500000L){tsDiff.tv_sec++;tsDiff.tv_nsec=0;}
               snprintf(szDec,21,".%03ld",tsDiff.tv_nsec/1000000);
               break;
      case 6 : if(tsDiff.tv_nsec>=999999500L){tsDiff.tv_sec++;tsDiff.tv_nsec=0;}
               snprintf(szDec,21,".%06ld",tsDiff.tv_nsec/   1000);
               break;
      default: snprintf(szDec,21,".%09ld",tsDiff.tv_nsec        );
               break;
    }
    ptm = localtime(&tsDiff.tv_sec);
    strftime(szBuf, LINE_BUF, "%s", ptm);
    printf("%s%s ", szBuf, szDec);
  }

  /*--- Finish -----------------------------------------------------*/
  return;
}
