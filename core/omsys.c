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
 * This source is sysfs interface to the ofs.
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
#include <linux/slab.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include "log.h"
#include "omsys.h"

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       macro       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/


/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       types       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/**
 * @brief magic mount object
 */
struct omobject {
	struct kset mm;		/**< relate to magic mount */
	struct ofs_root *root;	/**< ofs root */
};

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********         function prototypes         ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** omsys: /sys/fs/ofs/xxx entry ******** ********/
static
int omsys_uevent_filter(struct kset *kset, struct kobject *kobj);
static
ssize_t omsys_attr_show(struct kobject *kobj, struct attribute *attr,
			char *buf);
static
ssize_t omsys_attr_store(struct kobject *kobj, struct attribute *attr,
			 const char *buf, size_t count);
static
void omsys_release(struct kobject *kobj);

static __always_inline
struct ofs_root *omsys_get(struct ofs_root *root);

static __always_inline
void omsys_put(struct ofs_root *root);

/******** ******** /sys/fs/ofs/state ******** ********/
static
ssize_t ofs_attr_state_show(struct kobject *kobj, struct kobj_attribute *attr,
			    char *buf);

static
ssize_t ofs_attr_state_store(struct kobject *kobj, struct kobj_attribute *attr,
			     const char *buf, size_t count);

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********   .sdata & .data  ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** omsys: /sys/fs/ofs/xxx entry ******** ********/
static const struct kset_uevent_ops omsys_uevent_ops = {
	.filter = omsys_uevent_filter,
};

static const struct sysfs_ops omsys_sysfs_ops = {
	.show = omsys_attr_show,
	.store = omsys_attr_store,
};

static struct kobj_type omsys_ktype = {
	.sysfs_ops = &omsys_sysfs_ops,
	.release = omsys_release,
};

/******** ******** /sys/fs/ofs entry ******** ********/
/**
 * @brief /sys/fs/ofs entry
 */
static struct kset *omsys_kset;

/******** ******** /sys/fs/ofs/state entry ******** ********/
/**
 * @brief /sys/fs/ofs/state entry
 */
OFS_ATTR(state, ofs_attr_state_show, ofs_attr_state_store);

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********      function implementations       ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** omsys: /sys/fs/ofs/xxx entry ******** ********/
/**
 * @brief sysfs bindings for ofs: read syscall
 * @param kobj: kobject
 * @param attr: sysfs attribute file
 * @param buf: buffer related to user buffer of syscall `read'
 * @return actual size of `read'
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 */
static
ssize_t omsys_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	struct omsys_attribute *mattr = TO_MATTR(attr);
	struct omobject *omobj = TO_OMOBJ(kobj);
	ssize_t rc = 0;

	if (mattr->show)
		rc = mattr->show(omobj->root, buf);
	return rc;
}

/**
 * @brief sysfs bindings for ofs: write syscall
 * @param kobj: kobject
 * @param attr: sysfs attribute file
 * @param buf: buffer related to user buffer of syscall `write'
 * @param count: count of `write'
 * @return actual size of `write'
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 */
static
ssize_t omsys_attr_store(struct kobject *kobj, struct attribute *attr,
			 const char *buf, size_t count)
{
	struct omsys_attribute *mattr = TO_MATTR(attr);
	struct omobject *omobj = TO_OMOBJ(kobj);
	ssize_t rc = 0;

	if (mattr->store)
		rc = mattr->store(omobj->root, buf, count);
	return rc;
}

static
void omsys_release(struct kobject *kobj)
{
	struct omobject *omobj = container_of(kobj, typeof(*omobj), mm.kobj);
	struct ofs_root *root = omobj->root;

	kfree(omobj);
	root->omobj = NULL;
}

static __always_inline
struct ofs_root *omsys_get(struct ofs_root *root)
{
	if (root) {
		kset_get(&root->omobj->mm);
		return root;
	}
	return NULL;
}

static __always_inline
void omsys_put(struct ofs_root *root)
{
	if (root)
		kset_put(&root->omobj->mm);
}

