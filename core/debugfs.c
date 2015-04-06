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
 * This source is debugfs interface to the ofs.
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
#include <linux/slab.h>
#include <linux/parser.h>
#include <asm/uaccess.h>
#include <linux/debugfs.h>
#include "log.h"
#include "debugfs.h"

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       macro       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#define OFSAPI_EXPORT(dbg, api, __call)					\
	do {								\
		dbg->__api_##api.result = 0;				\
		dbg->__api_##api.entry =				\
			debugfs_create_file(#api, 0664, dbg->apis,	\
					    &dbg->__api_##api,		\
					    &ofsdebug_api_fops);	\
		if (IS_ERR_OR_NULL(dbg->__api_##api.entry)) {		\
			rc = -ENOMEM;					\
			goto out_remove;				\
		}							\
		dbg->__api_##api.dbg = dbg;				\
		dbg->__api_##api.call = __call;				\
		mutex_init(&dbg->__api_##api.mtx);			\
	} while (0)

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       types       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
enum {
	ARG_NAME,
	ARG_MODE,
	ARG_FOLDER,
	ARG_TARGET,
	ARG_SINGULARITY,
	ARG_RECURSIVELY,
	ARG_FLAGS,
	ARG_ERR,
};

struct api_args {
	char *name;
	mode_t mode;
	ino_t folder;
	ino_t target;
	bool singularity;
	bool recursively;
	unsigned int flags;
};

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********         function prototypes         ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#ifdef CONFIG_DEBUG_FS
/******** ******** entry: tree ******** ********/
static
int ofsdebug_tree_open(struct inode *inode, struct file *file);
static
int ofsdebug_tree_release(struct inode *inode, struct file *file);
static
ssize_t ofsdebug_tree_read(struct file *file, char __user *buf,
			   size_t len, loff_t *pos);
static
ssize_t ofsdebug_tree_write(struct file *file, const char __user *buf,
			    size_t len, loff_t *pos);

/******** ******** entry: api ******** ********/
static
int ofsdebug_api_open(struct inode *inode, struct file *file);
static
int ofsdebug_api_release(struct inode *inode, struct file *file);
static
ssize_t ofsdebug_api_read(struct file *file, char __user *buf,
			  size_t len, loff_t *pos);
static
ssize_t ofsdebug_api_write(struct file *file, const char __user *buf,
			   size_t len, loff_t *pos);
#endif
/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********   .sdata & .data  ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#ifdef CONFIG_DEBUG_FS
/* /sys/kernel/debug/ofs */
static struct dentry *ofs_debugfs_entry;

/******** ******** entry: tree ******** ********/
static const struct file_operations ofsdebug_tree_fops = {
	.owner = THIS_MODULE,
	.open = ofsdebug_tree_open,
	.release = ofsdebug_tree_release,
	.read = ofsdebug_tree_read,
	.write = ofsdebug_tree_write,
};

/******** ******** entry: api ******** ********/
static const struct file_operations ofsdebug_api_fops = {
	.owner = THIS_MODULE,
	.open = ofsdebug_api_open,
	.release = ofsdebug_api_release,
	.read = ofsdebug_api_read,
	.write = ofsdebug_api_write,
};

/******** ******** api: ofs_mkdir_magic ******** ********/
static const match_table_t api_args_tokens = {
	{ARG_NAME, "name=%s"},
	{ARG_MODE, "mode=%o"},
	{ARG_FOLDER, "folder=%u"},
	{ARG_TARGET, "target=%u"},
	{ARG_SINGULARITY, "-s"},
	{ARG_RECURSIVELY, "-r"},
	{ARG_FLAGS, "flags=%s"},
	{ARG_ERR, NULL},
};

#endif

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********      function implementations       ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#ifdef CONFIG_DEBUG_FS

/******** ******** api: ofs_mkdir_magic ******** ********/
int call_ofs_mkdir_magic(struct ofs_api *api, const struct api_args *args)
{
	struct ofs_dbg *dbg = api->dbg;
	struct ofs_root *root = dbg->root;
	int rc;
	oid_t folder = (oid_t){root->sb, args->folder};
	oid_t result;

	if (args->name == NULL)
		return -EINVAL;
	rc = ofs_mkdir_magic(root, args->name, args->mode, folder, &result);
	if (!rc)
		api->result = result.i_ino;
	return rc;
}

/******** ******** api: ofs_symlink_magic ******** ********/
int call_ofs_symlink_magic(struct ofs_api *api, const struct api_args *args)
{
	struct ofs_dbg *dbg = api->dbg;
	struct ofs_root *root = dbg->root;
	int rc;
	oid_t folder = (oid_t){root->sb, args->folder};
	oid_t target = (oid_t){root->sb, args->target};
	oid_t result;

	if (args->name == NULL)
		return -EINVAL;
	rc = ofs_symlink_magic(root, args->name, folder, target, &result);
	if (!rc)
		api->result = result.i_ino;
	return rc;
}

/******** ******** api: ofs_create_magic ******** ********/
int call_ofs_create_magic(struct ofs_api *api, const struct api_args *args)
{
	struct ofs_dbg *dbg = api->dbg;
	struct ofs_root *root = dbg->root;
	int rc;
	oid_t folder = (oid_t){root->sb, args->folder};
	oid_t result;

	if (args->name == NULL)
		return -EINVAL;
	rc = ofs_create_magic(root, args->name, args->mode,
			      args->singularity, folder, &result);
	if (!rc)
		api->result = result.i_ino;
	return rc;
}

/******** ******** api: ofs_rmdir_magic ******** ********/
int call_ofs_rmdir_magic(struct ofs_api *api, const struct api_args *args)
{
	struct ofs_dbg *dbg = api->dbg;
	struct ofs_root *root = dbg->root;
	int rc;
	oid_t target = (oid_t){root->sb, args->target};

	if (args->target == 0)
		return -EINVAL;

	rc = ofs_rmdir_magic(root, target);
	if (!rc)
		api->result = rc;
	return rc;
}

/******** ******** api: ofs_unlink_magic ******** ********/
int call_ofs_unlink_magic(struct ofs_api *api, const struct api_args *args)
{
	struct ofs_dbg *dbg = api->dbg;
	struct ofs_root *root = dbg->root;
	int rc;
	oid_t target = (oid_t){root->sb, args->target};

	if (args->target == 0)
		return -EINVAL;

	rc = ofs_unlink_magic(root, target);
	if (!rc)
		api->result = rc;
	return rc;
}

/******** ******** api: ofs_rm_singularity ******** ********/
int call_ofs_rm_singularity(struct ofs_api *api, const struct api_args *args)
{
	struct ofs_dbg *dbg = api->dbg;
	struct ofs_root *root = dbg->root;
	int rc;
	oid_t target = (oid_t){root->sb, args->target};

	if (args->target == 0)
		return -EINVAL;

	rc = ofs_rm_singularity(root, target);
	if (!rc)
		api->result = rc;
	return rc;
}

/******** ******** api: ofs_rm_magic ******** ********/
int call_ofs_rm_magic(struct ofs_api *api, const struct api_args *args)
{
	struct ofs_dbg *dbg = api->dbg;
	struct ofs_root *root = dbg->root;
	int rc;
	oid_t target = (oid_t){root->sb, args->target};

	if (args->target == 0)
		return -EINVAL;

	rc = ofs_rm_magic(root, target, args->recursively);
	if (!rc)
		api->result = rc;
	return rc;
}

/******** ******** api: ofs_rename_magic ******** ********/
int call_ofs_rename_magic(struct ofs_api *api, const struct api_args *args)
{
	struct ofs_dbg *dbg = api->dbg;
	struct ofs_root *root = dbg->root;
	int rc;
	oid_t target = (oid_t){root->sb, args->target};
	oid_t folder = (oid_t){root->sb, args->folder};

	if (args->name == NULL)
		return -EINVAL;
	if (args->target == 0)
		return -EINVAL;
	rc = ofs_rename_magic(root, target, folder, args->name, args->flags);
	if (!rc)
		api->result = rc;
	return rc;
}

/******** ******** entry: tree ******** ********/
static
int ofsdebug_tree_open(struct inode *inode, struct file *file)
{
	mutex_lock(&inode->i_mutex);
	return 0;
}

static
int ofsdebug_tree_release(struct inode *inode, struct file *file)
{
	mutex_unlock(&inode->i_mutex);
	return 0;
}

static
ssize_t ofsdebug_tree_read(struct file *file, char __user *buf,
			   size_t len, loff_t *pos)
{
	return 0;
}

static
ssize_t ofsdebug_tree_write(struct file *file, const char __user *buf,
			    size_t len, loff_t *pos)
{
	return len;
}

/******** ******** entry: api ******** ********/
static
int ofsdebug_api_open(struct inode *inode, struct file *file)
{
	struct ofs_api *api = inode->i_private;
	ofs_dbg("enter debugging: %s\n", api->dbg->root->magic);
	mutex_lock(&api->mtx);
	file->private_data = inode->i_private;
	return 0;
}

static
int ofsdebug_api_release(struct inode *inode, struct file *file)
{
	struct ofs_api *api = inode->i_private;
	file->private_data = NULL;
	mutex_unlock(&api->mtx);
	ofs_dbg("exit debugging: %s\n", api->dbg->root->magic);
	return 0;
}

static
ssize_t ofsdebug_api_read(struct file *file, char __user *buf,
			  size_t len, loff_t *pos)
{
	struct ofs_api *api = file->private_data;
	char kbuf[8];
	int rc;
	ssize_t res;

	if (*pos)
		return 0;

	res = snprintf(kbuf, 8, "%lu\n", api->result);
	ofs_dbg("read api result: %s\n", kbuf);
	rc = copy_to_user(buf, kbuf, res);
	if (rc)
		return rc;
	*pos += res;
	return res;
}

static __always_inline
int match_ulong(substring_t *s, unsigned long *result, int base)
{
	char *endp;
	char *buf;
	int rc;
	unsigned long val;
	size_t len = s->to - s->from;

	buf = kmalloc(len + 1, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;
	memcpy(buf, s->from, len);
	buf[len] = '\0';

	rc = 0;
	val = simple_strtoul(buf, &endp, base);
	if (endp == buf)
		rc = -EINVAL;
	else
		*result = (int) val;
	kfree(buf);
	return rc;
}

static
ssize_t ofsdebug_api_write(struct file *file, const char __user *buf,
			   size_t len, loff_t *pos)
{
	struct ofs_api *api = file->private_data;
	struct api_args args;
	int token;
	char *argsbuf, *argspos;
	substring_t tmp[MAX_OPT_ARGS];
	char *p;
	int rc;

	args.name = NULL;
	args.mode = OFS_DEFAULT_MODE;
	args.folder = 0;
	args.target = 0;
	args.singularity = false;
	args.recursively = false;
	args.flags = 0;

	argsbuf = __getname();
	if (unlikely(!argsbuf)) {
		rc = -ENOMEM;
		goto out_return;
	}
	rc = strncpy_from_user(argsbuf, buf, len);
	if (unlikely(rc < 0))
		goto out_putname;

	
	ofs_dbg("call api, args=\"%s\" len=%ld\n", argsbuf, (unsigned long)len);

	rc = 0;
	argspos = argsbuf;
	while ((p = strsep(&argspos, ",")) != NULL) {
		if (!*p)
			continue;
		token = match_token(p, api_args_tokens, tmp);
		switch (token) {
		case ARG_NAME:
			args.name = match_strdup(&tmp[0]);
			if (!args.name) {
				rc = -ENOMEM;
				goto out_putname;
			}
			break;
		case ARG_MODE:
			rc = match_octal(&tmp[0], &args.mode);
			if (rc < 0) {
				rc = -EINVAL;
				goto out_putname;
			}
			args.mode &= 07777;
			break;
		case ARG_FOLDER:
			rc = match_ulong(&tmp[0], &args.folder, 10);
			if (rc < 0) {
				rc = -EINVAL;
				goto out_putname;
			}
			break;
		case ARG_TARGET:
			rc = match_ulong(&tmp[0], &args.target, 10);
			if (rc < 0) {
				rc = -EINVAL;
				goto out_putname;
			}
			break;
		case ARG_SINGULARITY:
			args.singularity = true;
			break;
		case ARG_RECURSIVELY:
			args.recursively = true;
			break;
		case ARG_FLAGS:
			break;
		}
	}
	ofs_dbg("args:name=\"%s\",mode=0%o,folder=%lu,target=%lu%s%s,"
		"flags=%d}\n",
		args.name, args.mode, args.folder, args.target,
		args.singularity ? ",singularity":"",
		args.recursively ? ",recursively":"",
		args.flags);

	if (api->call) {
		rc = api->call(api, &args);
		ofs_dbg("rc of call api: %d\n", rc);
		if (!rc)
			rc = len;
	} else {
		rc = -ENOSYS;
	}

out_putname:
	__putname(argsbuf);
out_return:
	if (args.name)
		kfree(args.name);
	return rc;
}
#endif

/******** ******** construct & destruct ******** ********/
int ofsdebug_construct(struct ofs_root *root)
{
	int rc = 0;
#ifdef CONFIG_DEBUG_FS
	struct ofs_dbg *dbg;

	dbg = (struct ofs_dbg *)kmalloc(sizeof(struct ofs_dbg),
						GFP_KERNEL);
	if (IS_ERR_OR_NULL(dbg))
		return -ENOMEM;
	dbg->this = debugfs_create_dir(root->magic, ofs_debugfs_entry);
	if (IS_ERR_OR_NULL(dbg->this)) {
		rc = -ENOMEM;
		goto out_kfree;
	}
	dbg->apis = debugfs_create_dir("apis", dbg->this);
	if (IS_ERR_OR_NULL(dbg->apis)) {
		rc = -ENOMEM;
		goto out_remove;
	}
	dbg->tree = debugfs_create_file("tree", 0664, dbg->this,
					    root, &ofsdebug_tree_fops);
	if (IS_ERR_OR_NULL(dbg->tree)) {
		rc = -ENOMEM;
		goto out_remove;
	}

	OFSAPI_EXPORT(dbg, ofs_mkdir_magic, call_ofs_mkdir_magic);
	OFSAPI_EXPORT(dbg, ofs_symlink_magic, call_ofs_symlink_magic);
	OFSAPI_EXPORT(dbg, ofs_create_magic, call_ofs_create_magic);
	OFSAPI_EXPORT(dbg, ofs_rmdir_magic, call_ofs_rmdir_magic);
	OFSAPI_EXPORT(dbg, ofs_unlink_magic, call_ofs_unlink_magic);
	OFSAPI_EXPORT(dbg, ofs_rm_singularity, call_ofs_rm_singularity);
	OFSAPI_EXPORT(dbg, ofs_rm_magic, call_ofs_rm_magic);
	OFSAPI_EXPORT(dbg, ofs_rename_magic, call_ofs_rename_magic);

	dbg->root = root;
	root->dbg = dbg;
	return rc;

out_remove:
	debugfs_remove_recursive(root->dbg->this);
out_kfree:
	kfree(dbg);
#endif
	return rc;
}

void ofsdebug_destruct(struct ofs_root *root)
{
#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(root->dbg->this);
	root->dbg = NULL;
	kfree(root->dbg);
#endif
}

/******** ******** init & exit ******** ********/
int ofs_debugfs_init(void)
{
	int rc = 0;
#ifdef CONFIG_DEBUG_FS
	ofs_debugfs_entry = debugfs_create_dir("ofs", NULL);
	if (!ofs_debugfs_entry) {
		ofs_dbg("Can't create entry in debugfs");
		rc = -ENOMEM;
	}
#endif
	return rc;
}

void ofs_debugfs_exit(void)
{
#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(ofs_debugfs_entry);
#endif
}
