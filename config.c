/*
     VeryNice -- a dynamic process re-nicer
     Copyright (C) 2000 Stephen D. Holland
 
     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; version 2 of the License.
 
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.
 
     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <pwd.h>
#include <syslog.h>

#include <sys/stat.h>
#include <unistd.h>

#include "linklist.h"
#include "stringstack.h"

#include "verynice.h"

  struct Stack *Stk;


  FILE *cfgin;
  char *filename;
  int userfile; /* is this global config, or owned by a user */
  uid_t uid; /* uid if owned by a user */

#define GET_INPUT ((PCB).input_code = fgetc(cfgin))
#define SYNTAX_ERROR syslog(LOG_WARNING,"Config parse error %s. file: %s, line %d, column %d\n",(PCB).error_message,filename,(PCB).line,(PCB).column)
#define PARSER_STACK_OVERFLOW syslog(LOG_WARNING,"Config parser stack overflow. file: %s, line %d, column %d\n",filename,(PCB).line,(PCB).column)
#define REDUCTION_TOKEN_ERROR syslog(LOG_WARNING,"Config reduction token error. file: %s, line %d, column %d\n",filename,(PCB).line,(PCB).column)



/*

 AnaGram Parsing Engine
 Copyright (c) 1993, 1996, Parsifal Software.
 All Rights Reserved.

*/



#ifndef CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define RULE_CONTEXT (&((PCB).cs[(PCB).ssx]))
#define ERROR_CONTEXT ((PCB).cs[(PCB).error_frame_ssx])
#define CONTEXT ((PCB).cs[(PCB).ssx])



config_pcb_type config_pcb;
#define PCB config_pcb

/*  Line 196, /home1/sdh4/verynice/config.syn */
void DoConfig(char *Filename,FILE *Cfgin, int Userfile, uid_t Uid)
  /* WARNING: side effects:
       - Assumes syslog(3) is activated. Caller is responsible for opening/closing
       - May open password database if not already open, and adjust location,
         but only if userfile==0
       - Caller is responsible for closing password database
  */
{
  
  Stk=InitStack();
  /* set globals accesible by parser */
  cfgin=Cfgin;
  filename=Filename;
  userfile=Userfile;
  uid=Uid;

  config(); /* run parser */

  UnitStack(Stk);
}

void ReadCfgFiles(char *prefix)
/* WARNING: side effects:
       - Assumes syslog(3) is activated. Caller is responsible for opening/closing
       - May open password database if not already open, and adjust location
       - This routine closes password database
  */
{
  char *centralname;
  struct immuneuid *u;
  struct immuneexe *i;
  struct badexe *b;
  struct goodexe *g;
  struct runawayexe *r;
  char *userfname;
  struct stat filestat;
  
  FILE *f;
  struct passwd *pwd;
  

  if (!strcmp(prefix,"/usr")) {
    centralname=malloc(1+3+1+strlen(GLOBALCFGFILE)+1);
    strcpy(centralname,"/etc/");
    strcat(centralname,GLOBALCFGFILE);
  } else {
    centralname=malloc(strlen(prefix)+1+3+1+strlen(GLOBALCFGFILE)+1);
    strcpy(centralname,prefix);
    strcat(centralname,"/etc/");
    strcat(centralname,GLOBALCFGFILE);
  }
  
  /* clear out current cfg database */
  while (u=(struct immuneuid *)RemHead(&iuidlist)) {
    free(u);
  }

  while (i=(struct immuneexe *)RemHead(&iexelist)) {
    free(i->exename);
    free(i);
  }

  while (b=(struct badexe *)RemHead(&bexelist)) {
    free(b->exename);
    free(b);
  }
 
  while (g=(struct goodexe *)RemHead(&gexelist)) {
    free(g->exename);
    free(g);
  }

  while (r=(struct runawayexe *)RemHead(&runawaylist)) {
    free(r->exename);
    free(r);
  }

  /* load central config */
  f=fopen(centralname,"r");
  if (f) {
    DoConfig(centralname,f,0,0);
    fclose(f);
  }
  free(centralname);

  /* load by-user config */
  setpwent();
  while (pwd=getpwent()) {
    /* read per-user info */
    userfname=malloc(strlen(pwd->pw_dir)+1+strlen(USERCFGFILE)+1);
    strcpy(userfname,pwd->pw_dir);
    strcat(userfname,"/");
    strcat(userfname,USERCFGFILE);
    if (!lstat(userfname,&filestat)) {
      if (!S_ISLNK(filestat.st_mode) && S_ISREG(filestat.st_mode)) {
	/* regular file that exists and is not a symbolic link */
	/* read it!!! */
	f=fopen(userfname,"r");
	DoConfig(userfname,f,1,pwd->pw_uid);
	fclose(f);
	

      }
    }
    
    free(userfname);
  }


  endpwent();
}



#ifndef CONVERT_CASE
#define CONVERT_CASE(c) (c)
#endif
#ifndef TAB_SPACING
#define TAB_SPACING 8
#endif

static void ag_rp_1(void) {
/* Line 76, /home1/sdh4/verynice/config.syn */
    char *str;
    struct immuneuid *u;
    struct passwd *pwd;

    str=PopString(Stk); /* get user name */
    if (!userfile) { /* allow this option only for central config */
      u=calloc(sizeof(struct immuneuid),1);
      pwd=getpwnam(str);
      u->uid=pwd->pw_uid;
      AddTail(&iuidlist,(struct Node *)u);
    }
  
}

static void ag_rp_2(void) {
/* Line 89, /home1/sdh4/verynice/config.syn */
    struct immuneexe *exe;
    char *str;
    
    str=PopString(Stk);
    exe=calloc(sizeof(struct immuneexe),1);
    exe->exename=strdup(str);
    exe->alluid=!userfile;
    exe->uid=uid;
    AddTail(&iexelist,(struct Node *)exe);
  
}

