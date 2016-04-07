// --------------------------------------------------------------------------
// Author: Yasin HINISLIOÐLU
//
// 2008 - Ankara, TURKEY
// --------------------------------------------------------------------------
#include <stdio.h>
#include <memory.h>
#include "cgc.h"

#include <windows.h>


#define KILOBYTE (1024)
#define MEGABYTE (1024 * KILOBYTE)
#define GIGABYTE (1024 * MEGABYTE)



// --------------------------------------------------------------------------
// simplest test case. create a memory area and equalize it to zero. 
void case1( void )
{
	int *p;

	printf("case 1:\n");

	p = (int*)gc_malloc( sizeof(int) );
	p = 0;

	gc_collect();

	printf("\n");
}

// --------------------------------------------------------------------------
// this test case loses the beggining of the memory in this case there is
// no variable holds the start of the array. gc should not release this memory
// area so it is in use.
void case2( void )
{
	int *start, *end;
	
	printf("case 2:\n");

	start = (int*)gc_malloc( sizeof(int)*20 );
	memset( start, sizeof(int)*20, 0 );
	start[0] = 33;
	
	// what we are doing here is losing start of the array
	end = start+20;
	for( ; start<end; start++ )
	{
		// do stg
	}

	gc_collect();

	// if GC mechanism is not working fine we can lose some memory
	// area when it is in use. following two lines are for testing this case
	start -= 20;
	printf("%d\n", start[0]);

	printf("\n");
}



// --------------------------------------------------------------------------
// this case is about pointer of pointers
void case3( void )
{
	int *p;
	int **p1;
	int ***p2;
	int ****p3;

	printf("case 3:\n");
	p = (int*)gc_malloc( sizeof(int) );
	p1 = &p;
	p2 = &p1;
	p3 = &p2;

	p = 0;
	gc_collect();
	printf("\n");
}


// --------------------------------------------------------------------------
// this test case is passing pointers to functions
void case4_1( int *p, char *c )
{
	*p = 0x33;
	*c = 0x01;
	gc_collect();
}

void case4( void )
{
	printf("case 4:\n");
	case4_1( (int*)gc_malloc( sizeof(int) ), (char*)gc_malloc( sizeof(char) ) );
	printf("\n");
}


// --------------------------------------------------------------------------
// pointer of global variable test
int *g_test5;

void case5( void )
{
	printf("case 5:\n");
	g_test5 = (int*)gc_malloc( sizeof(int) );
	*g_test5 = 0x100;
	gc_collect();
	printf("\n");
}


// --------------------------------------------------------------------------
// this test case is to see what happens if we have cyclic data structure
void case6( void )
{
	int *p;
	int *p1;
	int *p2;

	printf("case 6:\n");
	p = (int*)gc_malloc( sizeof(int) );
	p1 = (int*)gc_malloc( sizeof(int) );
	p2 = (int*)gc_malloc( sizeof(int) );

	*p = 0x11;
	*p1 = 0x22;
	*p2 = 0x33;

	p2 = p;
	p = p1;
	p1 = p2;
	
	p = 0;
	gc_collect();
	printf("\n");
}

// --------------------------------------------------------------------------
// test case for large memory blocks
void case7( void )
{
	int *p;

	printf("case 7:\n");
	p = (int*)gc_malloc( sizeof(int)*(10*MEGABYTE) );
	p = 0;
	gc_collect();
	printf("\n");
}

// --------------------------------------------------------------------------
// test case for false pointers
void case8( void )
{
	// @TODO this case is not solved
	int *p;
	int t;

	printf("case 8\n");

	// Assume following scenario. p holds really big memory block size in MBs.
	// Application requests GC to create new memory block. GC tries to allocate it
	// but realizes that there is no more memory available. Executes garbage collection
	// step. p which holds size in MBs is not used anymore but coincidentally
	// variable t which holds a random 32bit integer value holds the same value as
	// p. Pointer identification passes in GC, passes t as pointer actually which is not.
	// GC phase should release memory block of p in reality but t confuses it and
	// memory block still stands active as a space leak.
	p = (int*)gc_malloc( sizeof(int) );
	t = (int)p;

	p = 0;
	gc_collect();
	printf("\n");
}


// --------------------------------------------------------------------------
// multi dimensional array test
void case9( void )
{
	int **p;
	int i;
	int limit;

	printf("case 9\n");
	limit = 10;
	p = (int**)gc_malloc( sizeof(int*)*limit );
	for( i = 0; i<limit; i++ )
	{
		p[i] = (int*)gc_malloc( sizeof(int)*500 );
	}

	// break the start address of the array
	p = 0;

	gc_collect();
	printf("\n");
}


// --------------------------------------------------------------------------
// first stress test. this function creates reasonable amount of pointers and
// allocates space and releases it
void stress1( void )
{
	int **p;
	int i;
	int limit;
	DWORD t;

	// notes about this test.
	//
	// gc_malloc and malloc should take almost same time to work. gc_malloc does not
	// add additional overhead on runtime. contrary gc_collect should work much more
	// slower than free. the main reason is lies on marking phase of the gc algorithm
	// while on mark step, gc traces through all available pointers which we have
	// 1M. this means we have at least 1M loops. at the same time it gc has pointer 
	// identification algorithm which also traces through allocated heap which 
	// is 1M cycles too. at worst case we have at most 1M*1M running
	// complexity. 
	//
	// @TODO pointer identification algorithm can be improved
	// @TODO marking phase can be improved

	printf("stress test started\n");
	limit = 1000000;

	// allocation test with gc_malloc
	t = GetTickCount();
	printf("allocation started with gc_malloc\n");
	p = (int**)gc_malloc( sizeof(int*)*limit );
	for( i = 0; i<limit; i++ )
	{
		p[i] = (int*)gc_malloc( sizeof(int) );
	}
	printf("allocation completed. time:%d ms\n", GetTickCount() - t);

	t = GetTickCount();
	printf("garbage collection started\n");
	p = 0;
	gc_collect();
	printf("garbage collection completed. time: %d ms\n", GetTickCount() - t);


	// same allocation with malloc
	t = GetTickCount();
	printf("allocation started with malloc\n");
	p = (int**)malloc( sizeof(int*)*limit );
	for( i = 0; i<limit; i++ )
	{
		p[i] = (int*)malloc( sizeof(int) );
	}
	printf("allocation completed. time:%d ms\n", GetTickCount() - t);

	t = GetTickCount();
	printf("release started\n");
	for( i = 0; i<limit; i++ )
	{
		free( p[i] );
	}
	free( p );
	printf("release completed. time: %d ms\n", GetTickCount() - t);
	printf("stress test completed.\n");

}



// --------------------------------------------------------------------------
void run_test_cases( void )
{
	printf("executing test cases\n");
	case1();
	case2();
	case3();
	case4();
	case5();
	case6();
	case7();
	case8();
	case9();
	
	//stress1();
	printf("test cases executed\n");
}