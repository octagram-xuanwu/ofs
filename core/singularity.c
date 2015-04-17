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
 * C source about singularity
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
#include "dir.h"
#include "regfile.h"
#include "singularity.h"

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
/******** ******** singularity operations ******** ********/
/******** default ofs_operations ********/
const struct ofs_operations ofs_singularity_ofsops = {
	.owner = THIS_MODULE,
	.read = ofs_singularity_ofsops_read,
	.write = ofs_singularity_ofsops_write,
	.read_iter = ofs_singularity_ofsops_read_iter,
	.write_iter = ofs_singularity_ofsops_write_iter,
	.mmap = ofs_singularity_ofsops_mmap,
};

/******** inode_operations ********/
const struct inode_operations ofs_singularity_iops = {
	.lookup = ofs_dir_iops_lookup,
	.permission = ofs_dir_iops_permission,
	.create = ofs_dir_iops_create,
	.link = ofs_dir_iops_link,
	.unlink = ofs_dir_iops_unlink,
	.symlink = ofs_dir_iops_symlink,
	.mkdir = ofs_dir_iops_mkdir,
	.rmdir = ofs_dir_iops_rmdir,
	.mknod = ofs_dir_iops_mknod,
	.rename2 = ofs_dir_iops_rename2,
	.setattr = ofs_regfile_iops_setattr,
	.getattr = ofs_regfile_iops_getattr,
};

/******** file_operations ********/
const struct file_operations ofs_singularity_fops = {
	.owner = THIS_MODULE,
	.llseek = ofs_singularity_fops_llseek,
	.read = ofs_singularity_fops_read,
	.write = ofs_singularity_fops_write,
	.read_iter = ofs_singularity_fops_read_iter,
	.write_iter = ofs_singularity_fops_write_iter,
	.iterate = ofs_singularity_fops_iterate,
	.unlocked_ioctl = ofs_singularity_fops_unlocked_ioctl,
	.mmap = ofs_singularity_fops_mmap,
	.open = ofs_singularity_fops_open,
	.release = ofs_singularity_fops_release,
	.fsync = ofs_singularity_fops_fsync,
};

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********      function implementations       ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** singularity operations ******** ********/
/******** default ofs_operations ********/
ssize_t ofs_singularity_ofsops_read(struct file *file, char __user *buf,
				    size_t len, loff_t *pos)
{
	ofs_dbg("file<%p>; dentry<%p>==\"%pd\"; "
		"buf<%p>; len==%zu; pos<%p>==%lld;\n",
		file, file->f_path.dentry, file->f_path.dentry,
		buf, len, pos, *pos);
	return new_sync_read(file, buf, len, pos);
}

ssize_t ofs_singularity_ofsops_read_iter(struct kiocb *iocb,
					 struct iov_iter *iter)
{
	ofs_dbg("iocb<%p>; dentry<%p>==\"%pd\"; iter<%p>\n",
		iocb, iocb->ki_filp->f_path.dentry,
		iocb->ki_filp->f_path.dentry, iter);
	return generic_file_read_iter(iocb, iter);
}

ssize_t ofs_singularity_ofsops_write(struct file *file, const char __user *buf,
				     size_t len, loff_t *pos)
{
	ofs_dbg("file<%p>; dentry<%p>==\"%pd\"; "
		"buf<%p>; len==%zu; pos<%p>==%lld;\n",
		file, file->f_path.dentry, file->f_path.dentry,
		buf, len, pos, *pos);
	return new_sync_write(file, buf, len, pos);
}

ssize_t ofs_singularity_ofsops_write_iter(struct kiocb *iocb,
					  struct iov_iter *iter)
{
	ofs_dbg("iocb<%p>; dentry<%p>=\"%pd\"; iter<%p>\n",
		iocb, iocb->ki_filp->f_path.dentry,
		iocb->ki_filp->f_path.dentry, iter);
	return generic_file_write_iter(iocb, iter);
}

int ofs_singularity_ofsops_mmap(struct file *file, struct vm_area_struct *vma)
{
	ofs_dbg("file<%p>; dentry<%p>==\"%pd\"; vma<%p>;\n",
		file, file->f_path.dentry, file->f_path.dentry, vma);
	return generic_file_mmap(file, vma);
}

/******** inode_operations ********/