static void ag_rp_3(void) {
/* Line 100, /home1/sdh4/verynice/config.syn */
    struct badexe *exe;
    char *str;
    
    str=PopString(Stk);
    exe=calloc(sizeof(struct badexe),1);
    exe->exename=strdup(str);
    exe->alluid=!userfile;
    exe->uid=uid;
    AddTail(&bexelist,(struct Node *)exe);
    
    
  
}

static void ag_rp_4(void) {
/* Line 113, /home1/sdh4/verynice/config.syn */
    struct goodexe *exe;
    char *str;
    
    str=PopString(Stk);
    exe=calloc(sizeof(struct goodexe),1);
    exe->exename=strdup(str);
    exe->alluid=!userfile;
    exe->uid=uid;
    AddTail(&gexelist,(struct Node *)exe);
  
}

static void ag_rp_5(void) {
/* Line 124, /home1/sdh4/verynice/config.syn */
    struct runawayexe *exe;
    char *str;
    
    str=PopString(Stk);
    exe=calloc(sizeof(struct runawayexe),1);
    exe->exename=strdup(str);
    exe->alluid=!userfile;
    exe->uid=uid;
    AddTail(&runawaylist,(struct Node *)exe);
  
}

static void ag_rp_6(int i) {
/* Line 134, /home1/sdh4/verynice/config.syn */
if(!userfile) notnice=i;
}

static void ag_rp_7(int i) {
/* Line 135, /home1/sdh4/verynice/config.syn */
if (!userfile) batchjob=i;
}

static void ag_rp_8(int i) {
/* Line 136, /home1/sdh4/verynice/config.syn */
if (!userfile) runaway=i;
}

static void ag_rp_9(int i) {
/* Line 137, /home1/sdh4/verynice/config.syn */
if(!userfile) term=i;
}

static void ag_rp_10(double r) {
/* Line 138, /home1/sdh4/verynice/config.syn */
if (!userfile) badkarmarate=r;
}

static void ag_rp_11(double r) {
/* Line 139, /home1/sdh4/verynice/config.syn */
if (!userfile) karmarestorationrate=r;
}

static void ag_rp_12(int i) {
/* Line 140, /home1/sdh4/verynice/config.syn */
if (!userfile) periodicity=i;
}

static void ag_rp_13(int i) {
/* Line 141, /home1/sdh4/verynice/config.syn */
if (!userfile) rereadcfgperiodicity=i;
}

static void ag_rp_14(int c) {
/* Line 146, /home1/sdh4/verynice/config.syn */
InitString(Stk);AddChar(Stk,c);
}

static void ag_rp_15(int c) {
/* Line 147, /home1/sdh4/verynice/config.syn */
AddChar(Stk,c);
}

static void ag_rp_16(int c) {
/* Line 154, /home1/sdh4/verynice/config.syn */
InitString(Stk);AddChar(Stk,c);
}

static void ag_rp_17(char c) {
/* Line 155, /home1/sdh4/verynice/config.syn */
InitString(Stk);AddChar(Stk,c);
}

#define ag_rp_18(c) (AddChar(Stk,c))

#define ag_rp_19(c) (AddChar(Stk,c))

#define ag_rp_20() ('\\')

#define ag_rp_21() ('\"')

#define ag_rp_22() ('\n')

#define ag_rp_23() ('\r')

#define ag_rp_24(i) (i)

#define ag_rp_25(i) (i)

#define ag_rp_26(i) (-i)

#define ag_rp_27(d) ((int)(d-'0'))

#define ag_rp_28(i, d) ((i*10)+(int)(d-'0'))

#define ag_rp_29(m) (m)

#define ag_rp_30(m, e) (pow(10,e)*m)

#define ag_rp_31(i) (i)

#define ag_rp_32(i) (i)

#define ag_rp_33(i, f) (i+f)

#define ag_rp_34(f) (f)

#define ag_rp_35(d) ((double)(d-'0'))

#define ag_rp_36(i, d) ((i*10.0)+(double)(d-'0'))

#define ag_rp_37(d) (((double)(d-'0'))/10.0)

#define ag_rp_38(d, f) ((f+(double)(d-'0'))/10.0)


#define READ_COUNTS 
#define WRITE_COUNTS 
static config_vs_type ag_null_value;
#define V(i,t) (*(t *) (&(PCB).vs[(PCB).ssx + i]))
#define VS(i) (PCB).vs[(PCB).ssx + i]

#ifndef GET_CONTEXT
#define GET_CONTEXT CONTEXT = (PCB).input_context
#endif

typedef enum {
  ag_action_1,
  ag_action_2,
  ag_action_3,
  ag_action_4,
  ag_action_5,
  ag_action_6,
  ag_action_7,
  ag_action_8,
  ag_action_9,
  ag_action_10,
  ag_action_11,
  ag_action_12
} ag_parser_action;

static int ag_ap;



static const unsigned char ag_rpx[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,
    5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0, 16, 17, 18, 19, 20, 21,
   22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38
};

static const unsigned char ag_key_itt[] = {
 0
};

static const unsigned short ag_key_pt[] = {
0
};

static const unsigned char ag_key_ch[] = {
    0, 97,101,255,114,255, 97,255,109,255,114,255, 97,255,101,107,255,100,
  116,255, 97,255,101,117,255,101,255,110,255,117,255,109,255,109,255,101,
  255,121,255, 97,255,119,255, 97,255,110,255,101,117,255, 35, 98,103,105,
  110,112,114,116,255
};

