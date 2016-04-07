// --------------------------------------------------------------------------
// Author: Yasin HINISLIOÐLU
//
// 2008 - Ankara, TURKEY
// --------------------------------------------------------------------------

// portable primitive types
typedef int *GC_PTR;
typedef unsigned char GC_BYTE;
typedef unsigned char *GC_PBYTE;
typedef unsigned long GC_ULONG;
typedef unsigned int size_t;
typedef int GC_BOOL;
#define GC_TRUE 1
#define GC_FALSE !GC_TRUE


// maximum root sets
#define MAX_ROOTS 1024

// global variable to hold register values
extern GC_PTR g_Regs[4];

// these macros adds active registers in use, to root list
#define ADD_ONE_REG( a, ind ) _asm { mov g_Regs+ind, ##a }

// we can include eax and ebx accumulator registers but in some cases they
// hold values of pointers which actually released. this may prevent those pointers
// being collected, if collector takes place after that type of statement.
#define SAVE_REGS() \
	/*ADD_ONE_REG( eax, 0 ); 
	ADD_ONE_REG( ebx, 4 );*/ \
	ADD_ONE_REG( ecx, 8 ); \
	ADD_ONE_REG( edx, 12 );

// --------------------------------------------------------------------------
extern void *gc_malloc( size_t size );
extern void gc_free( void *ptr );
extern void gc_collect_inner( void );

// this macro saves register values to our global variables before they have
// been deformed by callee
#define gc_collect() SAVE_REGS(); gc_collect_inner();


