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
 * Source about kernel module of ofs
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

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********    header files   ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#include "ofs.h"
#include "fs.h"
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include "log.h"
#include "ksym.h"

#ifdef CONFIG_OFS_SYSFS
#include "omsys.h"
#endif
#ifdef CONFIG_OFS_DEBUGFS
#include "debugfs.h"
#endif

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       macro       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#define SHOW_MACRO_HELPER(x)	#x
#define SHOW_MACRO(x)		#x":"SHOW_MACRO_HELPER(x)

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       types       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********         function prototypes         ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********   .sdata & .data  ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** module parameter ******** ********/
int hashtable_size_bits = 10;
module_param(hashtable_size_bits, uint, S_IRUGO);
MODULE_PARM_DESC(hashtable_size_bits,
	"When creating hashtable, the size is (1 << hashtable_size_bits).");

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Octagram Sun");

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********      function implementations       ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** module init & exit ******** ********/
/**
 * @brief module init
 * @retval 0: OK
 * @retval errno: Indicate the error code if failed.
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 */
static __init
int ofs_init(void)
{
	int rc;
	unsigned int size;

	rc = ofs_load_ksym();
	if (rc) {
		ofs_err("Failed in ofs_load_ksym() errno:%d\n", rc);
		return rc;
	}

	ofs_inode_cache = kmem_cache_create("ofs_inode_cache",
					    sizeof(struct ofs_inode),
					    0,
					    SLAB_PANIC,
					    ofs_inode_init_once);
	if (IS_ERR_OR_NULL(ofs_inode_cache)) {
		rc = -ENOMEM;
		ofs_err("Can't create ofs_inode_cache.\n");
		goto out_return;
	}

	ofs_file_cache = kmem_cache_create("ofs_file_cache",
					   sizeof(struct ofs_file),
					   0,
					   SLAB_PANIC,
					   ofs_file_init_once);
	if (IS_ERR_OR_NULL(ofs_file_cache)) {
		rc = -ENOMEM;
		ofs_err("Can't create ofs_file_cache.\n");
		goto out_inode_cache_destory;
	}

	rc = bdi_init(&ofs_backing_dev_info);
	if (rc) {
		ofs_err("Could not init bdi! errno:%d\n", rc);
		goto out_file_cache_destroy;
	}

	if (hashtable_size_bits < OFS_HASHTABLE_SIZE_BITS_MIN)
		hashtable_size_bits = OFS_HASHTABLE_SIZE_BITS_MIN;
	else if (hashtable_size_bits > OFS_HASHTABLE_SIZE_BITS_MAX)
		hashtable_size_bits = OFS_HASHTABLE_SIZE_BITS_MAX;
	size = 1 << hashtable_size_bits;
	ofs_rbtrees = kmalloc((sizeof(struct ofs_rbtree)) * size, GFP_KERNEL);
	if (IS_ERR_OR_NULL(ofs_rbtrees)) {
		ofs_rbtrees = NULL;
		rc = -ENOMEM;
		ofs_err("Could kmalloc rbtree hashtable!\n");
		goto out_bdi_destroy;
	}
	for (rc = 0; rc < size; rc++) {
		ofs_rbtrees[rc].root = RB_ROOT;
		rwlock_init(&ofs_rbtrees[rc].lock);
	}

	rc = register_filesystem(&ofs_fstype);
	if (rc) {
		ofs_err("Could not register ofs! errno:%d\n", rc);
		goto out_kfree;
	}
	ofs_dbg("sizeof(inode):%d; sizeof(ofs_inode):%d\n",
		(int)sizeof(struct inode), (int)sizeof(struct ofs_inode));
	ofs_inf("register filesystem: %s.\n", ofs_fstype.name);

#ifdef CONFIG_OFS_SYSFS
	rc = ofs_construct_sysfs();
	if (rc) {
		ofs_err("Failed to ofs_construct_sysfs()! errno:%d\n", rc);
		goto out_unregfs;
	}
#endif

#ifdef CONFIG_OFS_DEBUGFS
	rc = ofs_debugfs_init();
	if (rc) {
		ofs_err("Failed to ofs_construct_debugfs()! errno:%d\n", rc);
		goto out_destruct_sysfs;
	}
#endif

	return 0;

#ifdef CONFIG_OFS_DEBUGFS
out_destruct_sysfs:
#endif
#ifdef CONFIG_OFS_SYSFS
	ofs_destruct_sysfs();
#endif
out_unregfs:
	unregister_filesystem(&ofs_fstype);
out_kfree:
	kfree(ofs_rbtrees);
	ofs_rbtrees = NULL;
out_bdi_destroy:
	bdi_destroy(&ofs_backing_dev_info);
out_file_cache_destroy:
	kmem_cache_destroy(ofs_file_cache);
out_inode_cache_destory:
	kmem_cache_destroy(ofs_inode_cache);
out_return:
	return rc;
}
module_init(ofs_init);

/**
 * @brief module exit
 */
static __exit
void ofs_exit(void)
{
#ifdef CONFIG_OFS_DEBUGFS
	ofs_debugfs_exit();
#endif
#ifdef CONFIG_OFS_SYSFS
	ofs_destruct_sysfs();
#endif
	unregister_filesystem(&ofs_fstype);
	kfree(ofs_rbtrees);
	ofs_rbtrees = NULL;
	bdi_destroy(&ofs_backing_dev_info);
	kmem_cache_destroy(ofs_file_cache);
	kmem_cache_destroy(ofs_inode_cache);
	ofs_inf("unregister filesystem: %s\n", ofs_fstype.name);
}
module_exit(ofs_exit);