static const unsigned char ag_key_act[] = {
  0,3,3,4,2,4,2,4,2,4,2,4,2,4,3,2,4,2,3,4,2,4,3,3,4,2,4,2,4,2,4,2,4,2,4,
  3,4,1,4,2,4,2,4,2,4,2,4,3,2,4,0,2,3,2,3,3,2,3,4
};

static const unsigned char ag_key_parm[] = {
    0, 25, 27,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 17,  0,  0,  0,
   22,  0,  0,  0, 15, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 19,
    0, 23,  0,  0,  0,  0,  0,  0,  0,  0,  0, 30,  0,  0,  9,  0, 18,  0,
   20, 28,  0, 24,  0
};

static const unsigned char ag_key_jmp[] = {
    0,  3,  6,  0,  1,  0,  4,  0,  6,  0,  8,  0, 10,  0,  0, 12,  0, 14,
   20,  0, 17,  0, 33, 36,  0, 22,  0, 25,  0, 27,  0, 29,  0, 31,  0, 77,
    0, 35,  0, 37,  0, 39,  0, 41,  0, 43,  0, 58, 45,  0,  0, 20, 26, 33,
   40, 47, 47, 80,  0
};

static const unsigned char ag_key_index[] = {
   50, 50, 50,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 50, 50,  0, 50,
   50, 50, 50,  0,  0, 50, 50, 50, 50, 50,  0, 50, 50, 50, 50, 50, 50, 50,
    0, 50, 50,  0,  0
};

static const unsigned char ag_key_ends[] = {
120,101,0, 116,101,0, 115,116,111,114,97,116,105,111,110,114,97,116,101,0, 
99,104,106,111,98,0, 111,111,100,101,120,101,0, 120,101,0, 
115,101,114,0, 111,116,110,105,99,101,0, 
101,114,105,111,100,105,99,105,116,121,0, 
114,101,97,100,99,102,103,112,101,114,105,111,100,105,99,105,116,121,0, 
120,101,0, 101,114,109,0, 
};
#define AG_TCV(x) (((int)(x) >= -1 && (int)(x) <= 255) ? ag_tcv[(x) + 1] : 0)

static const unsigned char ag_tcv[] = {
    3, 47, 47, 47, 47, 47, 47, 47, 47, 47,  4,  8, 47, 47,  8, 47, 47, 47,
   47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47,  4, 47, 32,
   47, 47, 47, 47, 47, 47, 47, 47, 39, 47, 40, 45, 47, 41, 41, 41, 41, 41,
   41, 41, 41, 41, 41, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 43, 47,
   47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
   47, 47, 47, 36, 47, 47, 47, 47, 47, 47, 47, 47, 43, 47, 47, 47, 47, 47,
   47, 47, 47, 37, 47, 47, 47, 38, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
   47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
   47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
   47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
   47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
   47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
   47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
   47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
   47, 47, 47, 47, 47
};

#ifndef SYNTAX_ERROR
#define SYNTAX_ERROR fprintf(stderr,"%s, line %d, column %d\n", \
  (PCB).error_message, (PCB).line, (PCB).column)
#endif

#ifndef FIRST_LINE
#define FIRST_LINE 1
#endif

#ifndef FIRST_COLUMN
#define FIRST_COLUMN 1
#endif


#ifndef PARSER_STACK_OVERFLOW
#define PARSER_STACK_OVERFLOW {fprintf(stderr, \
   "\nParser stack overflow, line %d, column %d\n",\
   (PCB).line, (PCB).column);}
#endif

#ifndef REDUCTION_TOKEN_ERROR
#define REDUCTION_TOKEN_ERROR {fprintf(stderr, \
    "\nReduction token error, line %d, column %d\n", \
    (PCB).line, (PCB).column);}
#endif


typedef enum
  {ag_accept_key, ag_set_key, ag_jmp_key, ag_end_key, ag_no_match_key,
   ag_cf_accept_key, ag_cf_set_key, ag_cf_end_key} key_words;

#ifndef GET_INPUT
#define GET_INPUT ((PCB).input_code = getchar())
#endif


static unsigned ag_look_ahead(void) {
  if ((PCB).rx < (PCB).fx) {
    return (unsigned) CONVERT_CASE((PCB).lab[(PCB).rx++]);
  }
  GET_INPUT;
  (PCB).fx++;
  return (unsigned) CONVERT_CASE((PCB).lab[(PCB).rx++] = (PCB).input_code);
}

