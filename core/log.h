/**
 * @file
 * @brief C header for the ofs (octagram filesystem, outlandish filesystem, or
 *        odd filesystem) kernel module
 * @author Octagram Sun <octagram@qq.com>
 * @version 0.1.0
 * @date 2015
 * @copyright Octagram Sun <octagram@qq.com>
 *
 * @note
 * C header about logs of ofs
 * @note
 * This file is part of ofs, as available from\n
 * * https://gitcafe.com/octagram/ofs\n
 * * https://github.com/octagram-xuanwu/ofs\n
 * @note
 * This file is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License (GPL) as published by the Free
 * Software Foundation, in version 2. The ofs is distributed in the hope
 * that it will be useful, but <b>WITHOUT ANY WARRANTY</b> of any kind.
 */

#ifndef __OFS_LOG_H__
#define __OFS_LOG_H__

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********    header files   ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#include <linux/sched.h>
#include <linux/printk.h>

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       macro       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** logs ******** ********/
/**
 * @brief critical log
 * @param fmt: format string
 * @param x: arglist to format
 */
#define ofs_crt(fmt, x...)  printk(KERN_CRIT "[OFS CRT](tgid:%d) <%s> " \
				   fmt, task_tgid_vnr(current), __func__, ##x)

/**
 * @brief infomation log
 * @param fmt: format string
 * @param x: arglist to format
 */
#define ofs_inf(fmt, x...)  printk(KERN_INFO "[OFS INF](tgid:%d) <%s> " \
				   fmt, task_tgid_vnr(current), __func__, ##x)

/**
 * @brief error log
 * @param fmt: format string
 * @param x: arglist to format
 */
#define ofs_err(fmt, x...)  printk(KERN_ERR "[OFS ERR](tgid:%d) <%s> " \
				   fmt, task_tgid_vnr(current), __func__, ##x)

/**
 * @brief warning log
 * @param fmt: format string
 * @param x: arglist to format
 */
#define ofs_wrn(fmt, x...)  printk(KERN_WARNING "[OFS WRN](tgid:%d) <%s> " \
				   fmt, task_tgid_vnr(current), __func__, ##x)

#ifdef CONFIG_OFS_DEBUG_LOG
/**
 * @brief debugging log
 * @param fmt: format string
 * @param x: arglist to format
 */
#define ofs_dbg(fmt, x...)  printk(KERN_DEBUG "[OFS DBG](tgid:%d) <%s> " \
				   fmt, task_tgid_vnr(current), __func__, ##x)
#else
/**
 * @brief debugging log
 * @param fmt: format string
 * @param x: arglist to format
 */
#define ofs_dbg(fmt, x...)
#endif

#endif /* log.h */
