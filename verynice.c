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

/* verynice */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>
#include <pwd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include "linklist.h"

#include "verynice.h"

#define min_mac(a,b) (((a) < (b)) ? (a) : (b))

/*#define PREFIX "/usr/local"*/  /* PREFIX should be defined on compiler
  command line */

/* NICE levels */

#define NORMAL 0  /* DON'T change this one */
#define NOTNICE -4 /* intended for multimedia apps, etc. */
#define BATCHJOB 18 
#define RUNAWAY 20 /* SIGKILL level */
#define TERM 22 /* SIGTERM level */

int normal=NORMAL;
int notnice=NOTNICE;
int batchjob=BATCHJOB;
int runaway=RUNAWAY;
int term=TERM;

int numprocessors=1; /* FIXME: Should autodetect (grep processor /proc/cpuinfo |wc -l */


#define PERIODICITY 60 /* period of checking processes, in seconds */

//#define PERIODICITY 5

#define REREADCFGPERIODICITY 60 /* period of rereading configs (including 
				    in user's directories) in terms of
				    periodicity, above */ 

int periodicity=PERIODICITY;
int rereadcfgperiodicity=REREADCFGPERIODICITY;


/* badkarma rate -- 1 unit of badkarma is 1 nice level, up to limit */
#define BADKARMARATE (1.0/60.0)  /* amount of bad karma generated per second
				  of 100% equiv. cpu usage */
     //#define BADKARMARATE (1.0/5.0/2.0)
#define KARMARESTORATIONRATE (1.0/60.0) /* amount of karma restored per 
					  second of 0% equiv. cpu usage */

double badkarmarate=BADKARMARATE;
double karmarestorationrate=KARMARESTORATIONRATE;


struct List proclist;
struct List newproclist;

struct List iuidlist;

struct List iexelist;

struct List bexelist;

struct List gexelist;


struct List runawaylist;


volatile int killflag=0;
volatile int hupflag=0;
volatile int dumpflag=0;
volatile int reconfigflag=0;

void killhandler(int signum)
{
  killflag=1;
}

void huphandler(int signum)
{
  hupflag=1;
#ifdef TARGET_solaris /* on Solaris, it seems you must reassert the signal handler */
  signal(signum,huphandler);
#endif
}

void dumphandler(int signum)
{
  dumpflag=1;
#ifdef TARGET_solaris /* on Solaris, it seems you must reassert the signal handler */
  signal(signum,dumphandler);
#endif
}

void reconfighandler(int signum)
{
  reconfigflag=1;
#ifdef TARGET_solaris /* on Solaris, it seems you must reassert the signal handler */
  signal(signum,reconfighandler);
#endif
}

double timediff(struct timeval *starttime,struct timeval *endtime)
{
  int deltasec;
  int deltausec;
  
  deltasec=endtime->tv_sec-starttime->tv_sec;
  deltausec=endtime->tv_usec-starttime->tv_usec;

  if (deltausec < 0) {
    deltausec+=1000000;
    deltasec--;
  }

  return ((double)deltasec)+1.e-6*deltausec;

}

struct procent *CreateProc(void)
{
  struct procent *proc;
  proc=calloc(sizeof(struct procent),1);
  proc->cpuusage=0.0;
  proc->current_cpuusage=0.0;
  proc->badkarma=0.0;
  proc->exename=NULL;
  return proc;
}

void DeleteProc(struct procent *proc)
{
  free(proc->exename);
  free(proc);
}

void procinsert(struct List *list,struct procent *proc)
     /* insert into sorted list */
{
  struct procent *node;
  
  for (node=(struct procent *)list->lh_Head;node;node=(struct procent *)node->Node.ln_Succ) {
    if (node->Node.ln_Succ && node->pid > proc->pid) {
      Insert(list,(struct Node *)proc,node->Node.ln_Pred);
      break;
    }
    if (!node->Node.ln_Succ) {
      /* end of list and we still haven't added it! */
      AddTail(list,(struct Node *)proc);
    }
  }
}

struct procent *procfindbypid(struct List *plist,pid_t pid)
{
  struct procent *proc;

  for (proc=(struct procent *)plist->lh_Head;proc->Node.ln_Succ;proc=(struct procent *)proc->Node.ln_Succ) {
    if (proc->pid==pid)
      return proc;
  }
  return NULL;
}

