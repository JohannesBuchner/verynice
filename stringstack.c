
/*
     VeryNice -- a dynamic process re-nicer
     Copyright (C) 1992-1999 Stephen D. Holland
 
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; 
     version 2.1 of the License.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

*/


#define STDCALLOC
#ifdef STDCALLOC
#define MALLOCFUNC(size) malloc(size)
#define CALLOCFUNC(size) calloc(1,(size))
#define FREEFUNC(ptr,size) free(ptr)
#else
#ifdef AMIGAEXECALLOC
#define MALLOCFUNC(size) AllocMem((size),MEMF_PUBLIC)
#define CALLOCFUNC(size) AllocMem((size),MEMF_PUBLIC|MEMF_CLEAR)
#define FREEFUNC(ptr,size) FreeMem((ptr),(size))
#else
#error StringStack needs to know which allocation functions to use!
#endif
#endif


#include <string.h>
#include <stdlib.h>

#include "linklist.h"
#include "stringstack.h"

#if (STRINCREMENT < 1)
	#error Zero string increment!
#endif

static struct StkNode *GetTopStkNode(struct Stack *Stk)
{
	if (!Stk) return NULL;
	
	if (Stk->StringList.mlh_TailPred == (struct MinNode *)&Stk->StringList.mlh_Head)
		return NULL; /* Empty list */
	return (struct StkNode *)Stk->StringList.mlh_TailPred;
}

/* Push a string into a new stack entry */
char *PushString(struct Stack *Stk,char *Str)
{
	if (!Stk || !Str) return NULL;
	
	if (!InitString(Stk)) return NULL;
	if (!AddStr(Stk,Str)) return NULL;
	return Str;
}

/*
	remove and copy into static storage the top string on the stack.
	NOTE: pointer returned will become invalid the next time PopString()
	is called with the same stack pointer 
*/
char *PopString(struct Stack *Stk)
{
	struct StkNode *StkNode;
	int Len;
	
	if (!Stk) return NULL;
	
	StkNode=(struct StkNode *)RemTail((struct List *)&Stk->StringList);
	if (!StkNode) return NULL;
	
	Len=strlen(StkNode->String);
	if (Stk->PopStringBuf && Stk->PopStringBufSize >= (Len+1)) {
		strcpy(Stk->PopStringBuf,StkNode->String);
		FREEFUNC(StkNode,StkNode->AllocLen);
		return Stk->PopStringBuf;
	} else {
		if (Stk->PopStringBuf) {
			FREEFUNC(Stk->PopStringBuf,Stk->PopStringBufSize);
		}
		Stk->PopStringBuf=(char *)MALLOCFUNC(Stk->PopStringBufSize=(Len+1));
		if (!Stk->PopStringBuf) {FREEFUNC(StkNode,StkNode->AllocLen);return NULL;}
		strcpy(Stk->PopStringBuf,StkNode->String);
		FREEFUNC(StkNode,StkNode->AllocLen);
		return Stk->PopStringBuf;
	}
	
}

/* Add a character to the end of the string at the top of the stack */
/* Returns character, or -1 for error */
int AddChar(struct Stack *Stk,int Chr)
{
	struct StkNode *StkNode,*OldNode;
	struct MinNode *Pred;
	int Len,ExtraSpace,SpaceAlloc;
	
	if (!Stk) return -1;
	
	StkNode=GetTopStkNode(Stk);
	if (!StkNode) return -1;
	
	Len=strlen(StkNode->String);
	
	ExtraSpace=StkNode->AllocLen-sizeof(struct StkNode)-Len-1-1;
	if (ExtraSpace < 0) {
		SpaceAlloc=((((-ExtraSpace*STRINCREMENT)/STRINCREMENT)+STRINCREMENT)) + StkNode->AllocLen;
		Pred=StkNode->Node.mln_Pred;
		Remove((struct Node *)StkNode);
		StkNode=(struct StkNode *)realloc(OldNode=StkNode,SpaceAlloc);
		if (!StkNode) { /* if error, put it back on list and return error */
			Insert((struct List *)&Stk->StringList,(struct Node *)OldNode,(struct Node *)Pred);
			return -1;
		}
		Insert((struct List *)&Stk->StringList,(struct Node *)StkNode,(struct Node *)Pred);
    StkNode->AllocLen=SpaceAlloc;
    SETSTRING(StkNode);
	}
	StkNode->String[strlen(StkNode->String)+1]=0; /* Null terminator */
	StkNode->String[strlen(StkNode->String)]=Chr;
	return Chr;
	
}

