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

struct procent {
	struct Node Node;
	pid_t pid;
	int nicelevel;				/* actual nice level of process, including amount reniced by
								 * verynice */
	int reniced;				/* amount reniced by verynice(tm) */
	pid_t parentpid;
	double cpuusage;			/* total cpuusage up to now of process, in seconds */

	double current_cpuusage;	/* current delta of cpuusage */

	double badkarma;			/* accumulated bad karma of process */
	uid_t uid;
	int immuneflag;				/* pid and children of same uid are immune */
	int goodflag;				/* pid and children are 'good' -- should automatically 
								 * be reniced negatively, regardless of CPU usage */
	int badflag;				/* pid and children are 'bad'  -- should be niced automatically,
								 * regardless of CPU usage */
	int potentialrunaway;		/* very heavy CPU usage will cause this to be 
								 * set to RUNAWAY level */
	int hungryflag;				/* Always assume 100% cpu usage (not inherited) */
	char *exename;				/* name of executable, separate malloc */
	int reallynew;				/* set if this is really a new process or not */
	int deadflag;				/* waiting to be expunged */
};

extern struct List proclist;
extern struct List newproclist;

struct immuneuid {
	struct Node Node;
	uid_t uid;					/* these uid are immune */

};

extern struct List iuidlist;

struct immuneexe {
	struct Node Node;
	char *exename;				/* name or substring of executable, separate malloc */
	int alluid;					/* 1 if this applys to all uids, 0 if it applys only to uid,
								 * below */
	uid_t uid;
};

extern struct List iexelist;

struct badexe {
	struct Node Node;
	char *exename;				/* name or substring of executable, separate malloc */
	int alluid;					/* 1 if this applys to all uids, 0 if it applys only to uid,
								 * below */
	uid_t uid;
};
extern struct List bexelist;

struct goodexe {				/* for multimedia apps, which should be negatively niced */
	struct Node Node;
	char *exename;				/* name or substring of executable, separate malloc */
	int alluid;					/* 1 if this applys to all uids, 0 if it applys only to uid,
								 * below */
	uid_t uid;
};
extern struct List gexelist;

struct runawayexe {				/* for apps that tend to get into runaway CPU usage
								 * (e.g. netscape) */
	struct Node Node;
	char *exename;				/* name or substring of executable, separate malloc */
	int alluid;					/* 1 if this applys to all uids, 0 if it applys only to uid,
								 * below */
	uid_t uid;
};
extern struct List runawaylist;

struct hungryexe {
	/* for apps that are CPU hungry whether or not they appear to be
	 * appear to be (e.g. make) */
	struct Node Node;
	char *exename;
	int alluid;					/* 1 if this applys to all uids, 0 if it applys only to uid,
								 * below */
	uid_t uid;

};

extern struct List hungrylist;

struct knownuid {				/* track the UID's for which we've attempted to read
								 * config information */
	struct Node Node;
	uid_t uid;
};
extern struct List uidlist;

#define GLOBALCFGFILE "verynice.conf"
#define USERCFGFILE ".verynicerc"

extern int normal;
extern int notnice;
extern int batchjob;
extern int runaway;
extern int killproc;

extern int periodicity;
extern int rereadcfgperiodicity;

extern double badkarmarate;
extern double karmarestorationrate;

/* WARNING: side effects:
       - Assumes syslog(3) is activated. Caller is responsible for opening/closing
       - May open password database if not already open, and adjust location
       - This routine closes password database
  */
void ReadCfgFiles(char *prefix);

struct knownuid *finduid(uid_t uid);

/* caller responsible for calling endpwent() after this */
/* this reads in the config file, and adds an appropriate entry
   to the known uid list 
   (returns pointer to entry in known uid list) */
struct knownuid *ReadCfgFile(uid_t uid);