static void ag_get_key_word(int ag_k) {
  int save_index = (PCB).rx;
  const  unsigned char *sp;
  unsigned ag_ch;
  while (1) {
    switch (ag_key_act[ag_k]) {
    case ag_cf_end_key:
      sp = ag_key_ends + ag_key_jmp[ag_k];
      do {
        if ((ag_ch = *sp++) == 0) {
          int ag_k1 = ag_key_parm[ag_k];
          int ag_k2 = ag_key_pt[ag_k1];
          if (ag_key_itt[ag_k2 + ag_look_ahead()]) goto ag_fail;
          (PCB).rx--;
          (PCB).token_number = (config_token_type) ag_key_pt[ag_k1 + 1];
          return;
        }
      } while (ag_look_ahead() == ag_ch);
      goto ag_fail;
    case ag_end_key:
      sp = ag_key_ends + ag_key_jmp[ag_k];
      do {
        if ((ag_ch = *sp++) == 0) {
          (PCB).token_number = (config_token_type) ag_key_parm[ag_k];
          return;
        }
      } while (ag_look_ahead() == ag_ch);
    case ag_no_match_key:
ag_fail:
      (PCB).rx = save_index;
      return;
    case ag_cf_set_key: {
      int ag_k1 = ag_key_parm[ag_k];
      int ag_k2 = ag_key_pt[ag_k1];
      ag_k = ag_key_jmp[ag_k];
      if (ag_key_itt[ag_k2 + (ag_ch = ag_look_ahead())]) break;
      save_index = --(PCB).rx;
      (PCB).token_number = (config_token_type) ag_key_pt[ag_k1+1];
      break;
    }
    case ag_set_key:
      save_index = (PCB).rx;
      (PCB).token_number = (config_token_type) ag_key_parm[ag_k];
    case ag_jmp_key:
      ag_k = ag_key_jmp[ag_k];
      ag_ch = ag_look_ahead();
      break;
    case ag_accept_key:
      (PCB).token_number =  (config_token_type) ag_key_parm[ag_k];
      return;
    case ag_cf_accept_key: {
      int ag_k1 = ag_key_parm[ag_k];
      int ag_k2 = ag_key_pt[ag_k1];
      if (ag_key_itt[ag_k2 + ag_look_ahead()]) (PCB).rx = save_index;
      else {
        (PCB).rx--;
        (PCB).token_number =  (config_token_type) ag_key_pt[ag_k1+1];
      }
      return;
    }
    }
    while(ag_key_ch[ag_k] < ag_ch) ag_k++;
    if (ag_key_ch[ag_k] != ag_ch) {
      (PCB).rx = save_index;
      return;
    }
  }
}


static void ag_track(void) {
  int ag_k = 0;
  while (ag_k < (PCB).rx) {
    int ag_ch = (PCB).lab[ag_k++];
    switch (ag_ch) {
    case '\n':
      (PCB).column = 1, (PCB).line++;
    case '\r':
    case '\f':
      break;
    case '\t':
      (PCB).column += (TAB_SPACING) - ((PCB).column - 1) % (TAB_SPACING);
      break;
    default:
      (PCB).column++;
    }
  }
  ag_k = 0;
  while ((PCB).rx < (PCB).fx) (PCB).lab[ag_k++] = (PCB).lab[(PCB).rx++];
  (PCB).fx = ag_k;
  (PCB).rx = 0;
}


static void ag_prot(void) {
  int ag_k = 32 - ++(PCB).btsx;
  if (ag_k <= (PCB).ssx) {
    (PCB).exit_flag = AG_STACK_ERROR_CODE;
    PARSER_STACK_OVERFLOW;
    return;
  }
  (PCB).bts[(PCB).btsx] = (PCB).sn;
  (PCB).bts[ag_k] = (PCB).ssx;
  (PCB).vs[ag_k] = (PCB).vs[(PCB).ssx];
  (PCB).ss[ag_k] = (PCB).ss[(PCB).ssx];
}

static void ag_undo(void) {
  if ((PCB).drt == -1) return;
  while ((PCB).btsx) {
    int ag_k = 32 - (PCB).btsx;
    (PCB).sn = (PCB).bts[(PCB).btsx--];
    (PCB).ssx = (PCB).bts[ag_k];
    (PCB).vs[(PCB).ssx] = (PCB).vs[ag_k];
    (PCB).ss[(PCB).ssx] = (PCB).ss[ag_k];
  }
  (PCB).token_number = (config_token_type) (PCB).drt;
  (PCB).ssx = (PCB).dssx;
  (PCB).sn = (PCB).dsn;
  (PCB).drt = -1;
}


static const unsigned char ag_tstt[] = {
4,0,1,2,5,6,
4,0,
30,28,27,25,24,23,22,20,19,18,17,15,13,9,8,3,0,7,
4,0,5,6,
4,0,5,6,
4,0,5,6,
4,0,5,6,
4,0,5,6,
4,0,5,6,
4,0,5,6,
4,0,5,6,
4,0,5,6,
4,0,5,6,
4,0,5,6,
4,0,5,6,
4,0,5,6,
47,45,43,41,40,39,38,37,36,32,4,0,11,12,
41,0,29,
41,0,29,
45,41,0,26,42,44,
45,41,0,26,42,44,
41,40,39,0,21,29,
41,40,39,0,21,29,
41,40,39,0,21,29,
41,40,39,0,21,29,
32,0,16,
32,0,16,
32,0,16,
32,0,16,
47,45,43,41,40,39,38,37,36,32,0,14,
47,45,43,41,40,39,38,37,36,32,4,0,
8,0,
41,4,0,5,6,
41,4,0,5,6,
41,0,46,
45,41,0,
43,0,
4,0,5,6,
4,0,5,6,
41,0,29,
41,0,29,
41,0,
4,0,5,6,
4,0,5,6,
4,0,5,6,
4,0,5,6,
47,45,43,41,40,39,38,37,36,4,0,33,35,
4,0,5,6,
4,0,5,6,
4,0,5,6,
4,0,5,6,
47,45,43,41,40,39,38,37,36,32,4,0,5,6,
41,0,46,
41,0,46,
41,40,39,0,21,29,
41,0,
41,0,
38,37,36,32,0,
47,45,43,41,40,39,38,37,36,32,4,0,35,

};


