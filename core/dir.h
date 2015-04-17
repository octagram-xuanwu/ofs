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
 * C header about directory, is used in ofs internal.
 * @note
 * This file is a part of ofs, as available from\n
 * * https://gitcafe.com/octagram/ofs\n
 * * https://github.com/octagram-xuanwu/ofs\n
 * @note
 * This file is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License (GPL) as published by the Free
 * Software Foundation, in version 2. The ofs is distributed in the hope
 * that it will be useful, but <b>WITHOUT ANY WARRANTY</b> of any kind.
 */

#ifndef __OFS_DIR_H__
#define __OFS_DIR_H__

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********    header files   ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       macro       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       types       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********         function prototypes         ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** directory operations ******** ********/
/******** inode_operations ********/
extern
struct dentry *ofs_dir_iops_lookup(struct inode *iparent, struct dentry *dentry,
				   unsigned int flags);

extern
int ofs_dir_iops_create(struct inode *iparent, struct dentry *newd,
			umode_t mode, bool excl);

extern
int ofs_dir_iops_permission(struct inode *inode, int mask);

extern
int ofs_dir_iops_link(struct dentry *oldd, struct inode *iparent,
		      struct dentry *newd);

extern
int ofs_dir_iops_unlink(struct inode *iparent, struct dentry *dentry);

extern
int ofs_dir_iops_symlink(struct inode *iparent, struct dentry *newd,
			 const char *symname);

extern
int ofs_dir_iops_mkdir(struct inode *iparent, struct dentry *newd,
		       umode_t mode);

extern
int ofs_dir_iops_rmdir(struct inode *iparent, struct dentry *dentry);

extern
int ofs_dir_iops_mknod(struct inode *iparent, struct dentry *newd,
		       umode_t mode, dev_t dev);

extern
int ofs_dir_iops_rename2(struct inode *ioldparent, struct dentry *dold,
			 struct inode *inewparent, struct dentry *newd,
			 unsigned int flags);

extern
int ofs_dir_iops_setattr(struct dentry *dentry, struct iattr *iattr);

extern
int ofs_dir_iops_getattr(struct vfsmount *mnt, struct dentry *dentry,
			 struct kstat *stat);

/******** file_operations ********/
extern
loff_t ofs_dir_fops_llseek(struct file *file, loff_t offset, int whence);

extern
ssize_t ofs_dir_fops_read(struct file *file, char __user * buf, size_t len,
			  loff_t *pos);

extern
ssize_t ofs_dir_fops_write(struct file *file, const char __user * buf,
			   size_t len, loff_t *pos);

extern
ssize_t ofs_dir_fops_read_iter(struct kiocb *iocb, struct iov_iter *iter);

extern
int ofs_dir_fops_iterate(struct file *file, struct dir_context *ctx);

extern
unsigned int ofs_dir_fops_poll(struct file *file,
			       struct poll_table_struct *pts);

extern
long ofs_dir_fops_unlocked_ioctl(struct file *file, unsigned int cmd,
				 unsigned long args);

extern
long ofs_dir_fops_compat_ioctl(struct file *file, unsigned int cmd,
			       unsigned long args);

extern
int ofs_dir_fops_mmap(struct file *file, struct vm_area_struct *vma);

extern
int ofs_dir_fops_open(struct inode *inode, struct file *file);

extern
int ofs_dir_fops_release(struct inode *inode, struct file *file);

extern
int ofs_dir_fops_fsync(struct file *file, loff_t start, loff_t end, int dsync);

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********   .sdata & .data  ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

#endif /* dir.h */