/**
 * @brief create file in /sys/fs/ofs/xxx/
 * @param root: magic mount root
 * @param attr: attribute file to create
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 */
int omsys_create_file(struct ofs_root *root, struct omsys_attribute *attr)
{
	int rc;
	if (omsys_get(root)) {
		rc = sysfs_create_file(&root->omobj->mm.kobj, &attr->attr);
		omsys_put(root);
	} else
		rc = -EINVAL;
	return rc;
}

/**
 * @brief remove file in /sys/fs/ofs/xxx/
 * @param root: magic mount root
 * @param attr: attribute file to remove
 */
void omsys_remove_file(struct ofs_root *root, struct omsys_attribute *attr)
{
	if (omsys_get(root)) {
		sysfs_remove_file(&root->omobj->mm.kobj, &attr->attr);
		omsys_put(root);
	}
}

/**
 * @brief filter the uevent
 * @param kset: kset of kobj
 * @param kobj: kobject to filter
 * @retval 1: Don't filter it. generate uevent
 * @retval 0: Filter it. Don't generate uevent
 * @node 
 * * Help to decide whether to generate uevent.
 */
static
int omsys_uevent_filter(struct kset *kset, struct kobject *kobj)
{
	struct kobj_type *ktype = get_ktype(kobj);

	if (ktype == &omsys_ktype)
		return 1;
	return 0;
}

/**
 * @brief register a kset in sysfs (/sys/ofs).
 * @param root: magic filesystem to register
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @node 
 * Register the magic filesystem with the kobject infrastructure.
 */
int omsys_register(struct ofs_root *root,
		   const struct kset_uevent_ops *uevent_ops)
{
	int rc;
	struct omobject *omobj;

	omobj = kzalloc(sizeof(struct omobject), GFP_KERNEL);
	if (!omobj) {
		ofs_err("Failed to alloc a omobject!\n");
		rc = -ENOMEM;
		goto out_return;
	}

	rc = kobject_set_name(&omobj->mm.kobj, "%s", root->magic);
	if (rc) {
		ofs_err("Failed to set kobject name! (errno:%d)\n", rc);
		goto out_free_omobj;
	}
	omobj->mm.kobj.kset = omsys_kset;
	omobj->mm.kobj.ktype = &omsys_ktype;
	if (uevent_ops)
		omobj->mm.uevent_ops = uevent_ops;
	else
		omobj->mm.uevent_ops = &omsys_uevent_ops;

	rc = kset_register(&omobj->mm);
	if (rc) {
		ofs_err("Failed to kset_register()! (errno:%d)\n", rc);
		kfree(omobj->mm.kobj.name);
		omobj->mm.kobj.name = NULL;
		goto out_free_omobj;
	}

	omobj->root = root;
	root->omobj = omobj;
	return 0;

out_free_omobj:
	kfree(omobj);
out_return:
	return rc;
}

void omsys_unregister(struct ofs_root *root)
{
	if (root->omobj) {
		kset_unregister(&root->omobj->mm);
		root->omobj = NULL;
	}
}

/******** ******** /sys/fs/ofs/state ******** ********/
static
ssize_t ofs_attr_state_show(struct kobject *kobj,
				struct kobj_attribute *attr,
				char *buf)
{
	return 0;
}

static
ssize_t ofs_attr_state_store(struct kobject *kobj, struct kobj_attribute *attr,
			     const char *buf, size_t count)
{
	return count;
}

/******** ******** init & exit ******** ********/
int ofs_construct_sysfs(void)
{
	int rc;
	omsys_kset = kset_create_and_add("ofs", &omsys_uevent_ops,
					 fs_kobj);
	if (unlikely(IS_ERR_OR_NULL(omsys_kset))) {
		rc = -ENOMEM;
		ofs_dbg("Can't create ofs kset! \n");
		goto out_return;
	}

	rc = sysfs_create_file(&omsys_kset->kobj, &ofs_attr_state.attr);
	if (unlikely(rc)) {
		ofs_dbg("Can't create ofs_attr_state!\n");
		goto out_urg;
	}

	return 0;

out_urg:
	kset_unregister(omsys_kset);
out_return:
	return rc;
}

void ofs_destruct_sysfs(void)
{
	sysfs_remove_file(&omsys_kset->kobj, &ofs_attr_state.attr);
	kset_unregister(omsys_kset);
}
