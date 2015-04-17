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
 * C source about normal apis
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

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********      function implementations       ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
int ofs_rmdir_normal_hlp(struct ofs_inode *oip, struct path *pathp,
			 struct dentry *dtgt)
{
	int rc;
	struct inode *ip = &oip->inode;
	struct dentry *dp = pathp->dentry;

	rc = mnt_want_write(pathp->mnt);
	if (unlikely(rc)) {
		ofs_dbg("Failed to mnt_want_write()! (errno: %d)\n", rc);
		goto out_return;
	}
	mutex_lock_nested(&ip->i_mutex, I_MUTEX_PARENT);
	/* Get dentry and dp before mutex_lock. So it needs check. */
	if (dp->d_inode != ip) {
		ofs_dbg("The parent is death!\n");
		rc = -EOWNERDEAD;
		goto out_mutex_unlock;
	}
	if (dtgt->d_parent != dp) {
		ofs_dbg("Child changed!\n");
		rc = -ECHILD;
		goto out_mutex_unlock;
	}

	rc = security_path_rmdir(pathp, dtgt);
	if (rc) {
		ofs_dbg("Failed in security_path_rmdir() (errno: %d)\n", rc);
		goto out_mutex_unlock;
	}
	rc = vfs_rmdir(ip, dtgt);

out_mutex_unlock:
	mutex_unlock(&ip->i_mutex);
	mnt_drop_write(pathp->mnt);
out_return:
	return rc;
}

int ofs_rmdir_normal(struct ofs_root *root, struct inode *ip,
		     struct dentry *dtgt)
{
	struct path pathp;
	struct ofs_inode *oip;
	int rc;

#ifdef CONFIG_OFS_CHKPARAM
	if (unlikely(IS_ERR_OR_NULL(root))) {
		ofs_err("Pointer root is invalid!\n");
		return -EINVAL;
	}
	if (unlikely(IS_ERR_OR_NULL(ip))) {
		ofs_err("Pointer ip is invalid!\n");
		return -EINVAL;
	}
	if (unlikely(IS_ERR_OR_NULL(dtgt))) {
		ofs_err("Pointer dentry is invalid!\n");
		return -EINVAL;
	}
#endif

	down_read(&root->rwsem);
	if (unlikely(!root->is_registered)) {
		ofs_err("Filesystem is not registered!\n");
		rc = -EPERM;
		goto out_up_read;
	}

	ip = igrab(ip);
	if (!ip) {
		rc = -EOWNERDEAD;
		goto out_up_read;
	}
	oip = OFS_INODE(ip);
	if (S_ISDIR(ip->i_mode)) {
		pathp.dentry = d_find_any_alias(ip);
	} else if (S_ISREG(ip->i_mode) &&
		   (oip->state & OI_SINGULARITY)) {
		pathp.dentry = ofs_get_magic_alias(oip);
	} else {
		rc = -ENOTDIR;
		goto out_iput;
	}
	pathp.mnt = mntget(root->magic_root.mnt);

	rc = d_validate(dtgt, pathp.dentry);
	if (!rc) {
		rc = -ENOENT;
		goto out_path_put;
	}

	rc = ofs_rmdir_normal_hlp(oip, &pathp, dtgt);
	dput(dtgt);

out_path_put:
	path_put(&pathp);
out_iput:
	iput(ip);
out_up_read:
	up_read(&root->rwsem);
	return rc;
}
EXPORT_SYMBOL(ofs_rmdir_normal);

int ofs_unlink_normal_hlp(struct ofs_inode *oip, struct path *pathp,
			  struct dentry *dtgt)
{
	int rc;
	struct inode *idelegated = NULL;
	struct inode *ip = &oip->inode;
	struct dentry *dp = pathp->dentry;

	rc = mnt_want_write(pathp->mnt);
	if (unlikely(rc)) {
		ofs_dbg("Failed to mnt_want_write()! (errno: %d)\n", rc);
		goto out_return;
	}

retry_deleg:
	mutex_lock_nested(&ip->i_mutex, I_MUTEX_PARENT);
	/* Get dentry and dp before mutex_lock. So it needs check. */
	if (dp->d_inode != ip) {
		ofs_dbg("The parent is death!\n");
		rc = -EOWNERDEAD;
		goto out_mutex_unlock;
	}
	if (dtgt->d_parent != dp) {
		ofs_dbg("Child changed!\n");
		rc = -ECHILD;
		goto out_mutex_unlock;
	}

	rc = security_path_unlink(pathp, dtgt);
	if (rc) {
		ofs_dbg("Failed in security_path_unlink() (errno: %d)\n", rc);
		goto out_mutex_unlock;
	}
	rc = vfs_unlink(ip, dtgt, &idelegated);
	if (idelegated) {
		mutex_unlock(&ip->i_mutex);
		rc = break_deleg_wait(&idelegated);
		if (!rc)
			goto retry_deleg;
	}

out_mutex_unlock:
	mutex_unlock(&ip->i_mutex);
	mnt_drop_write(pathp->mnt);
out_return:
	return rc;
}

int ofs_unlink_normal(struct ofs_root *root, struct inode *ip,
		      struct dentry *dtgt)
{
	struct path pathp;
	struct ofs_inode *oip;
	int rc;

#ifdef CONFIG_OFS_CHKPARAM
	if (unlikely(IS_ERR_OR_NULL(root))) {
		ofs_err("Pointer root is invalid!\n");
		return -EINVAL;
	}
	if (unlikely(IS_ERR_OR_NULL(ip))) {
		ofs_err("Pointer ip is invalid!\n");
		return -EINVAL;
	}
	if (unlikely(IS_ERR_OR_NULL(dtgt))) {
		ofs_err("Pointer dentry is invalid!\n");
		return -EINVAL;
	}
#endif

	down_read(&root->rwsem);
	if (unlikely(!root->is_registered)) {
		ofs_err("Filesystem is not registered!\n");
		rc = -EPERM;
		goto out_up_read;
	}

	ip = igrab(ip);
	if (!ip) {
		rc = -EOWNERDEAD;
		goto out_up_read;
	}
	oip = OFS_INODE(ip);
	if (S_ISDIR(ip->i_mode)) {
		pathp.dentry = d_find_any_alias(ip);
	} else if (S_ISREG(ip->i_mode) &&
		   (oip->state & OI_SINGULARITY)) {
		pathp.dentry = ofs_get_magic_alias(oip);
	} else {
		rc = -ENOTDIR;
		goto out_iput;
	}
	pathp.mnt = mntget(root->magic_root.mnt);

	rc = d_validate(dtgt, pathp.dentry);
	if (!rc) {
		rc = -ENOENT;
		goto out_path_put;
	}

	rc = ofs_unlink_normal_hlp(oip, &pathp, dtgt);
	dput(dtgt);

out_path_put:
	path_put(&pathp);
out_iput:
	iput(ip);
out_up_read:
	up_read(&root->rwsem);
	return rc;
}
EXPORT_SYMBOL(ofs_unlink_normal);
