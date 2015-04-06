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
 * Source about directory
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
#include "log.h"
#include "dir.h"

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

/******** ******** directory operations ******** ********/
/******** inode_operations ********/
const struct inode_operations ofs_dir_iops = {
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
	.setattr = ofs_dir_iops_setattr,
	.getattr = ofs_dir_iops_getattr,
};

/******** file_operations ********/
const struct file_operations ofs_dir_fops = {
	.owner = THIS_MODULE,
	.llseek = ofs_dir_fops_llseek,
	.read = ofs_dir_fops_read,
	.write = ofs_dir_fops_write,
	.read_iter = ofs_dir_fops_read_iter,
	.iterate = ofs_dir_fops_iterate,
	.unlocked_ioctl = ofs_dir_fops_unlocked_ioctl,
	.mmap = ofs_dir_fops_mmap,
	.open = ofs_dir_fops_open,
	.release = ofs_dir_fops_release,
	.fsync = ofs_dir_fops_fsync,
};

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********      function implementations       ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** directory operations ******** ********/
/******** inode_operations ********/
/**
 * @brief ofs inode operation for directory: look up a dentry
 *        (linux >= 3.6.0)
 * @param iparent: base directory
 * @param dentry: the new dentry that VFS allocated.
 * @param flags: lookup flags.
 * @return the dentry that the function found.
 * @retval NULL: The function does nothing. Use the dentry that VFS allocated.
 * @retval pointer: (<b><em>struct dentry *</em></b>) that the function supplys
 *                  to replace the one that VFS allocated.
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * See <em>do_path_lookup()</em> in <b><i>fs/namei.c</i></b> to get the detail
 *   process of lookup. Default function is <em>simple_lookup()</em> in
 *   <b><i>fs/libfs.c</i></b>.
 * * If VFS can look up the dentry, this function will not be called. And VFS
 *   use the dentry as the result of lookup.
 * * If can't look up the dentry, VFS allocates a new dentry
 *   (via d_alloc()), then calls this function. the parameter
 *   <b><em>dentry</em></b> is the new dentry.
 * * If the return value of this function is <b>NOT</b> NULL, it means that the
 *   function can supply another dentry instead of the one that VFS
 *   allocated to you. VFS will dput(<b><em>dentry</em></b>), and use the return
 *   value as final result.
 * * If failed, return error code.
 * * linux >= 3.6.0.
 * @remark
 * * You can re-define this callback function and supply another dentry instead
 *   of the one that VFS allocated to you, please return your dentry, otherwise
 *   return NULL. If error occurs, return the error code.
 * * The function can't be used in ISR (Interrupt Service Routine).
 */
struct dentry *ofs_dir_iops_lookup(struct inode *iparent, struct dentry *dentry,
				   unsigned int flags)
{
	ofs_dbg("iparent<%p>; dentry<%p>==\"%pd\"; flags==0x%X;\n",
		iparent, dentry, dentry, flags);

	if (dentry->d_name.len > NAME_MAX)
		return ERR_PTR(-ENAMETOOLONG);
	if (!dentry->d_sb->s_d_op)
		d_set_d_op(dentry, &ofs_dops);
	d_add(dentry, NULL);
	return NULL;
}

/**
 * @brief ofs inode operation for directory: check permission
 * @param inode: target inode to be checked
 * @param mask: permission flags that want to be checked.
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * TODO
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 */
int ofs_dir_iops_permission(struct inode *inode, int mask)
{
//	ofs_dbg("inode<%p>; mask=0x%X;\n", inode, mask);
	return generic_permission(inode, mask);
}

/**
 * @brief ofs inode operation for directory: link (create hard link)
 * @param oldd: old dentry of inode
 * @param iparent: parent folder of new dentry
 * @param newd: new dentry of inode
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * This function is called under locking oldd->d_inode.i_mutex and
 *   iparent->i_mutex.
 * * Can't link a magic file by userspace syscall.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 */
int ofs_dir_iops_link(struct dentry *oldd, struct inode *iparent,
		      struct dentry *newd)
{
	struct inode *inode = oldd->d_inode;

	ofs_dbg("oldd<%p>==\"%pd\"; iparent<%p>; newd<%p>==\"%pd\";\n",
		oldd, oldd, iparent, newd, newd);
	inode->i_ctime = iparent->i_ctime = iparent->i_mtime = CURRENT_TIME;
	inc_nlink(inode);
	ihold(inode);
	dget(newd);
	d_instantiate(newd, inode);
	return 0;
}