static unsigned const char ag_astt[299] = {
  1,8,0,1,1,1,9,5,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,3,7,3,1,8,1,1,1,8,1,1,1,8,1,
  1,1,8,1,1,1,8,1,1,1,8,1,1,1,8,1,1,1,8,1,1,1,8,1,1,1,8,1,1,1,8,1,1,1,8,1,1,
  1,8,1,1,1,1,1,1,1,1,1,1,1,1,1,8,1,1,2,7,1,2,7,1,1,2,7,1,1,1,1,2,7,1,1,1,2,
  1,1,7,1,1,2,1,1,7,1,1,2,1,1,7,1,1,2,1,1,7,1,1,1,7,1,1,7,1,1,7,1,1,7,1,2,2,
  2,2,2,2,2,2,2,2,7,1,9,9,9,9,9,9,9,9,9,9,9,5,3,7,10,1,5,1,2,10,1,5,1,2,1,7,
  2,1,10,4,1,4,1,5,1,2,1,5,1,2,2,7,1,2,7,1,10,4,1,5,1,2,1,5,1,2,1,5,1,2,1,5,
  1,2,2,2,2,2,2,2,2,2,1,2,7,1,2,1,5,1,2,1,5,1,2,1,5,1,2,1,5,1,2,10,10,10,10,
  10,10,10,10,10,10,1,5,1,2,1,4,2,1,4,2,2,1,1,7,2,1,10,4,10,4,2,2,2,2,7,10,
  10,10,10,10,10,10,10,1,3,10,7,2
};


static const unsigned char ag_pstt[] = {
1,2,0,2,1,2,
3,5,
3,4,5,6,7,8,9,10,11,12,13,14,15,16,7,1,2,7,
1,17,1,17,
1,18,1,18,
1,19,1,19,
1,20,1,20,
1,21,1,21,
1,22,1,22,
1,23,1,23,
1,24,1,24,
1,25,1,25,
1,26,1,26,
1,27,1,27,
1,28,1,28,
1,29,1,29,
30,30,30,30,30,30,30,30,30,30,30,31,30,31,
41,17,32,
41,18,33,
34,49,19,37,36,35,
34,49,20,38,36,35,
41,39,40,21,42,41,
41,39,40,22,43,41,
41,39,40,23,44,41,
41,39,40,24,45,41,
46,25,47,
46,26,48,
46,27,49,
46,28,50,
27,27,27,27,27,27,27,27,27,27,29,51,
10,10,10,10,10,10,10,10,10,10,10,12,
13,31,
42,1,4,1,26,
42,1,4,1,25,
52,34,48,
53,50,45,
54,43,
1,4,1,24,
1,4,1,23,
41,39,55,
41,40,56,
42,38,
1,4,1,22,
1,4,1,21,
1,4,1,20,
1,4,1,19,
30,30,30,30,30,30,30,30,57,30,46,58,31,
1,4,1,18,
1,4,1,17,
1,4,1,16,
1,4,1,15,
28,28,28,28,28,28,28,28,28,28,1,4,1,14,
52,51,52,
52,46,47,
41,39,40,54,44,41,
42,40,
42,39,
37,36,34,35,57,
32,32,32,32,32,32,32,32,57,29,32,58,33,

};


static const unsigned short ag_sbt[] = {
     0,   6,   8,  26,  30,  34,  38,  42,  46,  50,  54,  58,  62,  66,
    70,  74,  78,  92,  95,  98, 104, 110, 116, 122, 128, 134, 137, 140,
   143, 146, 158, 170, 172, 177, 182, 185, 188, 190, 194, 198, 201, 204,
   206, 210, 214, 218, 222, 235, 239, 243, 247, 251, 265, 268, 271, 277,
   279, 281, 286, 299
};


static const unsigned short ag_sbe[] = {
     1,   7,  24,  27,  31,  35,  39,  43,  47,  51,  55,  59,  63,  67,
    71,  75,  89,  93,  96, 100, 106, 113, 119, 125, 131, 135, 138, 141,
   144, 156, 169, 171, 174, 179, 183, 187, 189, 191, 195, 199, 202, 205,
   207, 211, 215, 219, 232, 236, 240, 244, 248, 262, 266, 269, 274, 278,
   280, 285, 297, 299
};


static const unsigned char ag_fl[] = {
  1,2,1,2,0,1,1,2,1,1,2,0,1,3,4,4,4,4,4,4,4,4,4,4,4,4,4,1,2,3,1,1,2,2,2,
  2,2,2,1,2,2,1,2,1,3,1,2,3,2,1,2,1,2
};

static const unsigned char ag_ptt[] = {
    0,  1,  5,  5,  6,  6,  2,  2,  7, 11, 11, 12, 12,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7, 14, 14, 16, 33, 33, 33, 33, 35, 35,
   35, 35, 21, 21, 21, 29, 29, 26, 26, 42, 42, 42, 42, 44, 44, 46, 46
};




static void ag_ra(void)
{
  switch(ag_rpx[ag_ap]) {
  case   1: ag_rp_1(); break;
  case   2: ag_rp_2(); break;
  case   3: ag_rp_3(); break;
  case   4: ag_rp_4(); break;
  case   5: ag_rp_5(); break;
  case   6: ag_rp_6(V(2,int)); break;
  case   7: ag_rp_7(V(2,int)); break;
  case   8: ag_rp_8(V(2,int)); break;
  case   9: ag_rp_9(V(2,int)); break;
  case  10: ag_rp_10(V(2,double)); break;
  case  11: ag_rp_11(V(2,double)); break;
  case  12: ag_rp_12(V(2,int)); break;
  case  13: ag_rp_13(V(2,int)); break;
  case  14: ag_rp_14(V(0,int)); break;
  case  15: ag_rp_15(V(1,int)); break;
  case  16: ag_rp_16(V(0,int)); break;
  case  17: ag_rp_17(V(0,char)); break;
  case  18: ag_rp_18(V(1,int)); break;
  case  19: ag_rp_19(V(1,char)); break;
  case  20: V(0,char) = ag_rp_20(); break;
  case  21: V(0,char) = ag_rp_21(); break;
  case  22: V(0,char) = ag_rp_22(); break;
  case  23: V(0,char) = ag_rp_23(); break;
  case  24: V(0,int) = ag_rp_24(V(0,int)); break;
  case  25: V(0,int) = ag_rp_25(V(1,int)); break;
  case  26: V(0,int) = ag_rp_26(V(1,int)); break;
  case  27: V(0,int) = ag_rp_27(V(0,int)); break;
  case  28: V(0,int) = ag_rp_28(V(0,int), V(1,int)); break;
  case  29: V(0,double) = ag_rp_29(V(0,double)); break;
  case  30: V(0,double) = ag_rp_30(V(0,double), V(2,int)); break;
  case  31: V(0,double) = ag_rp_31(V(0,double)); break;
  case  32: V(0,double) = ag_rp_32(V(0,double)); break;
  case  33: V(0,double) = ag_rp_33(V(0,double), V(2,double)); break;
  case  34: V(0,double) = ag_rp_34(V(1,double)); break;
  case  35: V(0,double) = ag_rp_35(V(0,int)); break;
  case  36: V(0,double) = ag_rp_36(V(0,double), V(1,int)); break;
  case  37: V(0,double) = ag_rp_37(V(0,int)); break;
  case  38: V(0,double) = ag_rp_38(V(0,int), V(1,double)); break;
  }
}

