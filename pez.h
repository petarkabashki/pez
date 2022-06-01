/*

   Pez
   Program Linkage Definitions

   This module contains the definitions needed by programs that
   invoke the PEZ system.  It does contain the definitions used
   internally within PEZ (which might create conflicts if
   included in calling programs).

   See doc/CREDITS for information about the authors.
   This program is in the public domain.
*/

#ifndef __PEZ_H
#define __PEZ_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#include <pez_sysconfig.h>
#endif

#include <stdio.h>
#include <regex.h>

#define MAX_IO_STREAMS 10
#define MAX_REGEX_MATCHES 20 // Hey, they're small.

#define TOK_BUF_SZ 4096

typedef long pez_int;		// Stack integer type
typedef double pez_real;	// Real number type
typedef long pez_stackitem;

typedef struct pez_inst pez_instance;
typedef struct pez_load_path pez_load_path;
// External symbols accessible by the calling program.
typedef void (pez_word)(pez_instance *);	// Machine code pointer
typedef pez_word *pez_wordp;

//  Dictionary word entry
typedef struct dw {
	struct dw *wnext;	// Next word in dictionary
	char *wname;		/* Word name.  The first character is
				   actually the word flags, including
				   the (IMMEDIATE) bit...until flags are moved
				   outside. */
	pez_wordp wcode;	// Machine code implementation
} pez_dictword;
typedef pez_dictword **rstackitem;

// Internal state marker item
typedef struct {
	pez_int *mstack;	// Stack position marker
	pez_int *mheap;		// Heap allocation marker
	pez_real *mfstack;	// Float stack
	pez_dictword ***mrstack;	// Return stack position marker
	pez_dictword *mdict;	// Dictionary marker
} pez_statemark;

// For loading libraries
struct pez_load_path {
	pez_load_path *next;
	char *path;
};

// For handling structs:
typedef struct {
	char *name;
	long size;
} pez_struct;

// FFI libraries have gotten slightly more complicated than "pointer returned by
// dlopen()".

// Yet another linked list type.  CS101, keepin' it real fo lyfe.
typedef struct pez_ffi_include pez_ffi_include;
struct pez_ffi_include {
	char *fname;
	pez_ffi_include *next;
};

// The lib type:
typedef struct {
	char *name;
	pez_ffi_include *includes;
	void *sofile;
} pez_ffi_lib;

/* Permissions for instances of Pez: */

// Allow I/O operations:
#define PEZ_A_IO (1 << 0)
// Allow the FD manipulation words, like open, close, >output, etc.  Necessarily
// implies I/O permissions.
#define PEZ_A_FILES ((1 << 1) | PEZ_A_IO)
// Allow the manipulation of sighandlers for the current process.
#define PEZ_A_SIG (1 << 2)
// Allow Pez to fork(), wait(), send signals to processes, etc.
#define PEZ_A_PROCESS (1 << 3)
// Allow new FFI functions to be declared.  
#define PEZ_A_FFI (1 << 4)
// Allow Pez to open/close sockets and send data across the network.
#define PEZ_A_SOCKET (1 << 5)
// Allow use of the system() command.  Implies everything but pointers.
#define PEZ_A_SYSTEM (1 << 6)
// Unrestricted pointers (i.e., not pointing to memory allocated by Pez.).  This
// one's a little more complicated, and is only loosely enforced for now.  It
// precludes the use of several libc calls, like getenv/setenv.
#define PEZ_A_POINTERS (1 << 7)
// Initialize *everything*.  No restrictions on Pez.  This is how the Pez
// executable runs, and is required if you want to create more instances of Pez,
// for security reasons.
#define PEZ_A_EVERYTHING (\
		(1 << 8) |\
		PEZ_A_IO |\
		PEZ_A_FILES |\
		PEZ_A_SIG |\
		PEZ_A_PROCESS |\
		PEZ_A_FFI |\
		PEZ_A_SOCKET |\
		PEZ_A_SYSTEM |\
		PEZ_A_POINTERS |\
0)

struct pez_inst {
	// The evaluation stack:
	pez_stackitem *stk;		// Pointer to the current position
	pez_stackitem *stack;		// Stack bottom
	pez_stackitem *stacktop;	// Stack top

	// The return stack:
	pez_dictword ***rstk;	// Return stack pointer
	pez_dictword ***rstack;	// Return stack bottom
	pez_dictword ***rstacktop;	// Return stack top

	// The heap:
	pez_stackitem *hptr;	// Heap allocation pointer
	pez_stackitem *heap;	// Bottom of heap (temp string buffer)
	pez_stackitem *heaptop;	// Top of heap

	// Running code:
	pez_int broken;		// Break?
	pez_int trace;		// Tracing?
	pez_dictword *curword;	// Current word being executed
	pez_dictword **ip;	// Instruction pointer

	// The float stack:
	pez_real *fstk;		// Pointer to the current position
	pez_real *fstack;	// Beginning of the float stack
	pez_real *fstacktop;	// Float stack top

