/**
 * @file
 * @brief C header of the ofs (octagram filesystem, outlandish filesystem, or
 *        odd filesystem) kernel module
 * @author Octagram Sun <octagram@qq.com>
 * @version 0.1.0
 * @date 2015
 * @copyright Octagram Sun <octagram@qq.com>
 *
 * @note
 * C header about logs of ofs. 1 tab == 8 spaces.
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
