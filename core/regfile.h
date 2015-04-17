/**
 * @file
 * @brief C source of the ofs (octagram filesystem, outlandish filesystem, or
 *        odd filesystem) kernel module
 * @author Octagram Sun <octagram@qq.com>
 * @version 0.1.0
 * @date 2015
 * @copyright Octagram Sun <octagram@qq.com>
 *
 * @note
 * C header about regular file, is used in ofs internal.
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
#ifndef __OFS_REGFILE_H__
#define __OFS_REGFILE_H__

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

/******** ******** regular file operations ******** ********/
/* inode_operations */
extern
int ofs_regfile_iops_setattr(struct dentry *dentry, struct iattr *iattr);

extern
int ofs_regfile_iops_getattr(struct vfsmount *mnt, struct dentry *dentry,
			     struct kstat *stat);

/* file_operations */
extern
loff_t ofs_regfile_fops_llseek(struct file *file, loff_t offset, int whence);

extern
ssize_t ofs_regfile_fops_read(struct file *file, char __user *buf,
			      size_t len, loff_t *pos);

extern
ssize_t ofs_regfile_fops_write(struct file *file, const char __user *buf,
			       size_t len, loff_t *pos);

extern
ssize_t ofs_regfile_fops_read_iter(struct kiocb *iocb, struct iov_iter *iter);

extern
ssize_t ofs_regfile_fops_write_iter(struct kiocb *iocb, struct iov_iter *iter);

extern
int ofs_regfile_fops_mmap(struct file *file, struct vm_area_struct *vma);

extern
int ofs_regfile_fops_open(struct inode *inode, struct file *file);

extern
int ofs_regfile_fops_release(struct inode *inode, struct file *file);

extern
int ofs_regfile_fops_fsync(struct file *file, loff_t start,
			   loff_t end, int dsync);

extern
ssize_t ofs_regfile_fops_splice_read(struct file *in, loff_t *pos,
				     struct pipe_inode_info *pipe,
				     size_t len, unsigned int flags);

extern
ssize_t ofs_regfile_fops_splice_write(struct pipe_inode_info *pipe,
				      struct file *out, loff_t *pos,
				      size_t len, unsigned int flags);

#endif /* regfile.h */