#ifdef TARGET_linux

#include <asm/param.h> /* get HZ */

void ReadProcs(struct List *Target)
{
  DIR *dir;
  struct dirent *ent;
  int Pos;
  uid_t uid;
  pid_t pid,parentpid;
  struct procent *proc;
  FILE *statf;
  char *statfname;
  char *exelname;
  char exelbuf[2000];
  int exellen;
  static char statfbuf[2000];
  char *bufptr; /* points into statfbuf */
  int actsize;
  int lastcloseparenidx;
  unsigned long utime;
  unsigned long stime;
  int niceval;
  char *exename;
  struct stat statbuf;

  dir=opendir("/proc");
  
  while (ent=readdir(dir)) {
    for (Pos=0;ent->d_name[Pos];Pos++) {
      if (!isdigit(ent->d_name[Pos]))
	break;      
    }

    if (!ent->d_name[Pos]) {
      /* completely numeric file name -- this is a pid */
      pid=(pid_t)strtoul(ent->d_name,NULL,10);
      
      /* read stat file */
      statfname=malloc(6+strlen(ent->d_name)+1+4+1);
      strcpy(statfname,"/proc/");
      strcat(statfname,ent->d_name);
      strcat(statfname,"/stat"); 
      statf=fopen(statfname,"r");
      if (statf) {
	actsize=fread((char *)statfbuf,1,sizeof(statfbuf)-1,statf); 
	statfbuf[actsize]=0; /* add null terminator */
	fclose(statf);
	/* The process name field in the linux procfs stat file is not escaped,
	   so the only guaranteed way to find the end of it is to find the 
	   last close parenthesis in the file */
	for (lastcloseparenidx=actsize-1;lastcloseparenidx >= 0;lastcloseparenidx--) {
	  if (statfbuf[lastcloseparenidx]==')')
	    break;
	}
	if (lastcloseparenidx != 0) {
	  /* going ok... */
	  
	  /* extract data */
	  
	  /* it's all integer numeric starting at lastcloseparenidx+4, 
	     so we can use strtoul,etc. */
	  
	  /* use strtod for throwaways, so we can handle large and negative
	     numbers */
	  bufptr=&statfbuf[lastcloseparenidx+4];
	  
	  parentpid=(pid_t)strtol(bufptr,&bufptr,10);
	  for (Pos=0;Pos < 9; Pos ++) {
	    strtod(bufptr,&bufptr); /* throw away next 9 */
	  }
	  utime=strtoul(bufptr,&bufptr,10);
	  stime=strtoul(bufptr,&bufptr,10);
	  
	  for (Pos=0;Pos < 3; Pos ++) {
	    strtod(bufptr,&bufptr); /* throw away cutime and cstime, plus priority for now */
	  }
	  niceval=strtol(bufptr,&bufptr,10);
	  
	  /* read executable symbolic link */
	  exelname=malloc(6+strlen(ent->d_name)+1+3+1);
	  strcpy(exelname,"/proc/");
	  strcat(exelname,ent->d_name);
	  strcat(exelname,"/exe");
	  if (!lstat(exelname,&statbuf)) {
	    uid=statbuf.st_uid;
	    exellen=readlink(exelname,exelbuf,sizeof(exelbuf)-1);
	    if (exellen > 0) {
	      exelbuf[exellen]='\0';
	      
	      /* it all worked -- create data structure */
	      /* create entry */
	      proc=CreateProc();
	      proc->pid=pid;
	      proc->nicelevel=niceval;
	      proc->reniced=0;
	      proc->parentpid=parentpid;
	      proc->cpuusage=(((double)utime)+ ((double)stime))/HZ;
	      proc->current_cpuusage=0.0;
	      proc->badkarma=0.0;
	      proc->uid=uid;
	      proc->immuneflag=0;
	      proc->goodflag=0;
	      proc->badflag=0;
	      proc->potentialrunaway=0;
	      proc->exename=strdup(exelbuf);
	      
	      procinsert(Target,proc);
	    }
	  }
	  
	  free(exelname);
	  
	}
      }
      
      free(statfname);
      
    }
    
  }
  closedir(dir);
}

#endif /* TARGET_linux */