	// Bookkeeping for the three stacks plus heap:

	// Lengths:
	pez_int heaplen;	// Initial/current heap length
	pez_int rstklen;	// Initial/current return stack length
	pez_int stklen;		// Evaluation stack length
	pez_int fstklen;	// Float stack length

	// Max growth:
	pez_stackitem *heapmax;
	pez_dictword ***rstackmax;
	pez_stackitem *stackmax;
	pez_real *fstackmax;

	// The walkback trace stack:
	pez_dictword **wback;	// Walkback trace buffer
	pez_dictword **wbptr;	// Walkback trace pointer

	pez_int permissions;	// The Pez instance's permissions.

	// The dictionary:
	pez_dictword *dict;		// Dictionary chain head
	pez_dictword *dictprot;		// First protected item in dictionary

	// Compiling:
	pez_dictword *createword;	// Address of word pending creation
	pez_struct *createstruct;	// Address of struct being defined.

	// Lexing:
	char *instream;		// Current input stream line
	pez_int tokint;		// Scanned integer
	pez_real tokreal;	// Scanned real number

	// Status, bookkeeping:
	// TODO:  Most of these are booleans; make a flags field and add some
	// support macros.
	short evalstat;	// Evaluation status
	pez_int forgetpend:1;	// Is a "forget" pending?
	pez_int defpend:1;	// Definition pending?
	pez_int walkback:1;	// Walkback enabled?
	pez_int comment:1;	// Ignoring a comment?
	pez_int redef:1;		// Allow redefinition without issuing
				// the "not unique" message?
	pez_int errline:1;	// Line where last pez_load failed
	pez_int stringlit:1;	// Waiting for a string literal?
	pez_int tail_call_pending:1;	// Did we get a tail-call?
	pez_int tickpend:1;	// Waiting for the object of a '?
	pez_int ctickpend:1;	// Waiting for the object of a [']?
	pez_int cbrackpend:1;	// Waiting for the object of a [COMPILE]?

	pez_int base;  // The current base, for formatting output of '.'.

	pez_int argc;
	char **argv;	// The argv that the pez program sees.

	// I/O, as code running in Pez sees it:
	pez_stackitem output_stk[MAX_IO_STREAMS];
	pez_stackitem input_stk[MAX_IO_STREAMS];
	int output_idx;
	int input_idx;

	// Regexes:
	regmatch_t regex_matches[MAX_REGEX_MATCHES];
	// For both of these, offset is index 0, length is index 1.
	pez_stackitem regex_prematch[2];
	pez_stackitem regex_postmatch[2];

	// Library loading:
	pez_load_path *load_path;
	pez_load_path *already_loaded;
};

//  PEZ_EVAL return status codes

#define PEZ_SNORM	0	// Normal evaluation
#define PEZ_STACKOVER	1	// Stack overflow
#define PEZ_STACKUNDER	2	// Stack underflow
#define PEZ_RSTACKOVER	3	// Return stack overflow
#define PEZ_RSTACKUNDER 4	// Return stack underflow
#define PEZ_HEAPOVER	5	// Heap overflow
#define PEZ_BADPOINTER	6	// Pointer outside the heap
#define PEZ_UNDEFINED	7	// Undefined word
#define PEZ_FORGETPROT	8	// Attempt to forget protected word
#define PEZ_NOTINDEF	9	// Compiler word outside definition
#define PEZ_RUNSTRING	10	// Unterminated string
#define PEZ_RUNCOMM	11	// Unterminated comment in file
#define PEZ_BREAK	12	// Asynchronous break signal received
#define PEZ_DIVZERO	13	// Attempt to divide by zero
#define PEZ_APPLICATION 14	// Application primitive pez_error()
#define PEZ_BADFILE	15	// Attempt to load a bad file
#define PEZ_FSTACKUNDER	16	// Float stack underflow
#define PEZ_FSTACKOVER	17	// Float stack overflow

//  Entry points
extern pez_instance *pez_init(long flags);
extern void pez_mark(pez_instance *p, pez_statemark *mp),
       pez_unwind(pez_instance *p, pez_statemark *mp),
       pez_break(pez_instance *p),
       pez_free(pez_instance *p);
extern int pez_eval(pez_instance *p, char *sp);
extern long pez_pop(pez_instance *p);
extern int pez_push(pez_instance *p, long i);
extern int pez_anticipating_token(pez_instance *p);
extern char *pez_which_lib(pez_instance *p, char *libname);
extern int pez_load(pez_instance *p, FILE *fp);
extern int pez_ffi_load(pez_instance *p, char *libname);

extern void pez_stack_int(pez_instance *p, pez_int i);
extern void pez_stack_float(pez_instance *p, pez_real f);
extern void pez_stack_string(pez_instance *p, char *s);

extern void pez_heap_float(pez_instance *p, pez_real f);
extern void pez_heap_string(pez_instance *p, char *s);
extern void pez_memstat(pez_instance *p);

char *pez_strdup(char *s);
char *alloc(unsigned long size);

#endif
