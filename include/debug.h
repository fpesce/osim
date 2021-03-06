/*
 * Copyright (C) 2007 François Pesce : francois.pesce (at) gmail (dot) com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DEBUG_H
#define DEBUG_H
/**
 * @file debug.h
 * @brief UTIL debug output macros
 */

#include <apr_portable.h>
#include <stdio.h>

/**
 * Display error message at the level error.
 * @param str The format of the string.
 * @param arg The arguments to use while printing the data.
 */
#define DEBUG_ERR(str, arg...) fprintf(stderr, "[DEBUG_ERR][Thread:%lu]: [%s] " str " at line %d\n", (unsigned long) apr_os_thread_current(), __FUNCTION__, ## arg, __LINE__), fflush(stderr)

/**
 * Display error message at the level debug.
 * @param str The format of the string.
 * @param arg The arguments to use while printing the data.
 */
#define DEBUG_DBG(str, arg...) fprintf(stderr, "[DEBUG_DBG][Thread:%lu]: [%s] " str " at line %d\n", (unsigned long) apr_os_thread_current(), __FUNCTION__, ## arg, __LINE__), fflush(stderr)

#include <sys/time.h>
#include <sys/resource.h>

#define GET_MEMUSAGE(size)						\
{									\
    FILE *statm=fopen("/proc/self/statm", "r");				\
    if (statm) {							\
	if (1 != fscanf(statm, "%lu",&size)) {				\
	    DEBUG_ERR("Can't fscan proc self statm");			\
	}								\
	if (fclose(statm)) {						\
	    DEBUG_ERR("Can't close proc self statm");			\
	}								\
    } else {								\
	DEBUG_ERR("Can't open proc self statm");			\
    }									\
}

#define DISPLAY_MEMLEAK_open()						\
{									\
    unsigned long int size1, size2;					\
    GET_MEMUSAGE(size1);

#define DISPLAY_MEMLEAK_close()			\
    GET_MEMUSAGE(size2);				\
    if(size2 != size1)					\
        DEBUG_ERR("Delta memory %lu", size2 - size1);	\
}

/** @} */

#endif /* DEBUG_H */


