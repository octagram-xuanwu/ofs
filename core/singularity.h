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
 * C header about singularity, is used in ofs internal.
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

#ifndef __OFS_SINGULARITY_H__
#define __OFS_SINGULARITY_H__

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

/******** ******** magic file operations ******** ********/
/******** default ofs_operations ********/
extern
ssize_t ofs_singularity_ofsops_read(struct file *file, char __user *buf,
				    size_t len, loff_t *pos);

extern
ssize_t ofs_singularity_ofsops_read_iter(struct kiocb *iocb,
					 struct iov_iter *iter);

extern
ssize_t ofs_singularity_ofsops_write(struct file *file, const char __user *buf,
				     size_t len, loff_t *pos);

extern
ssize_t ofs_singularity_ofsops_write_iter(struct kiocb *iocb,
					  struct iov_iter *iter);

extern
int ofs_singularity_ofsops_mmap(struct file *file, struct vm_area_struct *vma);

/******** inode_operations ********/

/******** file_operations ********/
extern
int ofs_singularity_fops_open(struct inode *inode, struct file *file);

extern
int ofs_singularity_fops_release(struct inode *inode, struct file *file);

extern
loff_t ofs_singularity_fops_llseek(struct file *file, loff_t offset,
				   int whence);

extern
int ofs_singularity_fops_iterate(struct file *file, struct dir_context *ctx);

extern
int ofs_singularity_fops_fsync(struct file *file, loff_t start, loff_t end,
			       int dsync);

extern
int ofs_singularity_fops_mmap(struct file *file, struct vm_area_struct *vma);

extern
long ofs_singularity_fops_unlocked_ioctl(struct file *file, unsigned int cmd,
					 unsigned long args);

extern
ssize_t ofs_singularity_fops_write(struct file *file, const char __user *buf,
				   size_t len, loff_t *pos);

extern
ssize_t ofs_singularity_fops_write_iter(struct kiocb *iocb,
					struct iov_iter *iter);

extern
ssize_t ofs_singularity_fops_read(struct file *file, char __user *buf,
				  size_t len, loff_t *pos);

extern
ssize_t ofs_singularity_fops_read_iter(struct kiocb *iocb,
				       struct iov_iter *iter);

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********   .sdata & .data  ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

#endif /* singularity.h */