/**
 * @brief ofs inode operation for directory: mknod
 * @param iparent: inode of parent folder
 * @param newd: dentry of target
 * @param mode: inode type and authority
 * @param dev: device number (when the new file is a device node.)
 * @return error code
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * TODO
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 */
int ofs_dir_iops_mknod(struct inode *iparent, struct dentry *newd,
		       umode_t mode, dev_t dev)
{
	struct inode *newi;
	struct ofs_inode *oiparent;
	int ret = -ENOSPC;

	ofs_dbg("iparent<%p>, newd<%p>==\"%pd\", mode=0%o, dev=%u\n",
		iparent, newd, newd, mode, dev);

	oiparent = OFS_INODE(iparent);
	newi = ofs_new_inode(iparent->i_sb, iparent, mode, dev, false);
	if (newi) {
		d_instantiate(newd, newi);
		dget(newd);
		ret = 0;
		iparent->i_mtime = iparent->i_ctime = CURRENT_TIME;
	}
	return ret;
}

static inline
void ofs_d_instantiate_singularity(struct dentry *dentry, struct inode *inode)
{
	BUG_ON(!hlist_unhashed(&dentry->d_u.d_alias));
	if (inode) {
		unsigned int flags = DCACHE_DIRECTORY_TYPE;

		spin_lock(&inode->i_lock);
		if (unlikely(!inode->i_op->lookup))
			flags = DCACHE_AUTODIR_TYPE;
		else
			inode->i_opflags |= IOP_LOOKUP;
		spin_lock(&dentry->d_lock);
		__d_set_type(dentry, flags);
		dentry->d_flags |= DCACHE_CANT_MOUNT;
		hlist_add_head(&dentry->d_u.d_alias, &inode->i_dentry);
		dentry->d_inode = inode;
		dentry_rcuwalk_barrier(dentry);
		spin_unlock(&dentry->d_lock);
	}
	fsnotify_d_instantiate(dentry, inode);
	if (inode)
		spin_unlock(&inode->i_lock);
	security_d_instantiate(dentry, inode);
}

/**
 * @brief ofs inode operation for directory: create (linux < 3.6.0)
 * @param iparent: inode of parent folder
 * @param newd: dentry of new file
 * @param mode: authority
 * @param excl:
 * @retval 0: OK.
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * linux >= 3.6.0
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 */
int ofs_dir_iops_create(struct inode *iparent, struct dentry *newd,
			umode_t mode, bool excl)
{
	struct inode *newi;
	struct ofs_inode *oiparent;
	bool is_singularity;
	int ret = -ENOSPC;

	ofs_dbg("iparent<%p>; newd<%p>==\"%pd\"; mode=0%o; excl=%s;\n",
		iparent, newd, newd, mode, excl ? "true" : "false");
	oiparent = OFS_INODE(iparent);
	is_singularity = oiparent->state & OI_CREATING_SINGULARITY;
	newi = ofs_new_inode(iparent->i_sb, iparent, mode | S_IFREG, 0,
			     is_singularity);
	if (newi) {
		if (is_singularity) {
			ofs_d_instantiate_singularity(newd, newi);
			inc_nlink(iparent); /* entry "child_singularity/.." */
		} else
			d_instantiate(newd, newi);
		dget(newd);
		ret = 0;
		iparent->i_mtime = iparent->i_ctime = CURRENT_TIME;
	}
	return ret;
}

/**
 * @brief ofs inode operation for directory: create symlink
 * @param iparent: inode of parent folder
 * @param newd: dentry of new symlink (note: symlink is a inode)
 * @param symname: symlink path
 * @return error code
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * TODO
 */
int ofs_dir_iops_symlink(struct inode *iparent, struct dentry *newd,
			 const char *symname)
{
	struct inode *newi;
	struct ofs_inode *oiparent;
	int error = -ENOSPC;

	ofs_dbg("iparent<%p>; newd<%p>==\"%pd\"; symname=%s;\n",
		iparent, newd, newd, symname);

	oiparent = OFS_INODE(iparent);
	newi = ofs_new_inode(iparent->i_sb, iparent, S_IFLNK | S_IRWXUGO,
			     0, false);
	if (newi) {
		int l = strlen(symname) + 1;
		error = page_symlink(newi, symname, l);
		if (!error) {
			d_instantiate(newd, newi);
			dget(newd);
			iparent->i_mtime = iparent->i_ctime = CURRENT_TIME;
		} else {
			iput(newi);
		}
	}
	return error;
}