#define TOKEN_NAMES config_token_names
const char *const config_token_names[48] = {
  "ConfigFile",
  "ConfigFile",
  "lines",
  "EOF",
  "ws",
  "",
  "",
  "line",
  "EOL",
  "",
  "commentchars",
  "",
  "",
  "",
  "unquotedstring",
  "",
  "quotedstring",
  "",
  "",
  "",
  "",
  "signed integer",
  "",
  "",
  "",
  "",
  "unsigned real",
  "",
  "",
  "unsigned integer",
  "",
  "unquotedstringchar",
  "",
  "quotedstringchars",
  "RegularChar",
  "EscapeSequence",
  "",
  "",
  "",
  "",
  "",
  "DIGIT",
  "mantissa",
  "EXPSYM",
  "integer part",
  "",
  "fraction part",
  "",

};

static char ag_msg[82];

static void ag_diagnose(void) {
  int ag_snd = (PCB).sn, ag_k;

  ag_k = ag_sbt[ag_snd];
  if (*TOKEN_NAMES[ag_tstt[ag_k]] && ag_astt[ag_k + 1] == ag_action_8) {
    sprintf(ag_msg, "Missing %s", TOKEN_NAMES[ag_tstt[ag_k]]);
  }
  else if ((PCB).token_number && *TOKEN_NAMES[(PCB).token_number]) {
    sprintf(ag_msg, "Unexpected %s", TOKEN_NAMES[(PCB).token_number]);
  }
  else if (isprint((*(PCB).lab)) && (*(PCB).lab) != '\\') {
    sprintf(ag_msg, "Unexpected \'%c\'", (char) (*(PCB).lab));
  }
  else sprintf(ag_msg, "Unexpected input");
  (PCB).error_message = ag_msg;


}
static int ag_action_1_r_proc(void);
static int ag_action_2_r_proc(void);
static int ag_action_3_r_proc(void);
static int ag_action_4_r_proc(void);
static int ag_action_1_s_proc(void);
static int ag_action_3_s_proc(void);
static int ag_action_1_proc(void);
static int ag_action_2_proc(void);
static int ag_action_3_proc(void);
static int ag_action_4_proc(void);
static int ag_action_5_proc(void);
static int ag_action_6_proc(void);
static int ag_action_7_proc(void);
static int ag_action_8_proc(void);
static int ag_action_9_proc(void);
static int ag_action_10_proc(void);
static int ag_action_11_proc(void);
static int ag_action_8_proc(void);


static int (*const  ag_r_procs_scan[])(void) = {
  ag_action_1_r_proc,
  ag_action_2_r_proc,
  ag_action_3_r_proc,
  ag_action_4_r_proc
};

static int (*const  ag_s_procs_scan[])(void) = {
  ag_action_1_s_proc,
  ag_action_2_r_proc,
  ag_action_3_s_proc,
  ag_action_4_r_proc
};

static int (*const  ag_gt_procs_scan[])(void) = {
  ag_action_1_proc,
  ag_action_2_proc,
  ag_action_3_proc,
  ag_action_4_proc,
  ag_action_5_proc,
  ag_action_6_proc,
  ag_action_7_proc,
  ag_action_8_proc,
  ag_action_9_proc,
  ag_action_10_proc,
  ag_action_11_proc,
  ag_action_8_proc
};


static int ag_action_10_proc(void) {
  int ag_t = (PCB).token_number;
  (PCB).btsx = 0, (PCB).drt = -1;
  do {
    ag_track();
    if ((PCB).rx < (PCB).fx) {
      (PCB).input_code = (PCB).lab[(PCB).rx++];
      (PCB).token_number = (config_token_type) AG_TCV((PCB).input_code);}
    else {
      GET_INPUT;
      (PCB).lab[(PCB).fx++] = (PCB).input_code;
      (PCB).token_number = (config_token_type) AG_TCV((PCB).input_code);
      (PCB).rx++;
    }
    if (ag_key_index[(PCB).sn]) {
      unsigned ag_k = ag_key_index[(PCB).sn];
      unsigned char ag_ch = (unsigned char) CONVERT_CASE((PCB).input_code);
      while (ag_key_ch[ag_k] < ag_ch) ag_k++;
      if (ag_key_ch[ag_k] == ag_ch) ag_get_key_word(ag_k);
    }
  } while ((PCB).token_number == (config_token_type) ag_t);
  (PCB).rx = 0;
  return 1;
}