#ifdef TARGET_solaris

#include <procfs.h> /* defines Solaris psinfo_t */

void ReadProcs(struct List *Target)
{
  DIR *dir;
  struct dirent *ent;
  pid_t pid;
  psinfo_t procinfo;
  int nbytes;
  struct procent *proc;
  char *statfname;
  int statf;
  int Pos;


  dir=opendir("/proc");
  while (ent=readdir(dir)) {
    for (Pos=0;ent->d_name[Pos];Pos++) {
      if (!isdigit(ent->d_name[Pos]))
	break;      
    }
    if (!ent->d_name[Pos]) {
      /* completely numeric file name -- this is a pid */
      pid=(pid_t)strtoul(ent->d_name,NULL,10);
      
      /* read stat file */
      statfname=malloc(6+strlen(ent->d_name)+1+6+1);
      strcpy(statfname,"/proc/");
      strcat(statfname,ent->d_name);
      strcat(statfname,"/psinfo"); 
      statf=open(statfname,O_RDONLY);
      if (statf >= 0) {
	nbytes=read(statf,&procinfo,sizeof(procinfo));
	if (nbytes==sizeof(procinfo)) {
	  proc=CreateProc();
	  proc->pid=procinfo.pr_pid;
	  proc->nicelevel=procinfo.pr_lwp.pr_nice;
	  proc->reniced=0;
	  proc->parentpid=procinfo.pr_ppid;
	  proc->cpuusage=procinfo.pr_time.tv_sec + ((double)procinfo.pr_time.tv_nsec)/1.e9;
	  proc->current_cpuusage=0.0;
	  proc->badkarma=0.0;
	  proc->uid=procinfo.pr_uid;
	  proc->immuneflag=0;
	  proc->goodflag=0;
	  proc->badflag=0;
	  proc->potentialrunaway=0;
	  proc->exename=strdup(procinfo.pr_fname);
	  procinsert(Target,proc);
	}
	close(statf);
      }
      free(statfname);

    }
  }
  closedir(dir);
}

#endif /* TARGET_solaris */

void updatecpuusage(void)
{
  /* update cpuusage in proclist from newproclist, but do nothing else */
  struct procent *proc,*nproc;
  
  for (proc=(struct procent *)proclist.lh_Head,
	 nproc=(struct procent *)newproclist.lh_Head;
       proc->Node.ln_Succ || nproc->Node.ln_Succ;) {
    
    /* look at both processes and compare pids */
    if ( (!nproc->Node.ln_Succ || (proc->Node.ln_Succ && (nproc->pid > proc->pid)))) {
      /* nproc is missing -- process dead -- do nothing */ 
      /* increment proc */
      proc=(struct procent *)proc->Node.ln_Succ;
      
    } else if ( (!proc->Node.ln_Succ || (nproc->Node.ln_Succ && (nproc->pid < proc->pid)))) {
      /* proc is missing -- process new -- do nothing */ 
      /* increment nproc */
      nproc=(struct procent *)nproc->Node.ln_Succ;
      
    } else {
      /* process id's match -- copy cpu usage */
      assert(nproc->pid == proc->pid);
      proc->cpuusage=nproc->cpuusage;
      /* increment proc and nproc */
      proc=(struct procent *)proc->Node.ln_Succ;
      nproc=(struct procent *)nproc->Node.ln_Succ;
      
    }


  }

  /* clear out newproclist */

  while (nproc=(struct procent *)RemHead(&newproclist)) {
    DeleteProc(nproc);
  }
  
  
  
}

int MatchString(char *str,char *substr)
{
  int LenDiff;
  int Cnt;

  LenDiff=strlen(str)-strlen(substr);
  
  for (Cnt=0;Cnt < LenDiff+1;Cnt++) {
    if (!strncmp(str+Cnt,substr,strlen(substr))) {
      return 1;
    }
  }
  return 0;
}

int MatchExe(char *exe,char *pattern)
{
  if (pattern[0]=='/') {
    return !strcmp(exe,pattern);
  } else return(MatchString(exe,pattern));
}