/**
 * @brief ofs inode operation for directory: mk directory
 * @param iparent: inode of parent folder
 * @param newd: dentry of new folder
 * @param mode: authority
 * @return error code
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * i_nlink is the number of hard link. hard link of a directory is special.
 *   Only one name string, one "." and many ".." can be the hard link of
 *   a directory. So, when making a child directory of a parent directory,
 *   it means "child_name/.." is valid. As a result, The hard link of the parent
 *   directory inc 1.
 * * i_nlink of a directory is at least 2. Because name string and "." are
 *   always valid.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 */
int ofs_dir_iops_mkdir(struct inode *iparent, struct dentry *newd, umode_t mode)
{
	struct inode *newi;
	struct ofs_inode *oiparent;
	int ret = -ENOSPC;

	ofs_dbg("iparent<%p>; newd<%p>==\"%pd\";\n", iparent, newd, newd);

	oiparent = OFS_INODE(iparent);
	newi = ofs_new_inode(iparent->i_sb, iparent, mode | S_IFDIR, 0, false);
	if (newi) {
		d_instantiate(newd, newi);
		dget(newd);
		inc_nlink(iparent); /* entry of "child/.." */
		iparent->i_mtime = iparent->i_ctime = CURRENT_TIME;
		ret = 0;
	}
	return ret;
}

/**
 * @brief ofs inode operation for directory: rm directory
 * @param iparent: inode of parent folder
 * @param dentry: dentry of target directory
 * @return error code
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * This callback is called under locking dentry->d_inode.i_mutex and
 *   iparent->i_mutex.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 */
int ofs_dir_iops_rmdir(struct inode *iparent, struct dentry *dentry)
{
	struct ofs_inode *oiparent, *oi;
	struct inode *inode;

	ofs_dbg("iparent<%p>; dentry<%p>==\"%pd\"; dentry->d_inode<%p>};\n",
		iparent, dentry, dentry, dentry->d_inode);

	oiparent = OFS_INODE(iparent);
	inode = dentry->d_inode;
	oi = OFS_INODE(inode);

	/* Don't need to lock because reading a pointer is atomic. */
	if (oi->magic == dentry) {
		if (!(oiparent->state & OI_REMOVING_MAGIC_CHILD))
			return -ENOSYS;
	}
	if (!simple_empty(dentry))
		return -ENOTEMPTY;
	inode->i_ctime = iparent->i_ctime = iparent->i_mtime = CURRENT_TIME;
	/* drop_nlink() twice */
	WARN_ON(inode->i_nlink < 2);
	inode->__i_nlink -= 2;
	if (!inode->i_nlink)
		atomic_long_inc(&inode->i_sb->s_remove_count);
	dput(dentry);
	drop_nlink(iparent); /* rm "dentry/.." of parent */
	return 0;
}

/**
 * @brief ofs inode operation for directory: unlink (rm)
 * @param iparent: parent directory
 * @param dentry: target dentry to unlink
 * @return error code
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * This callback is called under locking dentry->d_inode.i_mutex and
 *   iparent->i_mutex.
 * * If removing a magic dentry of a magic file, parent needs the flag
 *   OI_REMOVING_MAGIC_CHILD.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 */
int ofs_dir_iops_unlink(struct inode *iparent, struct dentry *dentry)
{
	struct ofs_inode *oiparent, *oi;
	struct inode *inode;

	ofs_dbg("iparent<%p>; dentry<%p>==\"%pd\"; dentry->d_inode<%p>\n",
		iparent, dentry, dentry, dentry->d_inode);

	oiparent = OFS_INODE(iparent);
	inode = dentry->d_inode;
	oi = OFS_INODE(inode);
	/* Don't need to lock because reading a pointer is atomic. */
	if (oi->magic == dentry) {
		if (!(oiparent->state & OI_REMOVING_MAGIC_CHILD))
			return -ENOSYS;
		if (oi->state & OI_SINGULARITY) {
			if (!(oiparent->state & OI_REMOVING_SINGULARITY))
				return -EPERM;
			drop_nlink(iparent);
		}
	}
	inode->i_ctime = iparent->i_ctime = iparent->i_mtime = CURRENT_TIME;
	drop_nlink(inode);
	dput(dentry);
	return 0;

}

