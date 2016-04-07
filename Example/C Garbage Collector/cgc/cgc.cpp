// --------------------------------------------------------------------------
// Author: Yasin HINISLIOÐLU
//
// 2008 - Ankara, TURKEY
// --------------------------------------------------------------------------
#include "cgc.h"
#include <stdlib.h>
#include <assert.h>

#include <stdio.h> // remove this line
#include <conio.h> // remove this line


// this structure holds root information
typedef struct tagROOT_INFO
{
	GC_PTR start;
	GC_PTR end;
}ROOT_INFO, *PROOT_INFO;


// this structure holds memory block information
typedef struct tagMEM_BLOCK
{
	struct tagMEM_BLOCK *next;
	struct tagMEM_BLOCK *prev;
	size_t size;
	GC_BOOL mark;
}MEM_BLOCK, *PMEM_BLOCK;



// internal global variables
ROOT_INFO g_Roots[MAX_ROOTS];
GC_ULONG g_lNumRoots;
GC_PTR g_pHeapMax = (GC_PTR) 0;
GC_PTR g_pHeapMin = (GC_PTR)-1;
GC_BOOL g_bMarkCycle = GC_TRUE;
GC_BOOL g_bIsInitialized = GC_FALSE;
GC_PTR g_Regs[4];



// heap and free list headers
PMEM_BLOCK g_pBlkHead = 0;
PMEM_BLOCK g_pBlkTail = 0;
PMEM_BLOCK g_pFreeHead = 0;
PMEM_BLOCK g_pFreeTail = 0;



// useful macros
#define ADD_TO_LIST( head, tail, ptr ) \
	ptr->next = 0; \
	ptr->prev = 0; \
	if( 0 != tail ) \
	{ \
		tail->next = ptr; \
		ptr->prev = tail; \
	} \
	else head = ptr; \
	tail = ptr;

#define REMOVE_FROM_LIST( head, tail, ptr ) \
	if( 0 != ptr->prev ) ptr->prev->next = ptr->next; else head = ptr->next; \
	if( 0 != ptr->next ) ptr->next->prev = ptr->prev; else tail = ptr->prev;

// atom variables are skipped in mark phase
#define gc_is_atom( p ) ( p == (GC_PTR)&g_Roots \
				|| p == (GC_PTR)&g_pBlkHead \
				|| p == (GC_PTR)&g_pBlkTail \
				|| p == (GC_PTR)&g_pHeapMax \
				|| p == (GC_PTR)&g_pHeapMin \
				|| p == (GC_PTR)&g_pFreeHead \
				|| p == (GC_PTR)&g_pFreeTail )
				// we do not add g_Regs global variable here, because we want GC
				// to scan memory areas inside this variable


#define BLOCK_HEAD( ptr ) ((PMEM_BLOCK)(((PBYTE)ptr) - sizeof(MEM_BLOCK)))
#define BLOCK_PTR( ptr ) ((PMEM_BLOCK)(((PBYTE)ptr) + sizeof(MEM_BLOCK)))



// forward declarations
void gc_sweep_all( void );
void gc_add_root( GC_PTR start, GC_PTR end );




#ifdef WIN32
#include <windows.h> 

#ifndef DEBUG_SILENT
#define gc_out( format, ... ) \
		{ \
		va_list args = '\0'; \
		va_start( args, format ); \
		printf( format, __VA_ARGS__ ); \
		}
#else
#define gc_out( format, ... )
#endif

// --------------------------------------------------------------------------
void gc_stack_roots( void )
{
	MEMORY_BASIC_INFORMATION mem_info;
	size_t result;
	GC_PTR start;
	GC_PTR end;
	int dummy;
	
	result = VirtualQuery( &dummy, &mem_info, sizeof( MEMORY_BASIC_INFORMATION ) );
	if( result == sizeof( mem_info ) )
	{
		start = (GC_PTR)&dummy;
		end = (GC_PTR)(((GC_PBYTE)mem_info.BaseAddress) + mem_info.RegionSize);

		// this heuristic approach is to determine grow direction of the stack
		if( end > start )
			gc_add_root( start, end );
		else
			gc_add_root( end, start );
	}
}



