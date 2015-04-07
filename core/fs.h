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
 * C header about filesystem of ofs
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

#ifndef __OFS_FS_H__
#define __OFS_FS_H__

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********    header files   ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/vfs.h>
#include <linux/dcache.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/file.h>
#include <linux/limits.h>
#include <linux/fsnotify.h>
#include <linux/security.h>
#include <linux/backing-dev.h>
#include <linux/aio.h>
#include <linux/pagemap.h>
#include <linux/spinlock.h>
#include <linux/seqlock.h>
#include <linux/rbtree.h>
#include <linux/slab.h>
#include <linux/audit.h>
#include <linux/fs_struct.h>
#include <linux/percpu-defs.h>
#include <linux/hash.h>
#include <linux/string.h>
#include <asm/uaccess.h>

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       macro       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** hash rbtree ******** ********/
/**
 * @brief The max hashtable size is (1 << OFS_HASHTABLE_SIZE_BITS_MAX).
 */
#define OFS_HASHTABLE_SIZE_BITS_MAX	16

/**
 * @brief The min hashtable size is (1 << OFS_HASHTABLE_SIZE_BITS_MIN).
 */
#define OFS_HASHTABLE_SIZE_BITS_MIN	L1_CACHE_SHIFT

#define ofsops_get(ofsops) \
	(((ofsops) && try_module_get((ofsops)->owner) ? (ofsops) : NULL))

#define ofsops_put(ofsops) \
	do { if (ofsops) module_put((ofsops)->owner); } while(0)

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       types       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********         function prototypes         ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** red-black tree ******** ********/
extern
struct ofs_inode *ofs_rbtree_lookup(struct ofs_rbtree *tree, const oid_t oid);

extern
void ofs_rbtree_oiput(struct ofs_inode *oi);

extern
bool ofs_rbtree_insert(struct ofs_rbtree *tree, struct ofs_inode *newoi);

extern
void ofs_rbtree_remove(struct ofs_rbtree *tree, struct ofs_inode *oi);

extern
struct ofs_rbtree *ofs_get_rbtree(struct ofs_inode *oi);

extern
struct ofs_rbtree *ofs_get_rbtree_by_oid(const oid_t oid);

/******** ******** fs ******** ********/
extern
struct inode *ofs_new_inode(struct super_block *sb, const struct inode *iparent,
			    umode_t mode, dev_t dev, bool is_magic);

extern
void ofs_inode_init_once(void *data);

extern
void ofs_file_init_once(void *data);

extern
int ofs_test_super(struct super_block *sb, void *magic);

extern
int ofs_set_super(struct super_block *sb, void *data);

/******** ******** vfs ******** ********/
extern
int vfs_path_lookup(struct dentry *, struct vfsmount *, const char *,
		    unsigned int, struct path *);

static __always_inline
int ofs_simple_positive(struct dentry *dentry)
{
	return dentry->d_inode && !d_unhashed(dentry);
}

static inline
void dentry_rcuwalk_barrier(struct dentry *dentry)
{
	assert_spin_locked(&dentry->d_lock);
	/* Go through a barrier */
	write_seqcount_barrier(&dentry->d_seq);
}

/******** ******** internal helper functions ******** ********/
extern
int ofs_rmdir_normal_hlp(struct ofs_inode *oiparent, struct path *pathp,
			 struct dentry *dentry);

extern
int ofs_unlink_normal_hlp(struct ofs_inode *oiparent, struct path *pathp,
			  struct dentry *dentry);

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********   .sdata & .data  ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** default operations ******** ********/
extern const struct inode_operations ofs_dir_iops;
extern const struct file_operations ofs_dir_fops;
extern const struct ofs_operations ofs_singularity_ofsops;
extern const struct inode_operations ofs_singularity_iops;
extern const struct file_operations ofs_singularity_fops;
extern const struct inode_operations ofs_regfile_iops;
extern const struct file_operations ofs_regfile_fops;
extern const struct inode_operations ofs_symlink_iops;

/******** ******** fs ******** ********/
extern struct kmem_cache *ofs_inode_cache;
extern struct kmem_cache *ofs_file_cache;
extern struct backing_dev_info ofs_backing_dev_info;
extern struct ofs_rbtree *ofs_rbtrees;
extern struct file_system_type ofs_fstype;

/******** ******** dentry_operations ******** ********/
extern const struct dentry_operations ofs_dops;

#endif /* fs.h */