/* Add a string to the end of the string at the top of the stack */
/* Returns original string pointer, or NULL for error */
char *AddStr(struct Stack *Stk,char *Str)
{
	struct StkNode *StkNode,*OldNode;
	struct MinNode *Pred;
	int Len,ExtraSpace,SpaceAlloc;
	
	if (!Stk || !Str) return NULL;
	
	StkNode=GetTopStkNode(Stk);
	if (!StkNode) return NULL;
	
	Len=strlen(StkNode->String);
	
	ExtraSpace=StkNode->AllocLen-sizeof(struct StkNode)-Len-1-strlen(Str);
	if (ExtraSpace < 0) {
		SpaceAlloc=((((-ExtraSpace*STRINCREMENT)/STRINCREMENT)+STRINCREMENT)) + StkNode->AllocLen;
		Pred=StkNode->Node.mln_Pred;
		Remove((struct Node *)StkNode);
		StkNode=(struct StkNode *)realloc(OldNode=StkNode,SpaceAlloc);
		if (!StkNode) { /* if error, put it back on list and return error */
			Insert((struct List *)&Stk->StringList,(struct Node *)OldNode,(struct Node *)Pred);
			return NULL;
		}
		Insert((struct List *)&Stk->StringList,(struct Node *)StkNode,(struct Node *)Pred);
    StkNode->AllocLen=SpaceAlloc;
    SETSTRING(StkNode);
	}
	strcat(StkNode->String,Str);
	return Str;
}

/* Get pointer to the string on the top of the stack. This pointer
becomes invalid if AddChar or AddStr is called on the stack */
char *GetString(struct Stack *Stk)
{
	struct StkNode *StkNode;
	if (!Stk) return NULL;
	
	StkNode=GetTopStkNode(Stk);
	if (!StkNode) return NULL;
	return StkNode->String;
}

/* Initialize a string on the top of the stack */
char *InitString(struct Stack *Stk)
{
	struct StkNode *StkNode;
	if (!Stk) return NULL;
	
	StkNode=(struct StkNode *)CALLOCFUNC(sizeof(struct StkNode)+STRINCREMENT);
	if (!StkNode) return NULL;
	StkNode->AllocLen=sizeof(struct StkNode)+STRINCREMENT;
	SETSTRING(StkNode);
	StkNode->String[0]='\0';
	AddTail((struct List *)&Stk->StringList,(struct Node *)StkNode);
}

/* close down a stack */
void UnitStack(struct Stack *Stk)
{
	if (!Stk) return;
	while (PopString(Stk)) ; /* pop all strings off the stack */
	if (Stk->PopStringBuf) FREEFUNC(Stk->PopStringBuf,Stk->PopStringBufSize);
	FREEFUNC(Stk,sizeof(struct Stack));
}

/* Initialize a stack */
struct Stack *InitStack(void)
{
	struct Stack *Stk;
	
	Stk=(struct Stack *)CALLOCFUNC(sizeof(struct Stack));
	if (!Stk) return NULL;
	Stk->PopStringBuf=NULL;
	Stk->PopStringBufSize=0;
	NewList((struct List *)&Stk->StringList);
	return Stk;
}

int CountStack(struct Stack *Stk)
     /* Count the number of entries on a stack */
{
  struct MinNode *Node;
  int Cnt=0;

  for (Node=Stk->StringList.mlh_Head;Node->mln_Succ;Node=Node->mln_Succ)
    Cnt++;
  return Cnt;
}
