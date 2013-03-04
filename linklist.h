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
/* See linklist.c for documentation */

#ifndef NULL
#define NULL (void *)0L
#endif

struct Node {
	struct Node *ln_Succ,*ln_Pred;
	unsigned char ln_Type;
	char ln_Pri,*ln_Name;
};

struct List {
	struct Node *lh_Head,*lh_Tail,*lh_TailPred;
	unsigned char lh_Type,l_pad;
};

struct MinNode {
	struct MinNode *mln_Succ,*mln_Pred;
};

struct MinList {
	struct MinNode *mlh_Head, *mlh_Tail,*mlh_TailPred;
};

void Insert(struct List *ThisList,struct Node *AddNode,struct Node *Pred);
void Remove(struct Node *RemNode);
void AddHead(struct List *ThisList,struct Node *AddNode);
struct Node *RemHead(struct List *ThisList);
void AddTail(struct List *ThisList,struct Node *AddNode);
struct Node *RemTail(struct List *ThisList);
void Enqueue(struct List *ThisList,struct Node *AddNode);
struct Node *FindName(struct List *ThisList,char *Name);
void NewList(struct List *List);
