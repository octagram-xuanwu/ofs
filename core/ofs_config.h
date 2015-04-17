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
 * All configure definations are here.
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
