/**
 * @file
 * @brief C source for the ofs (octagram filesystem, outlandish filesystem, or
 *        odd filesystem) kernel module
 * @author Octagram Sun <octagram@qq.com>
 * @version 0.1.0
 * @date 2015
 * @copyright Octagram Sun <octagram@qq.com>
 *
 * @note
 * C source to load address of kernel symbols. 1 tab == 8 spaces.
 */

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********    header files   ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#include "ofs.h"
#include <linux/kallsyms.h>
#include "log.h"
#include "ksym.h"

#ifdef CONFIG_OFS_SYSFS
#include "omsys.h"
#endif

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       macro       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#define KSYM_LOAD(sym)							\
	do {								\
		__ksym__##sym =						\
			(typeof(sym) *)kallsyms_lookup_name(#sym);	\
		if (IS_ERR_OR_NULL(__ksym__##sym)) {			\
			ofs_dbg("Can't load ksym:\""#sym"\"\n");	\
			return PTR_ERR(__ksym__##sym);			\
		}							\
	} while (0)

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       types       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********         function prototypes         ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********   .sdata & .data  ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
KSYM(security_inode_unlink);

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********      function implementations       ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
int ofs_load_ksym(void)
{
	KSYM_LOAD(security_inode_unlink);
	return 0;
}

