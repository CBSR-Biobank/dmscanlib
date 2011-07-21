/*
 * main.c
 *
 *  Created on: 2011-07-18
 *      Author: thomas
 */
#include "TestApp.h"

int main(int argc, char ** argv) {
/*
	#if defined(_VISUALC_) && defined(_DEBUG)
		_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif
*/
    TestApp app(argc, argv);

    return 0;
}