/**
 * @brief ofs inode operation for directory: rename2 (mv)
 * @param ioldparent: the old parent directory inode
 * @param dold: the old dentry of target in the old parent directory
 * @param inewparent: the new parent directory inode
 * @param newd: the new dentry of target in the new parent directory
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * TODO rename2 flags: RENAME_EXCHANGE, RENAME_NOREPLACE and RENAME_WITHOUT
 * * TODO what is RENAME_WITHOUT?
 * * <i>mv</i> the <b><em>dold</em></b> in
 *   the <b><em>ioldparent</em></b> to the <b><em>inewparent</em></b>
 *   with the new name <b><em>newd</em></b>.
 */
int ofs_dir_iops_rename2(struct inode *ioldparent, struct dentry *dold,
			 struct inode *inewparent, struct dentry *newd,
			 unsigned int flags)
{
	struct inode *iold =dold->d_inode, *newi = newd->d_inode;
	struct ofs_inode *oiold = OFS_INODE(iold);
	struct ofs_inode *newoi = OFS_INODE(newi);
	struct ofs_inode *oioldparent = OFS_INODE(ioldparent);
	struct ofs_inode *oinewparent = OFS_INODE(inewparent);
	bool old_is_fdr = d_is_dir(dold);
	bool new_is_fdr = false;

	ofs_dbg("ioldparent<%p>; dold<%p>==\"%pd\"; dold->d_inode<%p>;"
		"inewparent<%p>; newd<%p>==\"%pd\"; newd->d_inode<%p>\n",
		ioldparent, dold, dold, dold->d_inode,
		inewparent, newd, newd, newd->d_inode);

	if (oiold->magic == dold) {
		if (!(oioldparent->state & OI_RENAMING_MAGIC_CHILD)) {
			ofs_err("Can't rename a magic ofs inode by userspace "
				"syscall.\n");
			return -ENOSYS;
		}
		if (!oinewparent->magic) {
			ofs_err("New parent<%p> is not magic.\n", oinewparent);
			return -ENOENT;
		}
	}

	if (flags & RENAME_EXCHANGE) {
		new_is_fdr = d_is_dir(newd);
		if (old_is_fdr) {
			drop_nlink(ioldparent);
			inc_nlink(inewparent);
		}
		if (new_is_fdr) {
			drop_nlink(inewparent);
			inc_nlink(ioldparent);
		}
	} else {
		new_is_fdr = old_is_fdr;
		if (!simple_empty(newd)) {
			ofs_err("Can't replace a non-empty directory.\n");
			return -ENOTEMPTY;
		}

		if (newi) {
			if (newoi->magic == newd) {
				ofs_err("Can't replace a magic ofs "
					"inode<%p>.\n", newoi);
				return -EPERM;
			}
			drop_nlink(newi);
			dput(newd);
			if (new_is_fdr) {
				drop_nlink(newi);
				drop_nlink(ioldparent);
				/* drop_nlink(inewparent);
				   inc_nlink(inewparent); */
			}
		} else if (old_is_fdr) {
			drop_nlink(ioldparent);
			inc_nlink(inewparent);
		}
	}

	ioldparent->i_ctime = ioldparent->i_mtime = inewparent->i_ctime =
		inewparent->i_mtime = iold->i_ctime = CURRENT_TIME;

	return 0;
}

/**
 * @brief ofs inode operation for directory: set attributes
 * @param dentry: target
 * @param iattr: the new attributes
 * @return error code
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * TODO
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 */
int ofs_dir_iops_setattr(struct dentry *dentry, struct iattr *iattr)
{
//	ofs_dbg("dentry<%p>; iattr<%p>\n", dentry, iattr);
	return simple_setattr(dentry, iattr);
}

/**
 * @brief ofs inode operation for directory: set attributes
 * @param mnt: TODO
 * @param dentry: target
 * @param stat: the buffur to receive the attributes
 * @return error code
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * TODO
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 */
int ofs_dir_iops_getattr(struct vfsmount *mnt, struct dentry *dentry,
			 struct kstat *stat)
{
//	ofs_dbg("mnt<%p>; dentry<%p>; stat<%p>;\n", mnt, dentry, stat);
	return simple_getattr(mnt, dentry, stat);
}

