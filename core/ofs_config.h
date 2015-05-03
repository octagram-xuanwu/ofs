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
 * All configure definations are here. 1 tab == 8 spaces.
 */

#ifndef __OFS_CONFIG_H__
#define __OFS_CONFIG_H__

#define CONFIG_OFS			1 /**< ofs */

#define CONFIG_OFS_DEBUG_LOG		1
	/**< Set to 1 to enable debugging log, otherwise comment the line. */

#define CONFIG_OFS_CHKPARAM		1
	/**< build apis with safe mode. The apis will check the parameters
	 *   in safe mode. Set to 1 to enable safe api mode, otherwise
	 *    comment the line
	 */

#define CONFIG_OFS_SYSFS		1 /**< /sysfs/fs/ofs entry */

#define CONFIG_OFS_DEBUGFS		1 /**< debug interface in debugfs */

#endif /* ofs_config.h */