void SetProcFlags(struct procent *proc)
{
  struct badexe *b;
  struct immuneuid *iuid;
  struct immuneexe *i;
  struct runawayexe *r;
  struct goodexe *g;


  /* If root sets a "goodexe" priority, it will even apply to immune uid's.
     This is so you can make root immune, but force setting X to the "goodexe"
     priority (X is basically a multimedia app and should be scheduled as such)
  */

  for (g=(struct goodexe *)gexelist.lh_Head;g->Node.ln_Succ;g=(struct goodexe *)g->Node.ln_Succ) {
    if (g->alluid) {
      if (MatchExe(proc->exename,g->exename)) {
	proc->badflag=0;
	proc->goodflag=1;
	proc->immuneflag=0;
	proc->potentialrunaway=0;
	return;
      }
    }
    
  }

  /* cancel out immune UID's */
  for (iuid=(struct immuneuid *)iuidlist.lh_Head;iuid->Node.ln_Succ;iuid=(struct immuneuid *)iuid->Node.ln_Succ) {
    if (iuid->uid==proc->uid) {
      proc->immuneflag=1;
      proc->goodflag=0;
      proc->badflag=0;
      proc->potentialrunaway=0;
      return;
    }
  }

  for (b=(struct badexe *)bexelist.lh_Head;b->Node.ln_Succ;b=(struct badexe *)b->Node.ln_Succ) {
    if (b->alluid || b->uid==proc->uid) {
      if (MatchExe(proc->exename,b->exename)) {
	proc->badflag=1;
	proc->goodflag=0;
	proc->immuneflag=0;
	break;
      }
    }
  }


  for (r=(struct runawayexe *)runawaylist.lh_Head;r->Node.ln_Succ;r=(struct runawayexe *)r->Node.ln_Succ) {
    if (r->alluid || r->uid==proc->uid) {
      if (MatchExe(proc->exename,r->exename)) {
	proc->goodflag=0;
	proc->immuneflag=0;
	proc->potentialrunaway=1;
	return;
      } 
    }
    
  }


  for (i=(struct immuneexe *)iexelist.lh_Head;i->Node.ln_Succ;i=(struct immuneexe *)i->Node.ln_Succ) {
    if (i->alluid || i->uid==proc->uid) {
      if (MatchExe(proc->exename,i->exename)) {
	proc->badflag=0;
	proc->goodflag=0;
	proc->immuneflag=1;
	proc->potentialrunaway=0;
	return;
      }
    }
    
  }

  for (g=(struct goodexe *)gexelist.lh_Head;g->Node.ln_Succ;g=(struct goodexe *)g->Node.ln_Succ) {
    if (g->alluid || g->uid==proc->uid) {
      if (MatchExe(proc->exename,g->exename)) {
	proc->badflag=0;
	proc->goodflag=1;
	proc->immuneflag=0;
	proc->potentialrunaway=0;
	return;
      }
    }
    
  }




}