/******** file_operations ********/
int ofs_singularity_fops_open(struct inode *inode, struct file *file)
{
	static struct qstr cursor_name = QSTR_INIT(".", 1);
	struct ofs_inode *oi = OFS_INODE(inode);
	struct ofs_file *of;
	int rc;

	ofs_dbg("oi<%p>; file<%p>; dentry<%p>==\"%pd\";\n",
		oi, file, file->f_path.dentry, file->f_path.dentry);
	
	of = kmem_cache_alloc(ofs_file_cache, GFP_KERNEL);
	if (IS_ERR_OR_NULL(of)) {
		rc = -ENOMEM;
		goto out_return;
	}
	of->cursor = d_alloc(file->f_path.dentry, &cursor_name);
	if (IS_ERR_OR_NULL(of->cursor)) {
		rc = -ENOMEM;
		goto out_cache_free;
	}
	ofs_dbg("entry: magic dentry, cursor<%p>\n", of->cursor);
	file->private_data = of;

	if (oi->ofsops) {
		ofsops_get(oi->ofsops);
		if (oi->ofsops->open) {
			rc = oi->ofsops->open(oi, file);
			if (rc) {
				goto out_ofsops_put;
			}
		}
	}
	return 0;

out_ofsops_put:
	ofsops_put(oi->ofsops);
	dput(of->cursor);
	of->cursor = NULL;
out_cache_free:
	kmem_cache_free(ofs_file_cache, of);
out_return:
	return rc;
}

int ofs_singularity_fops_release(struct inode *inode, struct file *file)
{
	struct ofs_inode *oi = OFS_INODE(inode);
	struct ofs_file *of;

	ofs_dbg("oi<%p>; file<%p>; dentry<%p>==\"%pd\";\n",
		oi, file, file->f_path.dentry, file->f_path.dentry);
	if (oi->ofsops) {
		if (oi->ofsops->release)
			oi->ofsops->release(oi, file);
		ofsops_put(oi->ofsops);
	}

	of = (struct ofs_file *)file->private_data;
	ofs_dbg("entry: magic dentry, cursor<%p>\n", of->cursor);
	dput(of->cursor);
	of->cursor = NULL;
	kmem_cache_free(ofs_file_cache, of);

	return 0;
}

static __always_inline
unsigned char ofs_dt_type(struct inode *inode)
{
	return (inode->i_mode >> 12) & 15;
}

loff_t ofs_singularity_fops_llseek(struct file *file, loff_t offset, int whence)
{
	struct dentry *dentry = file->f_path.dentry;
	struct ofs_inode *oi = OFS_INODE(dentry->d_inode);
	struct ofs_file *of = file->private_data;
	
	ofs_dbg("oi<%p>; file<%p>; dentry<%p>==\"%pd\"; "
		"offset==%lld; whence==%d;\n",
		oi, file, file->f_path.dentry, file->f_path.dentry,
		offset, whence);

	if (oi->magic != dentry) {
		if (oi->ofsops && oi->ofsops->llseek)
			return oi->ofsops->llseek(file, offset, whence);
		return -ENOSYS;
	}

	ofs_dbg("entry: magic dentry<%p>\n", of->cursor);
	mutex_lock(&oi->inode.i_mutex);
	switch (whence) {
		case 1:
			offset += file->f_pos;
		case 0:
			if (offset >= 0)
				break;
		default:
			mutex_unlock(&oi->inode.i_mutex);
			return -EINVAL;
	}
	if (offset != file->f_pos) {
		file->f_pos = offset;
		if (file->f_pos >= 2) {
			struct list_head *p;
			struct dentry *cursor = of->cursor;
			loff_t n = file->f_pos - 2;

			spin_lock(&dentry->d_lock);
			/* d_lock not required for cursor */
			list_del(&cursor->d_child);
			p = dentry->d_subdirs.next;
			while (n && p != &dentry->d_subdirs) {
				struct dentry *next;
				next = list_entry(p, struct dentry, d_child);
				spin_lock_nested(&next->d_lock,
						 DENTRY_D_LOCK_NESTED);
				if (ofs_simple_positive(next))
					n--;
				spin_unlock(&next->d_lock);
				p = p->next;
			}
			list_add_tail(&cursor->d_child, p);
			spin_unlock(&dentry->d_lock);
		}
	}
	mutex_unlock(&oi->inode.i_mutex);
	return offset;
}