static int ag_action_11_proc(void) {
  int ag_t = (PCB).token_number;

  (PCB).btsx = 0, (PCB).drt = -1;
  do {
    (*(int *) &(PCB).vs[(PCB).ssx]) = *(PCB).lab;
    (PCB).ssx--;
    ag_track();
    ag_ra();
    if ((PCB).exit_flag != AG_RUNNING_CODE) return 0;
    (PCB).ssx++;
    if ((PCB).rx < (PCB).fx) {
      (PCB).input_code = (PCB).lab[(PCB).rx++];
      (PCB).token_number = (config_token_type) AG_TCV((PCB).input_code);}
    else {
      GET_INPUT;
      (PCB).lab[(PCB).fx++] = (PCB).input_code;
      (PCB).token_number = (config_token_type) AG_TCV((PCB).input_code);
      (PCB).rx++;
    }
    if (ag_key_index[(PCB).sn]) {
      unsigned ag_k = ag_key_index[(PCB).sn];
      unsigned char ag_ch = (unsigned char) CONVERT_CASE((PCB).input_code);
      while (ag_key_ch[ag_k] < ag_ch) ag_k++;
      if (ag_key_ch[ag_k] == ag_ch) ag_get_key_word(ag_k);
    }
  }
  while ((PCB).token_number == (config_token_type) ag_t);
  (PCB).rx = 0;
  return 1;
}

static int ag_action_3_r_proc(void) {
  int ag_sd = ag_fl[ag_ap] - 1;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  (PCB).btsx = 0, (PCB).drt = -1;
  (PCB).reduction_token = (config_token_type) ag_ptt[ag_ap];
  ag_ra();
  return (PCB).exit_flag == AG_RUNNING_CODE;
}

static int ag_action_3_s_proc(void) {
  int ag_sd = ag_fl[ag_ap] - 1;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  (PCB).btsx = 0, (PCB).drt = -1;
  (PCB).reduction_token = (config_token_type) ag_ptt[ag_ap];
  ag_ra();
  return (PCB).exit_flag == AG_RUNNING_CODE;;
}

static int ag_action_4_r_proc(void) {
  int ag_sd = ag_fl[ag_ap] - 1;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  (PCB).reduction_token = (config_token_type) ag_ptt[ag_ap];
  return 1;
}

static int ag_action_2_proc(void) {
  (PCB).btsx = 0, (PCB).drt = -1;
  if ((PCB).ssx >= 32) {
    (PCB).exit_flag = AG_STACK_ERROR_CODE;
    PARSER_STACK_OVERFLOW;
  }
  (*(int *) &(PCB).vs[(PCB).ssx]) = *(PCB).lab;
  (PCB).ss[(PCB).ssx] = (PCB).sn;
  (PCB).ssx++;
  (PCB).sn = ag_ap;
  ag_track();
  return 0;
}

static int ag_action_9_proc(void) {
  if((PCB).drt == -1) {
    (PCB).drt=(PCB).token_number;
    (PCB).dssx=(PCB).ssx;
    (PCB).dsn=(PCB).sn;
  }
  ag_prot();
  (PCB).vs[(PCB).ssx] = ag_null_value;
  (PCB).ss[(PCB).ssx] = (PCB).sn;
  (PCB).ssx++;
  (PCB).sn = ag_ap;
  (PCB).rx = 0;
  return (PCB).exit_flag == AG_RUNNING_CODE;
}

static int ag_action_2_r_proc(void) {
  (PCB).ssx++;
  (PCB).sn = ag_ap;
  return 0;
}

static int ag_action_7_proc(void) {
  --(PCB).ssx;
  (PCB).rx = 0;
  (PCB).exit_flag = AG_SUCCESS_CODE;
  return 0;
}

static int ag_action_1_proc(void) {
  ag_track();
  (PCB).exit_flag = AG_SUCCESS_CODE;
  return 0;
}

static int ag_action_1_r_proc(void) {
  (PCB).exit_flag = AG_SUCCESS_CODE;
  return 0;
}

static int ag_action_1_s_proc(void) {
  (PCB).exit_flag = AG_SUCCESS_CODE;
  return 0;
}

static int ag_action_4_proc(void) {
  int ag_sd = ag_fl[ag_ap] - 1;
  (PCB).reduction_token = (config_token_type) ag_ptt[ag_ap];
  (PCB).btsx = 0, (PCB).drt = -1;
  (*(int *) &(PCB).vs[(PCB).ssx]) = *(PCB).lab;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  else (PCB).ss[(PCB).ssx] = (PCB).sn;
  ag_track();
  while ((PCB).exit_flag == AG_RUNNING_CODE) {
    unsigned ag_t1 = ag_sbe[(PCB).sn] + 1;
    unsigned ag_t2 = ag_sbt[(PCB).sn+1] - 1;
    do {
      unsigned ag_tx = (ag_t1 + ag_t2)/2;
      if (ag_tstt[ag_tx] < (const unsigned char)(PCB).reduction_token) ag_t1 = ag_tx + 1;
      else ag_t2 = ag_tx;
    } while (ag_t1 < ag_t2);
    ag_ap = ag_pstt[ag_t1];
    if ((ag_s_procs_scan[ag_astt[ag_t1]])() == 0) break;
  }
  return 0;
}

static int ag_action_3_proc(void) {
  int ag_sd = ag_fl[ag_ap] - 1;
  (PCB).btsx = 0, (PCB).drt = -1;
  (*(int *) &(PCB).vs[(PCB).ssx]) = *(PCB).lab;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  else (PCB).ss[(PCB).ssx] = (PCB).sn;
  ag_track();
  (PCB).reduction_token = (config_token_type) ag_ptt[ag_ap];
  ag_ra();
  while ((PCB).exit_flag == AG_RUNNING_CODE) {
    unsigned ag_t1 = ag_sbe[(PCB).sn] + 1;
    unsigned ag_t2 = ag_sbt[(PCB).sn+1] - 1;
    do {
      unsigned ag_tx = (ag_t1 + ag_t2)/2;
      if (ag_tstt[ag_tx] < (const unsigned char)(PCB).reduction_token) ag_t1 = ag_tx + 1;
      else ag_t2 = ag_tx;
    } while (ag_t1 < ag_t2);
    ag_ap = ag_pstt[ag_t1];
    if ((ag_s_procs_scan[ag_astt[ag_t1]])() == 0) break;
  }
  return 0;
}