void MergeProcs(void)
{

  struct procent *proc,*nproc,*next;
  int didsomething;
  

  for (proc=(struct procent *)proclist.lh_Head,
	 nproc=(struct procent *)newproclist.lh_Head;
       proc->Node.ln_Succ || nproc->Node.ln_Succ;) {
    
    /* look at both processes and compare pids */
    if ( (!nproc->Node.ln_Succ || (proc->Node.ln_Succ && (nproc->pid > proc->pid)))) {
      /* nproc is missing -- process dead -- remove process */ 
      //fprintf(stderr,"dead process nproc=%ld proc=%ld\n",(long)nproc->pid,(long)proc->pid);
      /* increment proc */
      proc->deadflag=1;
      proc=(struct procent *)proc->Node.ln_Succ;
	
      
    } else if ( (!proc->Node.ln_Succ || (nproc->Node.ln_Succ && (nproc->pid < proc->pid)))) {
      /* proc is missing -- process new -- do nothing. We will handle this later */ 
      //fprintf(stderr,"new process\n");
      nproc->reallynew=1;
      nproc->current_cpuusage=nproc->cpuusage; /* previous cpu usage was zero */
      /* increment nproc */
      nproc=(struct procent *)nproc->Node.ln_Succ;
      
      
    } else {
      /* process id's match -- merge data, remove nproc, increment proc */
      assert(nproc->pid == proc->pid);
      proc->current_cpuusage=nproc->cpuusage-proc->cpuusage;
      //fprintf(stderr,"pid=%ld uid=%ld current_cpuusage=%g  new=%g  old=%g\n",(long)proc->pid,(long)proc->uid,(double)proc->current_cpuusage,nproc->cpuusage,proc->cpuusage);
      proc->cpuusage=nproc->cpuusage;

      if (proc->uid != nproc->uid) {
	/* process changed uid's */
	/* remove proc instead -- a changed uid is like a new process */
	nproc->reniced=proc->reniced;

	/* increment proc */
	next=(struct procent *)proc->Node.ln_Succ;
	Remove((struct Node *)proc);
	DeleteProc(proc);
	proc=next;
	
	/* increment nproc, but no delete */
	nproc=(struct procent *)nproc->Node.ln_Succ;
      } else {
	/* in this case, we remove nproc*/
	next=(struct procent *)nproc->Node.ln_Succ;
	Remove((struct Node *)nproc);
	DeleteProc(nproc);
	nproc=next;
	

	/* increment proc, but no delete */
	proc=(struct procent *)proc->Node.ln_Succ;
	
      }
    }
    //fprintf(stderr,"proc->current_cpuusage=%g\n",proc->current_cpuusage);
  }

  /* OK. We've copied cpuusage data for pre-existing processes,
     cleaned up dead processes, and removed most pre-existing from 
     nproc. Now we need to properly add the new processes */

  /* go through new processes that are reallynew(tm) and copy their 
     "reniced" value from their parent. Because there can be several 
     layers of descendants, this has to be iterative and based upon a 
     search for the parent */
  

  
  do {
    didsomething=0;
    for (nproc=(struct procent *)newproclist.lh_Head;nproc->Node.ln_Succ;nproc=next) {
      next=(struct procent *)nproc->Node.ln_Succ;
      if (nproc->reallynew) {
	proc=procfindbypid(&proclist,nproc->parentpid); /* find parent */
	if (proc) {
	  didsomething=1;
	  nproc->reniced=proc->reniced;
	  if (nproc->uid==proc->uid) {
	    nproc->badkarma=proc->badkarma; /* bad karma is inherited */
	    nproc->goodflag=proc->goodflag;
	    nproc->immuneflag=proc->immuneflag;
	    nproc->badflag=proc->badflag;
	    nproc->potentialrunaway=proc->potentialrunaway;
	  }
	  nproc->reallynew=0; /* don't look any more */	
	}
      }
      
      /* copy any no longer reallynew processes into the master table */
      if (!nproc->reallynew) {
	didsomething=1;
	SetProcFlags(nproc);
	Remove((struct Node *)nproc); /* remove from newproclist */
	procinsert(&proclist,nproc); /* add to proclist */
	
      }
    }
  } while(didsomething);

  /* any processes remaining in newproclist are children for which we don't
     know the parents */
  
  /* copy remaining newproclist entries into master table */
  while (nproc=(struct procent *)RemHead(&newproclist)) {
    SetProcFlags(nproc);
    procinsert(&proclist,nproc);
  }
  


  /* clean up dead processes */
  
  for (proc=(struct procent *)proclist.lh_Head;proc->Node.ln_Succ;proc=next) {
    next=(struct procent *)proc->Node.ln_Succ;
    
    if (proc->deadflag) {
      Remove((struct Node *)proc);
      DeleteProc(proc);
    }
  }
  
}