int ofs_singularity_fops_iterate(struct file *file, struct dir_context *ctx)
{
	struct dentry *dentry = file->f_path.dentry;
	struct ofs_inode *oi = OFS_INODE(dentry->d_inode);
	struct ofs_file *of = file->private_data;
	struct dentry *cursor = of->cursor;
	struct list_head *p, *q = &cursor->d_child;

	ofs_dbg("oi<%p>; file<%p>; dentry<%p>==\"%pd\"; ctx<%p>;\n",
		oi, file, file->f_path.dentry, file->f_path.dentry, ctx);

	if (oi->magic != dentry)
		return -ENOSYS;

	ofs_dbg("entry: magic dentry\n");
	if (!dir_emit_dots(file, ctx))
		return 0;
	spin_lock(&dentry->d_lock);
	if (ctx->pos == 2)
		list_move(q, &dentry->d_subdirs);

	for (p = q->next; p != &dentry->d_subdirs; p = p->next) {
		struct dentry *next = list_entry(p, struct dentry, d_child);
		spin_lock_nested(&next->d_lock, DENTRY_D_LOCK_NESTED);
		if (!ofs_simple_positive(next)) {
			spin_unlock(&next->d_lock);
			continue;
		}

		spin_unlock(&next->d_lock);
		spin_unlock(&dentry->d_lock);
		if (!dir_emit(ctx, next->d_name.name, next->d_name.len,
			      next->d_inode->i_ino, ofs_dt_type(next->d_inode)))
			return 0;
		spin_lock(&dentry->d_lock);
		spin_lock_nested(&next->d_lock, DENTRY_D_LOCK_NESTED);
		/* next is still alive */
		list_move(q, p);
		spin_unlock(&next->d_lock);
		p = q;
		ctx->pos++;
	}
	spin_unlock(&dentry->d_lock);
	return 0;
}

int ofs_singularity_fops_fsync(struct file *file, loff_t start, loff_t end,
			       int dsync)
{
	return noop_fsync(file, start, end, dsync);
}

int ofs_singularity_fops_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct ofs_inode *oi = FILE_TO_OFS_INODE(file);

	ofs_dbg("oi<%p>; file<%p>; dentry<%p>==\"%pd\"; vma<%p>;\n",
		oi, file, file->f_path.dentry, file->f_path.dentry, vma);
	if (oi->ofsops && oi->ofsops->mmap)
		return oi->ofsops->mmap(file, vma);
	return -ENOSYS;
}

long ofs_singularity_fops_unlocked_ioctl(struct file *file, unsigned int cmd,
					 unsigned long args)
{
	struct ofs_inode *oi = FILE_TO_OFS_INODE(file);

	ofs_dbg("oi<%p>; file<%p>; dentry<%p>==\"%pd\"; cmd==%u; args==%lu;\n",
		oi, file, file->f_path.dentry, file->f_path.dentry, cmd, args);
	if (oi->ofsops && oi->ofsops->ioctl)
		return oi->ofsops->ioctl(file, cmd, args);
	return -ENOSYS;
}

ssize_t ofs_singularity_fops_write(struct file *file, const char __user *buf,
				   size_t len, loff_t *pos)
{
	struct ofs_inode *oi = FILE_TO_OFS_INODE(file);

	ofs_dbg("oi<%p>, file<%p>; dentry<%p>==\"%pd\"; "
		"buf<%p>; len==%zu; pos<%p>==%lld;\n",
		oi, file, file->f_path.dentry, file->f_path.dentry,
		buf, len, pos, *pos);
	if (oi->ofsops && oi->ofsops->write)
		return oi->ofsops->write(file, buf, len, pos);
	return -ENOSYS;
}

ssize_t ofs_singularity_fops_write_iter(struct kiocb *iocb,
					struct iov_iter *iter)
{
	struct ofs_inode *oi = FILE_TO_OFS_INODE(iocb->ki_filp);

	ofs_dbg("oi<%p>; iocb<%p>; dentry<%p>=\"%pd\"; iter<%p>\n",
		oi, iocb, iocb->ki_filp->f_path.dentry,
		iocb->ki_filp->f_path.dentry, iter);
	if (oi->ofsops && oi->ofsops->write_iter)
		return oi->ofsops->write_iter(iocb, iter);
	return -ENOSYS;
}

ssize_t ofs_singularity_fops_read(struct file *file, char __user *buf,
				  size_t len, loff_t *pos)
{
	struct ofs_inode *oi = FILE_TO_OFS_INODE(file);

	ofs_dbg("oi<%p>, file<%p>; dentry<%p>==\"%pd\"; "
		"buf<%p>; len==%zu; pos<%p>==%lld;\n",
		oi, file, file->f_path.dentry, file->f_path.dentry,
		buf, len, pos, *pos);
	if (oi->ofsops && oi->ofsops->read)
		return oi->ofsops->read(file, buf, len, pos);
	return -ENOSYS;
}

ssize_t ofs_singularity_fops_read_iter(struct kiocb *iocb,
				       struct iov_iter *iter)
{
	struct ofs_inode *oi = FILE_TO_OFS_INODE(iocb->ki_filp);

	ofs_dbg("oi<%p>; iocb<%p>; dentry<%p>==\"%pd\"; iter<%p>\n",
		oi, iocb, iocb->ki_filp->f_path.dentry,
		iocb->ki_filp->f_path.dentry, iter);
	if (oi->ofsops && oi->ofsops->read_iter)
		return oi->ofsops->read_iter(iocb, iter);
	return -ENOSYS;
}