// --------------------------------------------------------------------------
void gc_datasegment_roots( void )
{
	static GC_BYTE dummy;
	MEMORY_BASIC_INFORMATION mem_info;
	size_t result;
	GC_PBYTE start;
	GC_PBYTE end;
	LPVOID tmp;
	SYSTEM_INFO sys_info;


	// get basic information about system memory
	GetSystemInfo( &sys_info );


	// query to find base address for active data segment block
	// it is better to select application base address to scan data segment
	// @TODO: increase scan area to blocks less than and greater than active
	// block
	result = VirtualQuery( &dummy, &mem_info, sizeof( MEMORY_BASIC_INFORMATION ) );
	if( sizeof( MEMORY_BASIC_INFORMATION ) != result )
		return;


	start = (GC_PBYTE)mem_info.BaseAddress;
	tmp = (LPVOID)start;
	end = start;


	// scan through active data block to find the end of data segment
	while( tmp < sys_info.lpMaximumApplicationAddress )
	{
		result = VirtualQuery( tmp, &mem_info, sizeof( mem_info ) );
		if( sizeof( mem_info ) != result || 0 == mem_info.AllocationBase )
			break;

		// next page
		tmp = (GC_PBYTE)tmp + mem_info.RegionSize;
		if( mem_info.State == MEM_COMMIT &&  ( PAGE_READWRITE == mem_info.Protect || PAGE_WRITECOPY == mem_info.Protect || PAGE_EXECUTE_READWRITE == mem_info.Protect || PAGE_EXECUTE_WRITECOPY == mem_info.Protect ) )
		{
			end = (GC_PBYTE)tmp;
		}
	}

	// do we have something?
	if( start != end )
		gc_add_root( (GC_PTR)start, (GC_PTR)end );
}
#endif



// --------------------------------------------------------------------------
void gc_init( void )
{
	g_bIsInitialized = GC_TRUE;
	g_Regs[0] = 0;
	g_Regs[1] = 0;
	g_Regs[2] = 0;
	g_Regs[3] = 0;

	// call gc collector at application exit
	atexit( gc_sweep_all );
}



// --------------------------------------------------------------------------
void gc_add_root( GC_PTR start, GC_PTR end )
{
	assert( g_lNumRoots < MAX_ROOTS );

	g_Roots[ g_lNumRoots ].start = start;
	g_Roots[ g_lNumRoots ].end = end;


	gc_out("Root: 0x%X 0x%X (%d bytes)\n", start, end, end-start );
	g_lNumRoots++;
}



// --------------------------------------------------------------------------
GC_BOOL gc_is_heap_pointer( GC_PTR ptr )
{
	PMEM_BLOCK tmp;

	// scan allocated blocks to check whether sent
	// parameter is heap pointer or not
	// @TODO: speed up this routine. 
	tmp = g_pBlkHead;
	while( 0 != tmp )
	{
		if( (GC_PTR)BLOCK_PTR(tmp) == ptr )
			return GC_TRUE;

		tmp = tmp->next;
	}

	return GC_FALSE;
}



// --------------------------------------------------------------------------
GC_BOOL gc_is_possible_pointer( GC_PTR ptr )
{
	GC_ULONG i;

	// if data is possible heap pointer notify caller
	if( ptr >= g_pHeapMin && ptr <= g_pHeapMax )
		return GC_TRUE;

	// if it is not, check whether it is a pointer located inside
	// our root spaces.
	for( i=0; i<g_lNumRoots; i++ )
	{
		// it is pointing our root spaces. recursively check
		// whether it is a pointer or not
		if( ptr >= g_Roots[i].start && ptr <= g_Roots[i].end )
		{
			// this line is to prevent possible deadlock
			if( (GC_PTR)*ptr == ptr )
				return GC_FALSE;

			// call recursively to find pointer to pointers
			return gc_is_possible_pointer( (GC_PTR)*ptr );
		}
	}

	// this data is not belong to our spaces 
	// we can notify it as not a reference
	return GC_FALSE;
}



// --------------------------------------------------------------------------
GC_BOOL gc_is_pointer( GC_PTR ptr )
{
	// check whether is it pointer to pointer
	// or points somewhere between allocated heap minimum and maximum
	if( !gc_is_possible_pointer( ptr ) )
		return GC_FALSE;

	// this value passed first identification pass and it should point
	// one of the allocated chunks. trace through allocated heap to find
	// whether is it allocated by this collector or not
	if( gc_is_heap_pointer( (GC_PTR)*ptr ) )
	{
		gc_out( "Pointer Found: 0x%X 0x%X\n", ptr, *ptr );
		return GC_TRUE;
	}

	return GC_FALSE;
}