static int ag_action_8_proc(void) {
  ag_undo();
  (PCB).rx = 0;
  (PCB).exit_flag = AG_SYNTAX_ERROR_CODE;
  ag_diagnose();
  SYNTAX_ERROR;
  {(PCB).rx = 1; ag_track();}
  return (PCB).exit_flag == AG_RUNNING_CODE;
}

static int ag_action_5_proc(void) {
  int ag_sd = ag_fl[ag_ap];
  if((PCB).drt == -1) {
    (PCB).drt=(PCB).token_number;
    (PCB).dssx=(PCB).ssx;
    (PCB).dsn=(PCB).sn;
  }
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  else {
    ag_prot();
    (PCB).ss[(PCB).ssx] = (PCB).sn;
  }
  (PCB).rx = 0;
  (PCB).reduction_token = (config_token_type) ag_ptt[ag_ap];
  ag_ra();
  while ((PCB).exit_flag == AG_RUNNING_CODE) {
    unsigned ag_t1 = ag_sbe[(PCB).sn] + 1;
    unsigned ag_t2 = ag_sbt[(PCB).sn+1] - 1;
    do {
      unsigned ag_tx = (ag_t1 + ag_t2)/2;
      if (ag_tstt[ag_tx] < (const unsigned char)(PCB).reduction_token) ag_t1 = ag_tx + 1;
      else ag_t2 = ag_tx;
    } while (ag_t1 < ag_t2);
    ag_ap = ag_pstt[ag_t1];
    if ((ag_r_procs_scan[ag_astt[ag_t1]])() == 0) break;
  }
  return (PCB).exit_flag == AG_RUNNING_CODE;
}

static int ag_action_6_proc(void) {
  int ag_sd = ag_fl[ag_ap];
  (PCB).reduction_token = (config_token_type) ag_ptt[ag_ap];
  if((PCB).drt == -1) {
    (PCB).drt=(PCB).token_number;
    (PCB).dssx=(PCB).ssx;
    (PCB).dsn=(PCB).sn;
  }
  if (ag_sd) {
    (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  }
  else {
    ag_prot();
    (PCB).vs[(PCB).ssx] = ag_null_value;
    (PCB).ss[(PCB).ssx] = (PCB).sn;
  }
  (PCB).rx = 0;
  while ((PCB).exit_flag == AG_RUNNING_CODE) {
    unsigned ag_t1 = ag_sbe[(PCB).sn] + 1;
    unsigned ag_t2 = ag_sbt[(PCB).sn+1] - 1;
    do {
      unsigned ag_tx = (ag_t1 + ag_t2)/2;
      if (ag_tstt[ag_tx] < (const unsigned char)(PCB).reduction_token) ag_t1 = ag_tx + 1;
      else ag_t2 = ag_tx;
    } while (ag_t1 < ag_t2);
    ag_ap = ag_pstt[ag_t1];
    if ((ag_r_procs_scan[ag_astt[ag_t1]])() == 0) break;
  }
  return (PCB).exit_flag == AG_RUNNING_CODE;
}


void init_config(void) {
  (PCB).rx = (PCB).fx = 0;
  (PCB).ss[0] = (PCB).sn = (PCB).ssx = 0;
  (PCB).exit_flag = AG_RUNNING_CODE;
  (PCB).line = FIRST_LINE;
   (PCB).column = FIRST_COLUMN;
  (PCB).btsx = 0, (PCB).drt = -1;
}

void config(void) {
  init_config();
  (PCB).exit_flag = AG_RUNNING_CODE;
  while ((PCB).exit_flag == AG_RUNNING_CODE) {
    unsigned ag_t1 = ag_sbt[(PCB).sn];
    if (ag_tstt[ag_t1]) {
      unsigned ag_t2 = ag_sbe[(PCB).sn] - 1;
      if ((PCB).rx < (PCB).fx) {
        (PCB).input_code = (PCB).lab[(PCB).rx++];
        (PCB).token_number = (config_token_type) AG_TCV((PCB).input_code);}
      else {
        GET_INPUT;
        (PCB).lab[(PCB).fx++] = (PCB).input_code;
        (PCB).token_number = (config_token_type) AG_TCV((PCB).input_code);
        (PCB).rx++;
      }
      if (ag_key_index[(PCB).sn]) {
        unsigned ag_k = ag_key_index[(PCB).sn];
        unsigned char ag_ch = (unsigned char) CONVERT_CASE((PCB).input_code);
        while (ag_key_ch[ag_k] < ag_ch) ag_k++;
        if (ag_key_ch[ag_k] == ag_ch) ag_get_key_word(ag_k);
      }
      do {
        unsigned ag_tx = (ag_t1 + ag_t2)/2;
        if (ag_tstt[ag_tx] > (const unsigned char)(PCB).token_number)
          ag_t1 = ag_tx + 1;
        else ag_t2 = ag_tx;
      } while (ag_t1 < ag_t2);
      if (ag_tstt[ag_t1] != (const unsigned char)(PCB).token_number)
        ag_t1 = ag_sbe[(PCB).sn];
    }
    ag_ap = ag_pstt[ag_t1];
    (ag_gt_procs_scan[ag_astt[ag_t1]])();
  }
}