void ReniceProcs(double deltat)
{
  struct procent *proc;

  double largest=-1.0;
  double total=0.0;
  double idletime;
  double karmarate,karmarecoverrate;
  double badkarmaincreasefactor;
  double badkarmaincrease;
  int desirednicelevel;
  int ret;
  int actualnicelevel;

  /* find largest CPU user and total CPU time used */

  for (proc=(struct procent *)proclist.lh_Head;proc->Node.ln_Succ;proc=(struct procent *)proc->Node.ln_Succ) {
    if (proc->current_cpuusage > largest) {
      largest=proc->current_cpuusage;
    }
    total+=proc->current_cpuusage;
    
  }
  
  idletime=deltat*numprocessors-total;
  if (idletime > deltat) idletime=deltat; /* we're interested in maximum 
					     idle time on a SINGLE processor */

  if (idletime < 0) {
    if ( (-idletime) >  ((double)periodicity)/100.0) {
      syslog(LOG_WARNING,"%g seconds of CPU used in %g seconds on %d processors\n",total,deltat,numprocessors);
    }
    idletime=0.0;
  }
  
  if (idletime > largest) largest=idletime; /* idle counts as a process, in this case */

  karmarate=((double)badkarmarate)/largest;
  karmarecoverrate=karmarestorationrate/largest;
  
  //fprintf(stderr,"idletime %g largest %g\n",(double)idletime,(double)largest);

  /* Apply bad karma to each process, then renice */
  
  for (proc=(struct procent *)proclist.lh_Head;proc->Node.ln_Succ;proc=(struct procent *)proc->Node.ln_Succ) {

    errno=0;
    actualnicelevel=getpriority(PRIO_PROCESS,proc->pid);
    if (errno) {
      syslog(LOG_WARNING,"Error reading process priority");
      actualnicelevel=0;
    } else {
      proc->nicelevel=actualnicelevel;
    }
    
    if (!proc->immuneflag && !proc->goodflag) {
      /* proc->badkarma+=proc->current_cpuusage*deltat*karmarate-deltat*karmarestorationrate; */
      /* badkarmaincreasefactor makes the badkarma increase smaller once you 
	 have some. The idea is that processes will find a reasonable steady-state */
      badkarmaincreasefactor=1/(1+4*min_mac(proc->badkarma,batchjob)/batchjob);
      badkarmaincrease=proc->current_cpuusage*deltat*karmarate*badkarmaincreasefactor - deltat*karmarecoverrate*(largest-proc->current_cpuusage);
      if (badkarmaincrease > 1.0) badkarmaincrease=1.0; /* upper bound rate of
							   increase of bad karma */
      proc->badkarma+=badkarmaincrease;
      if (proc->badkarma < 0.0)
	proc->badkarma=0.0;
    } else if (proc->immuneflag) {
      proc->badkarma=0.0;
    } else if (proc->goodflag) {
      proc->badkarma=notnice; /* NOTNICE */
    } else if (proc->badflag && proc->badkarma < batchjob) {
      proc->badkarma=batchjob; /* BATCHJOB */
    }
    /* upper bound the bad karma of regular processes at BATCHJOB */
    if (!proc->potentialrunaway && proc->badkarma > batchjob)
      proc->badkarma=batchjob;


    desirednicelevel=(int)proc->badkarma;

    /* insure that we don't make an already-nice process unniced */
    if (proc->nicelevel > 0 && proc->nicelevel-proc->reniced > 0 && desirednicelevel < proc->nicelevel-proc->reniced && !proc->goodflag) {
      desirednicelevel=proc->nicelevel-proc->reniced;
    }


    if (proc->immuneflag)
      desirednicelevel=proc->nicelevel-proc->reniced;
    
    if (desirednicelevel != proc->nicelevel && (!proc->immuneflag || proc->reniced)) {
      /* renice process */
      ret=setpriority(PRIO_PROCESS,proc->pid,desirednicelevel);
      if (!ret) {
	/* setpriority worked! */
	/* update known priority */
	proc->reniced+=desirednicelevel-proc->nicelevel;
	proc->nicelevel=desirednicelevel;
      }
    }

    if(proc->immuneflag) {
      proc->reniced=0;
    }

    if (proc->potentialrunaway && proc->badkarma > term) {
      syslog(LOG_WARNING,"Sending SIGTERM to process %ld, uid %ld. BadKarma=%g",(long)proc->pid,(long)proc->uid,(double)proc->badkarma);
      kill(proc->pid,SIGTERM); /* die!!! */
    } else if (proc->potentialrunaway && proc->badkarma > runaway) { 
      syslog(LOG_WARNING,"Sending SIGKILL to process %ld, uid %ld. BadKarma=%g",(long)proc->pid,(long)proc->uid,(double)proc->badkarma);
      kill(proc->pid,SIGKILL);
    }

  }

}

void UnniceProcs(void)
{
  /* un-nice all procs and empty process table */
  struct procent *proc;

  while (proc=(struct procent *)RemHead(&proclist)) {
    if (!proc->immuneflag) setpriority(PRIO_PROCESS,proc->pid,proc->nicelevel-proc->reniced);
    
    DeleteProc(proc);
  }
}

