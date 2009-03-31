// scanlib.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "scanlib.h"


// This is an example of an exported variable
SCANLIB_API int nscanlib=0;

// This is an example of an exported function.
SCANLIB_API int fnscanlib(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see scanlib.h for the class definition
Cscanlib::Cscanlib()
{
	return;
}
