#ifndef _DEBUG_LOG_H_
#define _DEBUG_LOG_H_

#include <stdio.h>     /* for printf */

extern int debug_log;

#define DEBUG_LOG(...) {if(debug_log) printf( __VA_ARGS__);}

#endif
