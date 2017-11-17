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

struct Stack {
	struct MinList StringList;
	char *PopStringBuf;
	long PopStringBufSize;		/* Allocated size */
};

struct StkNode {
	struct MinNode Node;
	char *String;				/* String actually stored in memory after StkNode */
	int AllocLen;				/* Allocated length, including StkNode and string */
};

#define SETSTRING(StkNode) {((struct StkNode *)(StkNode))->String=((char *)(StkNode))+sizeof(struct StkNode);}
#define STRINCREMENT 150		/* incremental string size -- between realloc()s */

/* Push a string into a new stack entry */
/* returns NULL in case of error or original string pointer */
char *PushString(struct Stack *Stk, char *Str);

/*
	remove and copy into static storage the top string on the stack.
	NOTE: pointer returned will become invalid the next time PopString()
	is called with the same stack pointer
	returns NULL in case of error.
*/
char *PopString(struct Stack *Stk);

/* Add a character to the end of the string at the top of the stack */
/* Returns character, or -1 for error */
int AddChar(struct Stack *Stk, int Chr);

/* Add a string to the end of the string at the top of the stack */
/* Returns original string pointer, or NULL for error */
char *AddStr(struct Stack *Stk, char *Str);

/* Get pointer to the string on the top of the stack. This pointer
becomes invalid if AddChar or AddStr is called on that member of the stack.
IT REMAINS VALID IF InitString() IS CALLED AND AddChar/AddStr is used on
another member of the stack */
/* returns NULL in case of error */
char *GetString(struct Stack *Stk);

/* Initialize a string on the top of the stack */
/* returns NULL in case of error */
char *InitString(struct Stack *Stk);

/* Insert Str at the beginning of the string on the top of the stack */
void InsertString(struct Stack *Stk, char *Str);

/* Initialize a stack */
/* returns NULL in case of error */
struct Stack *InitStack(void);

/* close down a stack */
void UnitStack(struct Stack *Stk);

/* Count the number of entries on a stack */
int CountStack(struct Stack *Stk);