/******** file_operations ********/
/**
 * @brief ofs file operation for directory: llseek
 * @param file: target directory that is opened.
 * @param offset: position
 * @param whence: TODO
 * @return the position
 * @retval >0: the position
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * The llseek callback is used to readdir. The position is the number of child
 *   dentry in the directory. The position is the relevant of the order that
 *   child dentries link in the list d_subdirs.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 */
loff_t ofs_dir_fops_llseek(struct file *file, loff_t offset, int whence)
{
	struct ofs_inode *oi = FILE_TO_OFS_INODE(file);
	ofs_dbg("oi<%p>; file<%p>; dentry<%p>==\"%pd\"; "
		"offset==%lld; whence==%d;\n",
		oi, file, file->f_path.dentry, file->f_path.dentry,
		offset, whence);
	return dcache_dir_lseek(file, offset, whence);
}

/**
 * @brief ofs file operation for directory: read
 * @param file: target directory that is opened.
 * @param buf: user buf
 * @param len: buf length
 * @param pos: file position
 * @return real data size that read from the directory
 * @retval >0: size
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * VFS returns -EISDIR in do_last(). So this function can't be called.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 */
ssize_t ofs_dir_fops_read(struct file *file, char __user *buf, size_t len,
			  loff_t *pos)
{
	struct ofs_inode *oi = FILE_TO_OFS_INODE(file);

	ofs_dbg("oi<%p>, file<%p>; dentry<%p>==\"%pd\"; "
		"buf<%p>; len==%zu; pos<%p>==%lld;\n",
		oi, file, file->f_path.dentry, file->f_path.dentry,
		buf, len, pos, *pos);
	return -EISDIR;
}

/**
 * @brief ofs file operation for directory: write
 * @param file: target directory that is opened.
 * @param buf: user data that will write to the directory
 * @param len: data length
 * @param pos: file position
 * @return real data size that wrote into the directory
 * @retval >0: size
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 */
ssize_t ofs_dir_fops_write(struct file *file, const char __user *buf,
			   size_t len, loff_t *pos)
{
	return -EISDIR;
}

/**
 * @brief ofs file operation for directory: read_iter
 * TODO
 */
ssize_t ofs_dir_fops_read_iter(struct kiocb * iocb, struct iov_iter * iter)
{
	return -EISDIR;
}

int ofs_dir_fops_iterate(struct file *file, struct dir_context *ctx)
{
	struct ofs_inode *oi = FILE_TO_OFS_INODE(file);
	ofs_dbg("oi<%p>; file<%p>; dentry<%p>==\"%pd\"; ctx<%p>;\n",
		oi, file, file->f_path.dentry, file->f_path.dentry, ctx);
	return dcache_readdir(file, ctx);
}

long ofs_dir_fops_unlocked_ioctl(struct file *file, unsigned int cmd,
				 unsigned long args)
{
	struct ofs_inode *oi = FILE_TO_OFS_INODE(file);

	ofs_dbg("oi<%p>; file<%p>; dentry<%p>==\"%pd\"; cmd==%u; args==%lu;\n",
		oi, file, file->f_path.dentry, file->f_path.dentry, cmd, args);
	return -EISDIR;
}

int ofs_dir_fops_mmap(struct file *file, struct vm_area_struct *vma)
{
	return -EISDIR;
}

int ofs_dir_fops_open(struct inode *inode, struct file *file)
{
	struct ofs_inode *oi = OFS_INODE(inode);

	ofs_dbg("oi<%p>; file<%p>; dentry<%p>==\"%pd\";\n",
		oi, file, file->f_path.dentry, file->f_path.dentry);
	return dcache_dir_open(inode, file);
}

int ofs_dir_fops_release(struct inode *inode, struct file *file)
{
	struct ofs_inode *oi = OFS_INODE(inode);

	ofs_dbg("oi<%p>; file<%p>; dentry<%p>==\"%pd\";\n",
		oi, file, file->f_path.dentry, file->f_path.dentry);
	return dcache_dir_close(inode, file);
}

int ofs_dir_fops_fsync(struct file *file, loff_t start, loff_t end, int dsync)
{
	return noop_fsync(file, start, end, dsync);
}

