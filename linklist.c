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

#include <string.h>
#include "linklist.h"

/* linklist.c -- This API is derived from the link list routines 
   that were included as part of the original Amiga operating system.

   The basic idea is to provide a doubly linked-list system with a 
   minimum of special cases required in the handling code. This is
   achieved by having two permanent members ("nodes") of the list, 
   called the Head and the Tail, that are never removed and have no
   actual data associated with them. Because these members are permanent,
   there are no special cases involved with removing or adding the first
   or last nodes of the list. 

   The head and tail nodes are contained within the "List" structure, and
   actually overlap! The "Predecessor" field of the head node is the 
   same as the "Successor" field of the tail node (both are always NULL). 

   Lists must be initialized with NewList() before they can be used. */


void Insert(struct List *ThisList,struct Node *AddNode,struct Node *Pred)
{
	if (AddNode==NULL) return;
	if (Pred==NULL) {
		Pred=(struct Node *)&ThisList->lh_Head;
	}
	AddNode->ln_Pred=Pred;
	AddNode->ln_Succ=Pred->ln_Succ;
	Pred->ln_Succ=AddNode;
	AddNode->ln_Succ->ln_Pred=AddNode;
}

void Remove(struct Node *RemNode)
{
	RemNode->ln_Pred->ln_Succ=RemNode->ln_Succ;
	RemNode->ln_Succ->ln_Pred=RemNode->ln_Pred;
	RemNode->ln_Pred=NULL;
	RemNode->ln_Succ=NULL;
}

void AddHead(struct List *ThisList,struct Node *AddNode)
{
	Insert(ThisList,AddNode,NULL);
}

struct Node *RemHead(struct List *ThisList)
{
	struct Node *Head;
	Head=ThisList->lh_Head;
	if (Head->ln_Succ==NULL) {
		return NULL;
	}
	Remove(Head);
	return Head;
}

void AddTail(struct List *ThisList,struct Node *AddNode)
{
	Insert(ThisList,AddNode,ThisList->lh_TailPred);
}

struct Node *RemTail(struct List *ThisList)
{
	struct Node *Tail;
	Tail=ThisList->lh_TailPred;
	if (Tail->ln_Pred==NULL) {
		return NULL;
	}
	Remove(Tail);
	return Tail;
}

void Enqueue(struct List *ThisList,struct Node *AddNode)
{
	struct Node *CurrentSucc;
	for (CurrentSucc=ThisList->lh_Head;
		(CurrentSucc->ln_Succ!=NULL)&&(CurrentSucc->ln_Pri >= AddNode->ln_Pri);
		CurrentSucc=CurrentSucc->ln_Succ) ;
	Insert(ThisList,AddNode,CurrentSucc->ln_Pred);
}

struct Node *FindName(struct List *ThisList,char *Name)
{
	struct Node *CurrentNode;
	for (CurrentNode=ThisList->lh_Head;
		CurrentNode->ln_Succ->ln_Succ != NULL;
		CurrentNode=CurrentNode->ln_Succ) {
			if (strcmp(CurrentNode->ln_Name,Name)==0) {
				return CurrentNode;
			}
	}
	return NULL;
}

void NewList(struct List *List)
/* set up a list for use */
{
	if (!List) return;
	List->lh_Head=(struct Node *)&List->lh_Tail;
	List->lh_Tail=NULL;
	List->lh_TailPred=(struct Node *)&List->lh_Head;
	List->lh_Type=0;
}

