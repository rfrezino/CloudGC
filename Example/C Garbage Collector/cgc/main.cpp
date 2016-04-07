// --------------------------------------------------------------------------
// Author: Yasin HINISLIOÐLU
//
// 2008 - Ankara, TURKEY
// --------------------------------------------------------------------------
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <windows.h>

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>


#include "cgc.h"
#include "cgc_test.h"


// --------------------------------------------------------------------------
void on_exit( void )
{
	_CrtDumpMemoryLeaks();
}


// --------------------------------------------------------------------------
int main( void )
{
	atexit( on_exit );

	run_test_cases();

	_getch();
	return 0;
}