// --------------------------------------------------------------------------
GC_BOOL gc_is_valid_pointer( GC_PTR ptr )
{
	// possible pointer check

	// if this is one of the memory areas to be skipped, then return false
	if( gc_is_atom( ptr ) )
		return GC_FALSE;

	// if this is a pointer that passed our identification phase then return true
	if( gc_is_pointer( ptr ) )
		return GC_TRUE;

	// this is not a valid pointer
	return GC_FALSE;
}



// --------------------------------------------------------------------------
void gc_mark( void )
{
	GC_PTR start;
	GC_PTR end;
	PMEM_BLOCK blk;
	GC_ULONG i;

	g_bMarkCycle = !g_bMarkCycle;

	// scan and mark roots
	for( i=0; i<g_lNumRoots; i++ )
	{
		start = g_Roots[i].start;
		end = g_Roots[i].end;
		
		while( start < end )
		{
			if( gc_is_valid_pointer( start ) )
			{
				gc_out("Marked: 0x%X 0x%X\n", start, (GC_PTR)*start );
				blk = BLOCK_HEAD( (*start) );
				blk->mark = g_bMarkCycle;

				REMOVE_FROM_LIST( g_pBlkHead, g_pBlkTail, blk );
				ADD_TO_LIST( g_pFreeHead, g_pFreeTail, blk );
			}
			start += 1;
		}
	}

	blk = g_pBlkHead;
	g_pBlkHead = g_pFreeHead;
	g_pFreeHead = blk;
	blk = g_pBlkTail;
	g_pBlkTail = g_pFreeTail;
	g_pFreeTail = blk;
}




 // --------------------------------------------------------------------------
void gc_sweep( void )
{
	PMEM_BLOCK tmp, tmp1;

	// free unused data areas
	tmp = g_pFreeHead;
	while( 0 != tmp )
	{
		if( tmp->mark != g_bMarkCycle )
		{
			REMOVE_FROM_LIST( g_pFreeHead, g_pFreeTail, tmp );
			
			tmp1 = tmp;
			tmp = tmp->next;

			// free
			gc_out("Freed: 0x%X 0x%X\n", BLOCK_PTR( tmp1 ), *(BLOCK_PTR(tmp1)) );
			free( tmp1 );
		}
		else
			tmp = tmp->next;
	}
}



// --------------------------------------------------------------------------
void gc_sweep_all( void )
{
	PMEM_BLOCK tmp, tmp1;

	gc_sweep();

	tmp = g_pBlkHead;
	while( 0 != tmp )
	{
		tmp1 = tmp;
		tmp = tmp->next;
		free( tmp1 );
	}

	g_pBlkHead = 0;
	g_pBlkTail = 0;
}


// --------------------------------------------------------------------------
void *gc_malloc( size_t size )
{
	PMEM_BLOCK blk;

	if( !g_bIsInitialized ) gc_init();

	// no size return nil
	if( 0 == size ) 
		return 0;

	// increase size to hold our header
	size += sizeof( MEM_BLOCK );

	// create memory area
	blk = (PMEM_BLOCK)malloc( size );
	if( 0 == blk )
		return 0;

	// initialize header
	blk->next = 0;
	blk->prev = g_pBlkTail;
	blk->size = size;
	blk->mark = g_bMarkCycle;

	// adjust list
	ADD_TO_LIST( g_pBlkHead, g_pBlkTail, blk );

	// these variables are used to determine heap space
	if( (GC_PTR)BLOCK_PTR(blk) > g_pHeapMax ) g_pHeapMax = (GC_PTR)BLOCK_PTR(blk);
	if( (GC_PTR)BLOCK_PTR(blk) < g_pHeapMin ) g_pHeapMin = (GC_PTR)BLOCK_PTR(blk);

	gc_out("Alloc: header adr, 0x%X ptr adr, 0x%X\n", (GC_PTR)blk, (GC_PTR)BLOCK_PTR(blk) );

	return (void*)( BLOCK_PTR(blk) );
}



// --------------------------------------------------------------------------
void gc_free( void *ptr )
{
	PMEM_BLOCK blk;

	if( !g_bIsInitialized ) gc_init();

	if( 0 == ptr )
		return;

	blk = BLOCK_HEAD( ptr );
	REMOVE_FROM_LIST( g_pBlkHead, g_pBlkTail, blk );

	free( blk );
}



// --------------------------------------------------------------------------
void gc_collect_inner( void )
{
	if( !g_bIsInitialized ) gc_init();

	// get information about roots
	g_lNumRoots = 0;
	gc_stack_roots();
	gc_datasegment_roots();

	// mark and sweep phase
	gc_mark();
	gc_sweep();
}