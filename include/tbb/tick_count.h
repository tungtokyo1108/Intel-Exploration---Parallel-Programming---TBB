/*
 * tick_count.h
 *
 *  Created on: Oct 26, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_TICK_COUNT_H_
#define INCLUDE_TBB_TICK_COUNT_H_

#include "tbb_stddef.h"

#if _WIN32||_WIN64
#include "machine/windows_api.h"
#elif __linux__
#include <ctime>
#else
#include <sys/time.h>
#endif



#endif /* INCLUDE_TBB_TICK_COUNT_H_ */