void performdump(void)
{
  /* dump database to SYSLOG */
  struct procent *proc;
  
  for (proc=(struct procent *)proclist.lh_Head;proc->Node.ln_Succ;proc=(struct procent *)proc->Node.ln_Succ) {
    syslog(LOG_INFO,"%s: pid %ld, nice %ld+%ld, parent %ld, at %g secs CPU (current %g), badkarma %g, uid %ld %s %s %s %s",proc->exename,(long)proc->pid,(long)(proc->nicelevel-proc->reniced),(long)proc->reniced,(long)proc->parentpid,(double)proc->cpuusage,(double)proc->current_cpuusage,(double)proc->badkarma,(long)proc->uid,(proc->immuneflag==0) ? "":"immune",(proc->goodflag==0) ? "":"good",(proc->badflag==0) ? "":"bad",(proc->potentialrunaway==0) ? "": "potentialrunaway");
    
    
  }
}

int CountCPUs(void)
{
  FILE *cpuinfo;
  char Buf[1000];
  int numcpus=1;
  int cpucnt=0;

  cpuinfo=fopen("/proc/cpuinfo","r");
  if (cpuinfo) {
    while(fgets(Buf,sizeof(Buf)-1,cpuinfo)) {
      Buf[sizeof(Buf)-1]=0;
      if (!strncmp(Buf,"processor\t:",11)) {
	cpucnt++;
      }
    }
    fclose(cpuinfo);
    numcpus=cpucnt;
  }

  return numcpus;
}

int main()
{
  int Cnt;
  struct timeval start,end,lastmeas;

  start.tv_sec=0;
  start.tv_usec=0;
  end.tv_sec=0;
  end.tv_usec=0;
  lastmeas.tv_sec=0;
  lastmeas.tv_usec=0;

  signal(SIGTERM,&killhandler);
  signal(SIGINT,&killhandler);
  signal(SIGQUIT,&killhandler);
  signal(SIGHUP,&huphandler);
  signal(SIGUSR1,&dumphandler);
  signal(SIGUSR2,&reconfighandler);

  NewList(&proclist);
  NewList(&newproclist);
  NewList(&iuidlist);
  NewList(&iexelist);
 
  NewList(&bexelist);
  NewList(&gexelist);
  NewList(&runawaylist);
  
  /* open syslog */
#ifdef TARGET_linux /* LOG_PERROR is not standard */
  openlog("verynice",LOG_PERROR,LOG_DAEMON);
#else
  openlog("verynice",0,LOG_DAEMON);
#endif

  numprocessors=CountCPUs();
  if (numprocessors < 1 || numprocessors > 100) numprocessors=1;

  for (Cnt=0;!killflag;Cnt++) {
    if (dumpflag) {
      dumpflag=0;
      
      performdump();
    }

    gettimeofday(&start,NULL);
    /* if we waited an adequate amount */
    if (timediff(&end,&start) > .95*periodicity) {
      if (!(Cnt % rereadcfgperiodicity) || reconfigflag) {
	reconfigflag=0;
	ReadCfgFiles(PREFIX);
      }
      
      gettimeofday(&start,NULL);
      ReadProcs(&newproclist);
      gettimeofday(&end,NULL);
      
      /* do nothing if it took too long to take measurement -- 
	 probably suspend/resume or something */
      if (timediff(&start,&end) > ((double)periodicity)/100.0) {
	/* measurement wasn't fast enough. update cpuusage values, but
	   nothing else */
	syslog(LOG_WARNING,"Process measurement too slow");
	updatecpuusage();
	lastmeas=start;
	
      } else {
	/* measurement was fast enough */
	
	if (hupflag) {
	  hupflag=0;
	  UnniceProcs();
	  ReadCfgFiles(PREFIX);
	  lastmeas.tv_sec=0;
	  lastmeas.tv_usec=0;
	}
	
	MergeProcs();
	if (lastmeas.tv_sec) {
	  ReniceProcs(timediff(&lastmeas,&end));
	}
	lastmeas=start;
	
      }
      
    } else {
      /* Didn't wait long enough */
      Cnt--;
    }
    if (!killflag) sleep(1+periodicity/8);
  }

  UnniceProcs();
  closelog();
}





