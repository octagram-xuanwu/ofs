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
 * C source about regular file.
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

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********    header files   ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#include "ofs.h"
#include "fs.h"
#include "log.h"
#include "regfile.h"

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       macro       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       types       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********         function prototypes         ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********   .sdata & .data  ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** regular file operations ******** ********/
/* inode_operations */
const struct inode_operations ofs_regfile_iops = {
	.setattr = ofs_regfile_iops_setattr,
	.getattr = ofs_regfile_iops_getattr,
};

/* file_operations */
const struct file_operations ofs_regfile_fops = {
	.owner = THIS_MODULE,
	.llseek = ofs_regfile_fops_llseek,
	.read = ofs_regfile_fops_read,
	.write = ofs_regfile_fops_write,
	.read_iter = ofs_regfile_fops_read_iter,
	.write_iter = ofs_regfile_fops_write_iter,
	.mmap = ofs_regfile_fops_mmap,
	.open = ofs_regfile_fops_open,
	.release = ofs_regfile_fops_release,
	.fsync = ofs_regfile_fops_fsync,
	.splice_read = ofs_regfile_fops_splice_read,
	.splice_write = ofs_regfile_fops_splice_write,
};

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********      function implementations       ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** regular file operations ******** ********/
/* inode_operations */
int ofs_regfile_iops_setattr(struct dentry *dentry, struct iattr *iattr)
{
//	ofs_dbg("dentry<%p>; iattr<%p>;\n", dentry, iattr);
	return simple_setattr(dentry, iattr);
}

int ofs_regfile_iops_getattr(struct vfsmount *mnt, struct dentry *dentry,
			     struct kstat *stat)
{
//	ofs_dbg("mnt<%p>; dentry<%p>; stat<%p>;\n", mnt, dentry, stat);
	return simple_getattr(mnt, dentry, stat);
}

/* file_operations */
loff_t ofs_regfile_fops_llseek(struct file *file, loff_t offset, int whence)
{
	ofs_dbg("file<%p>; dentry<%p>==\"%pd\"; offset==%lld; whence==%d;\n",
		file, file->f_path.dentry, file->f_path.dentry, offset, whence);
	return generic_file_llseek(file, offset, whence);
}

ssize_t ofs_regfile_fops_read(struct file *file, char __user *buf,
			      size_t len, loff_t *pos)
{
	struct ofs_inode *oi = FILE_TO_OFS_INODE(file);

	ofs_dbg("oi<%p>, file<%p>; dentry<%p>==\"%pd\"; "
		"buf<%p>; len==%zu; pos<%p>==%lld;\n",
		oi, file, file->f_path.dentry, file->f_path.dentry,
		buf, len, pos, *pos);
	return new_sync_read(file, buf, len, pos);
}

ssize_t ofs_regfile_fops_write(struct file *file, const char __user *buf,
			       size_t len, loff_t *pos)
{
	struct ofs_inode *oi = FILE_TO_OFS_INODE(file);

	ofs_dbg("oi<%p>, file<%p>; dentry<%p>==\"%pd\"; "
		"buf<%p>; len==%zu; pos<%p>==%lld;\n",
		oi, file, file->f_path.dentry, file->f_path.dentry,
		buf, len, pos, *pos);
	return new_sync_write(file, buf, len, pos);
}

ssize_t ofs_regfile_fops_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	struct ofs_inode *oi = FILE_TO_OFS_INODE(iocb->ki_filp);

	ofs_dbg("oi<%p>; iocb<%p>; dentry<%p>==\"%pd\"; iter<%p>\n",
		oi, iocb, iocb->ki_filp->f_path.dentry,
		iocb->ki_filp->f_path.dentry, iter);
	return generic_file_read_iter(iocb, iter);
}

ssize_t ofs_regfile_fops_write_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	struct ofs_inode *oi = FILE_TO_OFS_INODE(iocb->ki_filp);

	ofs_dbg("oi<%p>; iocb<%p>; dentry<%p>=\"%pd\"; iter<%p>\n",
		oi, iocb, iocb->ki_filp->f_path.dentry,
		iocb->ki_filp->f_path.dentry, iter);
	return generic_file_write_iter(iocb, iter);
}

int ofs_regfile_fops_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct ofs_inode *oi = FILE_TO_OFS_INODE(file);

	ofs_dbg("oi<%p>; file<%p>; dentry<%p>==\"%pd\"; vma<%p>;\n",
		oi, file, file->f_path.dentry, file->f_path.dentry, vma);
	return generic_file_mmap(file, vma);
}

int ofs_regfile_fops_open(struct inode *inode, struct file *file)
{
	struct ofs_inode *oi = OFS_INODE(inode);

	ofs_dbg("oi<%p>; file<%p>; dentry<%p>==\"%pd\";\n",
		oi, file, file->f_path.dentry, file->f_path.dentry);
	return 0;
}

int ofs_regfile_fops_release(struct inode *inode, struct file *file)
{
	struct ofs_inode *oi = OFS_INODE(inode);

	ofs_dbg("oi<%p>; file<%p>; dentry<%p>==\"%pd\";\n",
		oi, file, file->f_path.dentry, file->f_path.dentry);
	return 0;
}

int ofs_regfile_fops_fsync(struct file *file, loff_t start,
			   loff_t end, int dsync)
{
	return noop_fsync(file, start, end, dsync);
}

ssize_t ofs_regfile_fops_splice_read(struct file *in, loff_t *pos,
				     struct pipe_inode_info *pipe,
				     size_t len, unsigned int flags)
{
	return generic_file_splice_read(in, pos, pipe, len, flags);
}

ssize_t ofs_regfile_fops_splice_write(struct pipe_inode_info *pipe,
				      struct file *out, loff_t *pos,
				      size_t len, unsigned int flags)
{
	return iter_file_splice_write(pipe, out, pos, len, flags);
}

