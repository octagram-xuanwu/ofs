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
 * C source about magic apis
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
#include "ksym.h"
#ifdef CONFIG_OFS_SYSFS
#include "omsys.h"
#endif
#ifdef CONFIG_OFS_DEBUGFS
#include "debugfs.h"
#endif

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
/******** ******** rbtree hash table ******** ********/
struct ofs_rbtree *ofs_rbtrees = NULL;
extern int hashtable_size_bits;

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********      function implementations       ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** red-black tree ******** ********/
/**
 * @brief ofs red-black tree function: lookup
 * @param tree: red-black tree
 * @param target: ofs inode descriptor (key in red-black tree)
 * @return ofs inode
 * @retval NULL: nothing to found.
 * @note
 * * This is a helper function. It is unsafe. It supposes that
 *   the @b tree is exactly right.
 * @remark
 * * Call @ref ofs_oiput() to do some cleaning work when finished
 *   all operation.
 * @sa ofs_oiput()
 */
struct ofs_inode *ofs_rbtree_lookup(struct ofs_rbtree *tree, const oid_t target)
{
	struct rb_node *n;
	struct ofs_inode *oitgt;
	oid_t d;
	int ret;

	read_lock(&tree->lock); /* Can be interrupted,
				   but can't be called in ISR */
	n = tree->root.rb_node;
	while (n) {
		oitgt = rb_entry(n, struct ofs_inode, rbnode);
		d = ofs_get_oid(oitgt);
		ret = ofs_compare_oid(target, d);
		if (ret < 0) {
			n = n->rb_left;
		} else if (ret > 0) {
			n = n->rb_right;
		} else {
			read_unlock(&tree->lock);
			if (igrab(&oitgt->inode)) {
				return oitgt;
			}
			return NULL;
		}
	}
	read_unlock(&tree->lock);
	return NULL;
}

/**
 * @brief Decrease the reference count of a ofs_inode.
 * @param oitgt: ofs inode
 * @note
 * * This is a helper function. It is unsafe. It supposes that
 *   the @b oitgt is exactly right.
 * * Used to decrease the reference count of the ofs_inode after
 *   @ref ofs_rbtree_lookup().
 * @sa ofs_rbtree_lookup
 */
void ofs_rbtree_oiput(struct ofs_inode *oitgt)
{
	iput(&oitgt->inode);
}

/**
 * @brief ofs red-black tree function: insert
 * @param tree: red-black tree
 * @param newoi: new ofs inode that will be inserted.
 * @retval true: OK
 * @retval false: Failed. There is a node that already has the inode number.
 * @note
 * * This is a helper function. It is unsafe. It supposes that the
 *   <b><em>tree</em></b> and <b><em>newoi</em></b> are exactly right.
 */
bool ofs_rbtree_insert(struct ofs_rbtree *tree, struct ofs_inode *newoi)
{
	struct rb_node **new, *parent = NULL;
	oid_t d, newoid;
	struct ofs_inode *oi;
	int ret;

	newoid = ofs_get_oid(newoi);
	write_lock(&tree->lock);
	new = &(tree->root.rb_node);
	while (*new) {
		oi = rb_entry(*new, struct ofs_inode, rbnode);
		d = ofs_get_oid(oi);
		ret = ofs_compare_oid(newoid, d);
		parent = *new;
		if (ret < 0) {
			new = &((*new)->rb_left);
		} else if (ret > 0) {
			new = &((*new)->rb_right);
		} else {
			write_unlock(&tree->lock);
			BUG();	/* inode number is unique */
			return false;
		}
	}

	rb_link_node(&newoi->rbnode, parent, new);
	rb_insert_color(&newoi->rbnode, &tree->root);
	write_unlock(&tree->lock);

	return true;
}

/**
 * @brief ofs red-black tree function: remove
 * @param tree: red-black tree
 * @param oitgt: ofs inode that will be removed.
 * @note
 * * This is a helper function. It is unsafe.
     It supposes that the <b><em>tree</em></b>
 *   and <b><em>oitgt</em></b> are exactly right.
 */
void ofs_rbtree_remove(struct ofs_rbtree *tree, struct ofs_inode *oitgt)
{
	write_lock(&tree->lock);
	rb_erase(&oitgt->rbnode, &tree->root);
	write_unlock(&tree->lock);
}

/**
 * @brief get the base address of rbtree
 * @param oitgt: ofs inode
 * @return rbtree
 * @note
 * * This is a helper function. It is unsafe.
     It supposes that the<b><em>oitgt</em></b>
 *   is exactly right.
 */
struct ofs_rbtree *ofs_get_rbtree(struct ofs_inode *oitgt)
{
	unsigned long hashval =
		(unsigned long)oitgt->inode.i_sb / L1_CACHE_BYTES +
		oitgt->inode.i_ino;
#ifdef CONFIG_64BIT
	hashval = hash_64(hashval, hashtable_size_bits);
#else
	hashval = hash_32(hashval, hashtable_size_bits);
#endif
	return &ofs_rbtrees[hashval];
}

/**
 * @brief get the base address of rbtree by ofs_id
 * @param target: ofs inode descriptor
 * @return rbtree
 * @note
 * * This is a helper function.
 */
struct ofs_rbtree *ofs_get_rbtree_by_oid(const oid_t target)
{
	unsigned long hashval = (unsigned long)target.i_sb / L1_CACHE_BYTES +
				target.i_ino;
#ifdef CONFIG_64BIT
	hashval = hash_64(hashval, hashtable_size_bits);
#else
	hashval = hash_32(hashval, hashtable_size_bits);
#endif
	return &ofs_rbtrees[hashval];
}

/******** ******** magic apis ******** ********/
/******** ofs mount apis ********/
/**
 * @brief ofs magic api: Register a magic ofs.
 * @param magic: unique magic string to identify the ofs.
 * @return pointer (<b><em>struct ofs_root *</em></b>) that indicate the
 *         magic ofs if successed.
 * @retval pointer: (<b><em>struct ofs_root *</em></b>)
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * All magic apis can be called under the situation that the magic ofs
 *   is registered.
 * * The function will mount ofs in kernel (To generate a object: vfsmount).
 *   The vfsmount is not visible in userspace. So when the vfsmount in
 *   <b><em>struct path</em></b>, the path is not related to userspace path.
 *   It is only used in magic apis.
 * * User can create a magic directory, a symlink, a regfile or a singularity
 *   only in a magic ofs.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_unregister()
 */
struct ofs_root *ofs_register(char *magic)
{
	struct vfsmount *mnt;
	struct ofs_root *root;
	int rc;

#ifdef CONFIG_OFS_CHKPARAM
	if (unlikely(IS_ERR_OR_NULL(magic))) {
		ofs_err("magic is invalid pointer.\n");
		return ERR_PTR(-EINVAL);
	}
	if (unlikely(strcmp(magic, "") == 0)) {
		ofs_err("magic is empty.\n");
		return ERR_PTR(-EINVAL);
	}
#endif

	ofs_dbg("magic=\"%s\";\n", magic);
	mnt = kern_mount_data(&ofs_fstype, magic);
	if (unlikely(IS_ERR(mnt))) {
		ofs_dbg("kern_mount_data() failed! (errno:%ld)\n", (long)mnt);
		rc = PTR_ERR(mnt);
		goto out_return;
	}

	root = (struct ofs_root *)(mnt->mnt_sb->s_fs_info);
	BUG_ON(root == NULL);
	down_write(&root->rwsem);
	if (unlikely(root->is_registered)) {
		ofs_err("magic fs (%s) is already registered!\n", magic);
		rc = -EEXIST;
		goto out_umount;
	}

#ifdef CONFIG_OFS_SYSFS
	rc = omsys_register(root, NULL);
	if (unlikely(rc)) {
		ofs_dbg("Can't register omsys! (errno:%d)\n", rc);
		goto out_umount;
	}
#endif
#ifdef CONFIG_OFS_DEBUGFS
	rc = ofsdebug_construct(root);
	if (unlikely(rc)) {
		ofs_dbg("Can't register debugfs! (errno:%d)\n", rc);
		goto out_unrg_omsys;
	}
#endif

	root->magic_root.mnt = mnt;
	root->magic_root.dentry = mnt->mnt_root;
	root->is_registered = true;
	up_write(&root->rwsem);
	return root;

#ifdef CONFIG_OFS_DEBUGFS
out_unrg_omsys:
#endif
#ifdef CONFIG_OFS_SYSFS
	omsys_unregister(root);
#endif
out_umount:
	up_write(&root->rwsem);
	kern_unmount(mnt);
out_return:
	return ERR_PTR(rc);
}
EXPORT_SYMBOL(ofs_register);

/**
 * @brief ofs magic api: Unregister a magic ofs.
 * @param root: The filesystem that will be unregistered.
 * @retval 0: OK.
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * To do some cleaning work after @ref ofs_register() when exiting.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_register()
 */
int ofs_unregister(struct ofs_root *root)
{
#ifdef CONFIG_OFS_CHKPARAM
	if (unlikely(IS_ERR_OR_NULL(root))) {
		ofs_dbg("root is invalid pointer.\n");
		return -EINVAL;
	}
#endif

	ofs_dbg("root<%p>;\n", root);
	down_write(&root->rwsem);
	if (unlikely(!root->is_registered)) {
		up_write(&root->rwsem);
		ofs_err("fs is not registered!\n");
		return -EPERM;
	}
	root->is_registered = false;
	up_write(&root->rwsem);

#ifdef CONFIG_OFS_DEBUGFS
	ofsdebug_destruct(root);
#endif
#ifdef CONFIG_OFS_SYSFS
	omsys_unregister(root);
#endif

	kern_unmount(root->magic_root.mnt);

	return 0;
}
EXPORT_SYMBOL(ofs_unregister);

/**
 * @brief ofs magic api: Look up a magic ofs that is registered by
 *        the unique magic string.
 * @param magic: unique magic string.
 * @return pointer (<b><em>struct ofs_root *</em></b>) that indicate
 *         the magic ofs if successful
 * @retval pointer: (<b><em>struct ofs_root *</b></em>)
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * The function will increase the active count of the super block that be
 *   found. So, need to call @ref ofs_put_root_magic() later.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_put_root()
 */
struct ofs_root *ofs_lookup_root(char *magic)
{
	struct super_block *sb;
	struct ofs_root *root = NULL;

#ifdef CONFIG_OFS_CHKPARAM
	if (unlikely(IS_ERR_OR_NULL(magic))) {
		ofs_dbg("magic is invalid pointer.\n");
		return ERR_PTR(-EINVAL);
	}
	if (unlikely(strcmp(magic, "") == 0)) {
		ofs_dbg("magic is empty.\n");
		return ERR_PTR(-EINVAL);
	}
#endif

	ofs_dbg("magic=%s;\n", magic);
	/* sget() ---> down_write(sb->s_umount)
	 * It is already locked. DON'T call deactivate_super()
	 */
	sb = sget(&ofs_fstype, ofs_test_super, ofs_set_super,
		  MS_KERNMOUNT, magic);
	root = (struct ofs_root *)sb->s_fs_info;
	if (unlikely(root == NULL)) {
		deactivate_locked_super(sb);
		return ERR_PTR(-ENODEV);
	} else {
		down_read(&root->rwsem);
		if (!root->is_registered) {
			up_read(&root->rwsem);
			deactivate_locked_super(sb);
			return ERR_PTR(-ENODEV);
		}
	}
	up_read(&root->rwsem);
	up_write(&sb->s_umount);
	return root;
}
EXPORT_SYMBOL(ofs_lookup_root);

/**
 * @brief ofs magic api: Decrease the active count of
 *        a registered magic ofs.
 * @param root: a registered magic ofs
 * @retval 0: OK.
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * To do some cleaning work after @ref ofs_lookup_root_magic()
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_lookup_root()
 */
int ofs_put_root(struct ofs_root *root)
{
#ifdef CONFIG_OFS_CHKPARAM
	if (IS_ERR_OR_NULL(root)) {
		ofs_dbg("root is invalid pointer.\n");
		return -EINVAL;
	}
#endif

	ofs_dbg("root<%p>;\n", root);
	down_read(&root->rwsem);
	if (unlikely(!root->is_registered)) {
		up_read(&root->rwsem);
		ofs_dbg("root is nod registered.\n");
		return -EPERM;
	}
	up_read(&root->rwsem);
	deactivate_super(root->sb);
	return 0;
}
EXPORT_SYMBOL(ofs_put_root);

/******** ofs inode apis ********/
/**
 * @brief get the dentry that owns the magic name
 * @param oitgt: ofs inode
 * @return the dentry
 * @retval NULL: not found
 * @note
 * * The function will increase the refcount of the dentry that be found.
 *   So need @ref ofs_put_magic_alias() later.
 * @sa ofs_put_magic_alias()
 */
struct dentry *ofs_get_magic_alias(struct ofs_inode *oitgt)
{
	struct dentry *alias;
	spin_lock(&oitgt->inode.i_lock);
	hlist_for_each_entry(alias, &oitgt->inode.i_dentry, d_u.d_alias) {
		spin_lock(&alias->d_lock);
		if (alias == oitgt->magic) {
			alias->d_lockref.count++;
			spin_unlock(&alias->d_lock);
			break;
		}
		spin_unlock(&alias->d_lock);
	}
	spin_unlock(&oitgt->inode.i_lock);
	return alias;
}
EXPORT_SYMBOL(ofs_get_magic_alias);

/**
 * @brief helper function to get the folder path and ofs inode from
 *        the ofs inode descriptor
 * @param root: magic ofs that has been registered
 * @param folder: ofs inode descriptor of folder
 * @param result: buffer to return the result
 * @return the folder path, and the folder ofs inode
 * @retval valid pointer to the folder ofs inode: OK
 * @retval errno pointer: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * This is a helper function. It is unsafe. It supposes that
 *   the <b><em>root<\b><\em> is registered.
 * * Call this function under locking <b><em>root->rwsem</em></b>.
 * * It is important that to call @ref ofs_put_folder() to do some
 *   cleaning work later.
 * * Folder is either a magic directory or a magic regfile.
 * * The function will increase the refcount of the inode and the path of the
 *   folder.
 * * If the folder is a symlink, the function follows it.
 * @sa ofs_get_folder()
 * @sa ofs_put_folder()
 */
static
struct ofs_inode *ofs_get_folder_hlp(struct ofs_root *root, oid_t folder,
				     struct path *result)
{
	struct ofs_rbtree *tree;
	struct ofs_inode *oifdr;
	struct inode *ifdr;
	struct dentry *dfdr;
	struct path pathfdr;
	struct ofs_nameidata nd;
	int rc;

	nd.depth = 0;
	nd.saved_links[nd.depth] = NULL;
	pathfdr.mnt = mntget(root->magic_root.mnt);
	BUG_ON(!pathfdr.mnt);
retry:
	tree = ofs_get_rbtree_by_oid(folder);
	oifdr = ofs_rbtree_lookup(tree, folder);
	if (unlikely(oifdr == NULL)) {
		ofs_dbg("Can't get folder {%p, %lu}.\n",
			folder.i_sb, folder.i_ino);
		rc = -ENOENT;
		goto out_exit;
	}

	ifdr = &oifdr->inode;
	if (unlikely(ifdr->i_sb != root->sb)) {
		ofs_dbg("The ofs inode {%p, %lu} is not in the magic ofs!\n",
			folder.i_sb, folder.i_ino);
		rc = -EPERM;
		goto out_oiput;
	}

	if (S_ISDIR(ifdr->i_mode) ||
	    (S_ISREG(ifdr->i_mode) && (oifdr->state & OI_SINGULARITY))) {
		dfdr = ofs_get_magic_alias(oifdr);
		if (unlikely(dfdr == NULL)) {
			ofs_crt("No magic dentry! Is it removing?\n");
			rc = -EOWNERDEAD;
			goto out_oiput;
		}
		ofs_rbtree_oiput(oifdr);
		pathfdr.dentry = dfdr;
		/* TODO if the folder is a mountpoint */
	} else if (S_ISLNK(ifdr->i_mode)) {
		oid_t sl;
		/* TODO symlink to other magic ofs */
		if (unlikely(nd.depth >= MAX_NESTED_LINKS)) {
			ofs_dbg("Too many symbolic links encountered!\n");
			rc = -ELOOP;
			goto out_oiput;
		}
		BUG_ON(!ofs_compare_oid(oifdr->symlink, OFS_NULL_OID));
		ofs_nd_set_link(&nd, &oifdr->symlink);
		sl = ofs_nd_get_link(&nd);
		nd.depth++;
		ofs_rbtree_oiput(oifdr);
		folder = sl;
		goto retry;
	} else {
		rc = -ENOTDIR;
		goto out_oiput;
	}

	*result = pathfdr;
	return oifdr;

out_oiput:
	ofs_rbtree_oiput(oifdr);
out_exit:
	mntput(pathfdr.mnt);
	return ERR_PTR(rc);
}

/**
 * @brief get the path and ofs inode of a folder from the
 *        ofs inode descriptor
 * @param root: magic ofs that has been registered
 * @param folder: ofs inode descriptor of folder
 * @param result: buffer to return the result
 * @return the folder path, and the folder ofs inode
 * @retval valid pointer to the folder ofs inode: OK
 * @retval errno pointer: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * It is important that to call @ref ofs_put_folder() to do some
 *   cleaning work later.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_put_folder()
 * @sa ofs_get_folder_hlp()
 */
struct ofs_inode *ofs_get_folder(struct ofs_root *root, oid_t folder,
				 struct path *result)
{
	struct ofs_inode *oifdr;

#ifdef CONFIG_OFS_CHKPARAM
	if (unlikely(IS_ERR_OR_NULL(root))) {
		ofs_err("Pointer root is invalid!\n");
		return ERR_PTR(-EINVAL);
	}
	if (unlikely(IS_ERR_OR_NULL(result))) {
		ofs_err("Pointer result is invalid!\n");
		return ERR_PTR(-EINVAL);
	}
#endif

	if (ofs_compare_oid(folder, OFS_NULL_OID) == 0)
		folder = ofs_get_oid(root->rootoi);

	down_read(&root->rwsem);
	if (!root->is_registered) {
		up_read(&root->rwsem);
		ofs_err("Filesystem is not registered!\n");
		return ERR_PTR(-EPERM);
	}
	oifdr = ofs_get_folder_hlp(root, folder, result);
	if (unlikely(IS_ERR_OR_NULL(oifdr)))
		ofs_err("Can't get folder!\n");
	up_read(&root->rwsem);
	return oifdr;
}
EXPORT_SYMBOL(ofs_get_folder);

/**
 * @brief helper function to get the path and ofs inode of parent
 *        from a child path.
 * @param root: magic ofs that has been registered
 * @param pathtgt: child path of ofs inode
 * @param result: buffer to return the result
 * @return the ofs inode, and the path
 * @retval vaild pointer to ofs inode of the parent: OK
 * @retval errno pointer: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * This is a helper function. It is unsafe. It supposes that
 *   the <b><em>root<\b><\em> is registered and oipath is already path_get().
 * * Call this function under read-locking <b><em>root->rwsem</em></b>.
 * * It is important that to call @ref ofs_put_parent() to do some
 *   cleaning work later.
 * * Parent is either a magic dir or a magic regfile.
 * * The function will increase the refcount of the inode and the path of the
 *   parent.
 */
static
struct ofs_inode *ofs_get_parent_hlp(struct ofs_root *root,
				     struct path *pathtgt, struct path *result)
{
	struct inode *ip;
	struct dentry *dp;
	struct path pathp;
	struct ofs_inode *oip;

	if (IS_ROOT(pathtgt->dentry))
		return ERR_PTR(-ENOENT);
	dp = dget_parent(pathtgt->dentry);
	ip = dp->d_inode; /* It's impossible that dp is negative. */
	oip = OFS_INODE(ip);
	pathp.dentry = dp;
	pathp.mnt = mntget(root->magic_root.mnt);
	*result = pathp;
	return oip;
}

/**
 * @brief helper function to get the path and ofs inode from
 *        a ofs inode descriptor
 * @param root: magic ofs that has been registered
 * @param target: ofs inode descriptor
 * @param result: buffer to return the path
 * @return the ofs inode, and the path
 * @retval valid pointer to the ofs inode if OK
 * @retval errno pointer: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * This is a helper function. It is unsafe. It supposes that
 *   the <b><em>root<\b><\em> is registered.
 * * Call this function under locking <b><em>root->rwsem</em></b>.
 * * It is important that to call @ref ofs_put_oi() to do some
 *   cleaning work later.
 * * The function will increase the refcount of the inode and the path of the
 *   parent.
 * @sa ofs_get_oi()
 * @sa ofs_put_oi()
 */
static
struct ofs_inode *ofs_get_oi_hlp(struct ofs_root *root, oid_t target,
				 struct path *result)
{
	struct ofs_rbtree *tree;
	struct ofs_inode *oitgt;
	struct inode *itgt;
	struct dentry *dtgt;
	struct path pathtgt;
	int rc;

	tree = ofs_get_rbtree_by_oid(target);
	oitgt = ofs_rbtree_lookup(tree, target);
	if (unlikely(oitgt == NULL)) {
		ofs_dbg("Can't find ofs inode: {%p, %lu}.\n",
			target.i_sb, target.i_ino);
		rc = -ENOENT;
		goto out_exit;
	}

	itgt = &oitgt->inode;
	if (unlikely(itgt->i_sb != root->sb)) {
		ofs_dbg("The inode {%p, %lu} is not in the magic ofs!\n",
			target.i_sb, target.i_ino);
		rc = -EPERM;
		goto out_iput;
	}
	dtgt = ofs_get_magic_alias(oitgt);
	if (unlikely(dtgt == NULL)) {
		ofs_crt("No magic dentry! Is it removing?\n");
		rc = -EOWNERDEAD;
		goto out_iput;
	}
	ofs_rbtree_oiput(oitgt);
	pathtgt.dentry = dtgt;
	pathtgt.mnt = mntget(root->magic_root.mnt);
	*result = pathtgt;
	return oitgt;

out_iput:
	ofs_rbtree_oiput(oitgt);
out_exit:
	return ERR_PTR(rc);;
}

/**
 * @brief Get the path and ofs inode from the ofs inode descriptor.
 * @param root: magic ofs that has been registered
 * @param target: ofs inode descriptor
 * @param result: buffer to return the result
 * @return the ofs inode, and the path
 * @retval valid pointer to the ofs inode if OK
 * @retval errno pointer: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * It is important that to call @ref ofs_put_oi() to do some
 *   cleaning work later.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_put_oi()
 * @sa ofs_get_oi_hlp()
 */
struct ofs_inode *ofs_get_oi(struct ofs_root *root, oid_t target,
			     struct path *result)
{
	struct ofs_inode *oitgt;

#ifdef CONFIG_OFS_CHKPARAM
	if (unlikely(IS_ERR_OR_NULL(root))) {
		ofs_err("Pointer root is invalid!\n");
		return ERR_PTR(-EINVAL);
	}
	if (unlikely(IS_ERR_OR_NULL(result))) {
		ofs_err("Pointer result is invalid!\n");
		return ERR_PTR(-EINVAL);
	}
#endif

	if (ofs_compare_oid(target, OFS_NULL_OID) == 0)
		target = ofs_get_oid(root->rootoi);

	down_read(&root->rwsem);
	if (unlikely(!root->is_registered)) {
		up_read(&root->rwsem);
		ofs_err("ofs is not registered!\n");
		return ERR_PTR(-EPERM);
	}
	oitgt = ofs_get_oi_hlp(root, target, result);
	if (unlikely(IS_ERR_OR_NULL(oitgt)))
		ofs_err("Can't get the path!\n");
	up_read(&root->rwsem);
	return oitgt;
}
EXPORT_SYMBOL(ofs_get_oi);

/**
 * @brief get the path and ofs inode of parent from a child oid.
 * @param root: magic ofs that has been registered
 * @param target: child oid
 * @param result: buffer to return the result
 * @return the ofs inode, and the path of parent
 * @retval vaild pointer to ofs inode of parent: OK
 * @retval errno pointer: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * It is important that to call @ref ofs_put_parent() to do some
 *   cleaning work later.
 * @sa ofs_get_parent()
 * @sa ofs_put_parent()
 */
struct ofs_inode *ofs_get_parent(struct ofs_root *root, oid_t target,
				 struct path *result)
{
	struct ofs_inode *oitgt, *oip = NULL;
	struct path pathtgt;
	int rc = 0;

#ifdef CONFIG_OFS_CHKPARAM
	if (unlikely(IS_ERR_OR_NULL(root))) {
		ofs_err("Pointer root is invalid!\n");
		return ERR_PTR(-EINVAL);
	}
	if (unlikely(IS_ERR_OR_NULL(result))) {
		ofs_err("Pointer result is invalid!\n");
		return ERR_PTR(-EINVAL);
	}
#endif

	if (ofs_compare_oid(target, OFS_NULL_OID) == 0) {
		rc = -ENOENT;
		goto out_return;
	}

	down_read(&root->rwsem);
	if (unlikely(!root->is_registered)) {
		up_read(&root->rwsem);
		ofs_err("ofs is not registered!\n");
		rc = -EPERM;
		goto out_return;
	}

	oitgt = ofs_get_oi_hlp(root, target, &pathtgt);
	if (unlikely(IS_ERR(oitgt))) {
		ofs_err("Can't get the path!\n");
		rc = PTR_ERR(oitgt);
		goto out_up_read;
	}
	oip = ofs_get_parent_hlp(root, &pathtgt, result);
	ofs_put_oi(oitgt, &pathtgt);

out_up_read:
	up_read(&root->rwsem);
out_return:
	if (rc)
		oip = ERR_PTR(rc);
	return oip;
}
EXPORT_SYMBOL(ofs_get_parent);

/**
 * @brief Get a copy of pathname string and analyze the dirname and basename.
 * @param pathname: path and name string
 * @param dirname: buffer to return the position of dirname in the copy
 * @param basename: buffer to return the position of basename in th copy
 * @return: the copy, and the position of the dirname and basename
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * When finishing to use the copy, @ref ofs_put_pathname() it.
 * * This is a helper function.
 * @sa ofs_put_pathname()
 */
static __always_inline
char *ofs_get_pathname(const char *pathname, char **dirname, char **basename)
{
	char *slash, *pn;
	size_t len;

	len = strlen(pathname);
	if (len == 0)
		return ERR_PTR(-EINVAL);
	if (len >= PATH_MAX)
		return ERR_PTR(-ENAMETOOLONG);

	pn = __getname();
	if (IS_ERR_OR_NULL(pn))
		return pn;
	strcpy(pn, pathname);

	slash = strrchr(pn, '/');
	if (slash) {
		*slash = '\0';
		*dirname = pn;
		*basename = slash + 1;
	} else {
		*basename = pn;
		*dirname = pn + len;
	}
	return pn;
}

/**
 * @brief To put a copy of pathname string.
 * @param pathname: copy to put
 * @note
 * * This is a helper function.
 * @sa ofs_get_pathname()
 */
static __always_inline
void ofs_put_pathname(char *pathname)
{
	__putname(pathname);
}

static
int ofs_mkdir_magic_hlp(struct ofs_inode *oifdr, struct path *pathfdr,
			const char *name, umode_t mode, oid_t *result)
{
	struct dentry *dfdr = pathfdr->dentry, *newd;
	struct inode *ifdr = &oifdr->inode, *newi;
	struct ofs_inode *newoi;
	struct ofs_rbtree *rbtree;
	size_t nlen = strlen(name);
	int rc;

	if (unlikely(nlen > NAME_MAX))
		return -ENAMETOOLONG;
	if (unlikely((nlen == 0)))
		return -EINVAL;

	/*
	 * reference: SYSCALL_DEFINE3(mkdirat, ...) in fs/namei.c
	 */
	rc = mnt_want_write(pathfdr->mnt);
	if (unlikely(rc)) {
		ofs_dbg("Failed to mnt_want_write()! (errno: %d)\n", rc);
		goto out_return;
	}
	mutex_lock_nested(&ifdr->i_mutex, I_MUTEX_PARENT);
	if (dfdr->d_inode != ifdr) { /* d_is_negative(dfdr) */
		/* If the folder is changed before i_mutex locks */
		ofs_dbg("The folder is death!\n");
		rc = -EOWNERDEAD;
		goto out_mutex_unlock;
	}

	/* Find the new dentry */
	newd = lookup_one_len(name, dfdr, nlen); /* dget() the newd */
	if (unlikely(IS_ERR_OR_NULL(newd))) {
		if (newd == NULL)
			rc = -ENOENT;
		else
			rc = PTR_ERR(newd);
		ofs_dbg("Failed to lookup_one_len()! (errno: %d)\n", rc);
		goto out_mutex_unlock;
	}

	/* security hook */
	rc = security_path_mkdir(pathfdr, newd, mode);
	if (rc) {
		ofs_dbg("Failed to security_path_mkdir(). (errno:%d)\n", rc);
		goto out_put_newd;
	}

	/* mkdir */
	oifdr->state |= OI_CREATING_MAGIC_CHILD;
	barrier();
	/* Call vfs_*** to support fsnotify & security hooks. */
	rc = vfs_mkdir(ifdr, newd, mode); /* dget() the newd */
	oifdr->state &= ~OI_CREATING_MAGIC_CHILD;
	if (unlikely(rc)) {
		ofs_dbg("Failed to vfs_mkdir()! (errno: %d)\n", rc);
		goto out_put_newd;
	}

	/* add to rbtree */
	newi = newd->d_inode;
	newoi = OFS_INODE(newi);
	spin_lock(&newi->i_lock);
	newoi->magic = newd;
	newoi->state |= OI_MAGIC;
	spin_unlock(&newi->i_lock);
	rbtree = ofs_get_rbtree(newoi);
	ofs_rbtree_insert(rbtree, newoi);
	*result = ofs_get_oid(newoi);
	rc = 0;

out_put_newd:
	dput(newd); /* dget() the newd twice in total, so dput() it once. */
out_mutex_unlock:
	mutex_unlock(&ifdr->i_mutex);
	mnt_drop_write(pathfdr->mnt);
out_return:
	return rc;
}

/**
 * @brief ofs magic api: Create a magic directory (by a single name string)
 *        in a magic ofs that has been registered.
 * @param root: magic ofs that has been registered
 * @param name: directory name
 * @param mode: authority
 * @param folder: ofs inode descriptor to the folder. (It is a singularity or
 *                a magic directory.) If OFS_NULL_OID, use the default folder
 *                (the root directory of this filesystem).
 * @param result: buffer to return the ofs inode descriptor of the new magic
 *		  directory, if successed.
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * When make a magic directory, its parent is a magic directory or
 *   a singularity.
 * * Support security_hooks.
 * * Support fsnotify.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_mkdir_magic_pn()
 */
int ofs_mkdir_magic(struct ofs_root *root, const char *name, umode_t mode,
		    oid_t folder, oid_t *result)
{
	struct ofs_inode *oifdr;
	struct path pathfdr;
	int rc = 0;

#ifdef CONFIG_OFS_CHKPARAM
	if (unlikely(IS_ERR_OR_NULL(result))) {
		ofs_err("Pointer result is invalid!\n");
		return -EINVAL;
	}
	if (unlikely(IS_ERR_OR_NULL(name))) {
		ofs_err("Pointer name is invalid!\n");
		return -EINVAL;
	}
	if (unlikely(IS_ERR_OR_NULL(root))) {
		ofs_err("Pointer root is invalid!\n");
		return -EINVAL;
	}
#endif

	if (mode == 0)
		mode = OFS_DEFAULT_MODE;
	if (ofs_compare_oid(folder, OFS_NULL_OID) == 0)
		folder = ofs_get_oid(root->rootoi);

	/* all process under read-lock */
	down_read(&root->rwsem);
	if (unlikely(!root->is_registered)) {
		ofs_err("Filesystem is not registered!\n");
		rc = -EPERM;
		goto out_up_read;
	}

	/* find the folder */
	oifdr = ofs_get_folder_hlp(root, folder, &pathfdr);
	if (unlikely(IS_ERR_OR_NULL(oifdr))) {
		rc = PTR_ERR(oifdr);
		ofs_dbg("Can't get the folder! (errno: %d)\n", rc);
		goto out_up_read;
	}

	rc = ofs_mkdir_magic_hlp(oifdr, &pathfdr, name, mode, result);
	ofs_put_folder(oifdr, &pathfdr);

out_up_read:
	up_read(&root->rwsem);
	return rc;
}
EXPORT_SYMBOL(ofs_mkdir_magic);

/*
 * @brief ofs magic api: Create a magic directory (by a path and name
 *        string) in a magic ofs that has been registered.
 * @param root: magic ofs that has been registered
 * @param pathname: directory path and name
 * @param mode: authority
 * @param folder: ofs inode descriptor to the folder. (It is a singularity or
 *                a magic directory.) If OFS_NULL_OID, use the default folder
 *                (the root directory of this filesystem).
 * @param result: buffer to return the ofs inode descriptor of the new magic
 *		  directory, if successed.
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * When make a magic directory, its parent is a magic directory or
 *   a singularity.
 * * Support security_hooks.
 * * Support fsnotify.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_mkdir_magic()
 */
/* int ofs_mkdir_magic_pn(struct ofs_root *root, const char *pathname, */
/* 		       umode_t mode, oid_t folder, oid_t *result) */
/* { */
/* 	struct ofs_inode *oifdr; */
/* 	struct path pathfdr, pathd; */
/* 	int rc = 0; */
/* 	size_t dlen; */
/* 	char *pn, *basename, *dirname; */

/* #ifdef CONFIG_OFS_CHKPARAM */
/* 	if (unlikely(IS_ERR_OR_NULL(result))) { */
/* 		ofs_err("Pointer result is invalid!\n"); */
/* 		return -EINVAL; */
/* 	} */
/* 	if (unlikely(IS_ERR_OR_NULL(pathname))) { */
/* 		ofs_err("Pointer pathname is invalid!\n"); */
/* 		return -EINVAL; */
/* 	} */
/* 	if (unlikely(IS_ERR_OR_NULL(root))) { */
/* 		ofs_err("Pointer root is invalid!\n"); */
/* 		return -EINVAL; */
/* 	} */
/* #endif */

/* 	if (mode == 0) */
/* 		mode = OFS_DEFAULT_MODE; */
/* 	if (ofs_compare_oid(folder, OFS_NULL_OID) == 0) */
/* 		folder = ofs_get_oid(root->rootoi); */

/* 	/\* analyze the path and name string *\/ */
/* 	pn = ofs_get_pathname(pathname, &dirname, &basename); */
/* 	if (unlikely(IS_ERR_OR_NULL(pn))) { */
/* 		if (pn == NULL) */
/* 			rc = -ENOMEM; */
/* 		else */
/* 			rc = PTR_ERR(pn); */
/* 		ofs_dbg("Failed to get pathname! (errno:%d)\n", rc); */
/* 		return rc; */
/* 	} */
/* 	dlen = strlen(dirname); */

/* 	/\* all process under read-lock *\/ */
/* 	down_read(&root->rwsem); */
/* 	if (unlikely(!root->is_registered)) { */
/* 		ofs_err("Filesystem is not registered!\n"); */
/* 		rc = -EPERM; */
/* 		goto out_up_read; */
/* 	} */

/* 	/\* find the parent folder *\/ */
/* 	oifdr = ofs_get_folder_hlp(root, folder, &pathfdr); */
/* 	if (unlikely(IS_ERR_OR_NULL(oifdr))) { */
/* 		rc = PTR_ERR(oifdr); */
/* 		ofs_dbg("Can't get the folder! (errno: %d)\n", rc); */
/* 		goto out_up_read; */
/* 	} */
/* 	if (dlen) { */
/* 		rc = vfs_path_lookup(pathfdr.dentry, pathfdr.mnt, dirname, */
/* 				     LOOKUP_FOLLOW, &pathd); */
/* 		if (rc) { */
/* 			ofs_dbg("Failed to vfs_path_lookup(). errno=%d\n", */
/*				rc); */
/* 			goto out_put_folder; */
/* 		} */
/* 		ofs_put_folder(oifdr, &pathfdr); */
/* 		pathfdr = pathd; */
/* 		oifdr = OFS_INODE(pathfdr.dentry->d_inode); */
/* 	} */

/* 	rc = ofs_mkdir_magic_hlp(oifdr, &pathfdr, basename, mode, result); */

/* out_put_folder: */
/* 	ofs_put_folder(oifdr, &pathfdr); */
/* out_up_read: */
/* 	up_read(&root->rwsem); */
/* 	ofs_put_pathname(pn); */
/* 	return rc; */
/* } */
/* EXPORT_SYMBOL(ofs_mkdir_magic_pn); */

/**
 * @brief helper function to compute the relative path string of two dentry.
 * @param root: magic ofs that has been registered
 * @param src: source
 * @param mode: destination
 * @param buf: buffer to accept the path string
 * @param len: buffer size
 * @return the path string of <b><em>dst</em></b> relate to the
 *         <b><em>src</em></b> and the size (exclude '\0').
 * @retval >0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * The two dentry must be magic and be in the same magic registered ofs.
 * * The two dentry must be dget() before calling this function, because we need
 *   hold the dentry in all process to guard against wild pointer.
 * * This is a helper function. The filesystem <b><em>root</em></b> must be
 *   registered.
 */
static
size_t ofs_compute_path(struct ofs_root *root, struct dentry *src,
			struct dentry *dst, char *buf, size_t len)
{
	size_t b = 0, e = len - 1;
	struct dentry *p = src;
	size_t sz = 0;

	while (p != root->magic_root.dentry) {
		buf[b++] = '.';
		buf[b++] = '.';
		buf[b++] = '/';
		sz += 3;
		p = p->d_parent;
	}
	b -= 3;
	sz -= 3;
	buf[b] = '\0';

	p = dst;
	buf[e] = '\0';
	while (p != root->magic_root.dentry) {
		e -= p->d_name.len;
		if (unlikely(e < b))
			return -ENOBUFS;
		memcpy(&buf[e], p->d_name.name, p->d_name.len);
		buf[--e] = '/';
		sz += p->d_name.len;
		sz ++;
		p = p->d_parent;
	}
	e++;
	sz--;
	memmove(&buf[b], &buf[e], len - e);
	return sz;
}

static
int ofs_symlink_magic_hlp(struct ofs_root *root, struct ofs_inode *oifdr,
			  struct path *pathfdr, oid_t target,
			  const char *name, oid_t *result)
{
	struct dentry *dfdr = pathfdr->dentry, *newd;
	struct inode *ifdr = &oifdr->inode, *newi;
	struct ofs_inode *oitgt;
	struct path pathtgt;
	char *slbuf;
	struct ofs_inode *newoi;
	struct ofs_rbtree *rbtree;
	size_t nlen = strlen(name);
	int rc;

	if (unlikely(nlen > NAME_MAX))
		return -ENAMETOOLONG;
	if (unlikely((nlen == 0)))
		return -EINVAL;

	slbuf = __getname();
	if (IS_ERR_OR_NULL(slbuf)) {
		if (slbuf)
			rc = PTR_ERR(slbuf);
		else
			rc = -ENOMEM;
		ofs_dbg("Can't get symlink buffer. (errno:%d)\n", rc);
		goto out_return;
	}

	/* find the target */
	oitgt = ofs_get_oi_hlp(root, target, &pathtgt);
	if (unlikely(IS_ERR_OR_NULL(oitgt))) {
		rc = PTR_ERR(oitgt);
		ofs_dbg("Can't get oid! (errno: %d)\n", rc);
		goto out_put_slbuf;
	}

	rc = mnt_want_write(pathfdr->mnt);
	if (unlikely(rc)) {
		ofs_dbg("Failed to mnt_want_write()! (errno: %d)\n", rc);
		goto out_put_target;
	}
	mutex_lock_nested(&ifdr->i_mutex, I_MUTEX_PARENT);
	if (ifdr != dfdr->d_inode) { /* d_is_negative(dfdr) */
		/* If the folder is changed before i_mutex locks */
		ofs_dbg("The folder is death!\n");
		rc = -EOWNERDEAD;
		goto out_mutex_unlock;
	}

	/* Find the new dentry */
	newd = lookup_one_len(name, dfdr, nlen); /* dget() the newd. */
	if (unlikely(IS_ERR_OR_NULL(newd))) {
		if (newd == NULL)
			rc = -ENOENT;
		else
			rc = PTR_ERR(newd);
		ofs_dbg("Can't get new dentry! (errno: %d)\n", rc);
		goto out_mutex_unlock;
	}

	/* compute the path string between two dentrys */
	rc = ofs_compute_path(root, newd, pathtgt.dentry, slbuf, PATH_MAX);
	if (unlikely(rc <= 0)) {
		ofs_dbg("Failed to compute path! (errno: %d)\n", rc);
		goto out_put_newd;
	}
	ofs_dbg("symlink target:\"%s\"\n", slbuf);

	/* security hook */
	rc = security_path_symlink(pathfdr, newd, slbuf);
	if (rc) {
		ofs_dbg("Failed in security_path_symlink() (errno:%d)\n", rc);
		goto out_put_newd;
	}

	/* create symlink */
	oifdr->state |= OI_CREATING_MAGIC_CHILD;
	barrier();
	/* Call vfs_*** to support fsnotify & security hooks*/
	rc = (int)vfs_symlink(ifdr, newd, slbuf); /* dget() the newd. */
	oifdr->state &= ~OI_CREATING_MAGIC_CHILD;
	if (unlikely(rc)) {
		ofs_dbg("Failed to vfs_symlink! (errno: %d)\n", rc);
		goto out_put_newd;
	}

	/* add to rbtree */
	newi = newd->d_inode;
	newoi = OFS_INODE(newi);
	spin_lock(&newi->i_lock);
	newoi->magic = newd;
	newoi->state |= OI_MAGIC;
	spin_unlock(&newi->i_lock);
	newoi->symlink = target;
	rbtree = ofs_get_rbtree(newoi);
	ofs_rbtree_insert(rbtree, newoi);
	*result = ofs_get_oid(newoi);
	rc = 0;

out_put_newd:
	dput(newd); /* dget() the newd twice in total, so dput() it once. */
out_mutex_unlock:
	mutex_unlock(&ifdr->i_mutex);
	mnt_drop_write(pathfdr->mnt);
out_put_target:
	ofs_put_oi(oitgt, &pathtgt);
out_put_slbuf:
	__putname(slbuf);
out_return:
	return rc;
}

/**
 * @brief ofs magic api: Create a magic symlink (by a single name string)
 *        in a magic filesystem that has been registered.
 * @param root: magic filesystem that has been registered
 * @param name: symlink name
 * @param mode: authority
 * @param folder: ofs inode descriptor to the folder. (It is a singularity or
 *                a magic directory.) If OFS_NULL_OID, use the default folder
 *                (the root directory of this filesystem).
 * @param target: target of the symlink
 * @param result: buffer to return the ofs inode descriptor of the new magic
 *		  symlink, if successed.
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * When make a magic symbol link, its folder is also magic.
 * * Support security_hooks.
 * * Support fsnotify.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_symlink_magic_hlp()
 */
int ofs_symlink_magic(struct ofs_root *root, const char *name, oid_t folder,
		      oid_t target, oid_t *result)
{
	struct ofs_inode *oifdr;
	struct path pathfdr;
	int rc = 0;

#ifdef CONFIG_OFS_CHKPARAM
	if (unlikely(IS_ERR_OR_NULL(result))) {
		ofs_err("Pointer result is invalid!\n");
		return -EINVAL;
	}
	if (unlikely(IS_ERR_OR_NULL(name))) {
		ofs_err("Pointer name is invalid!\n");
		return -EINVAL;
	}
	if (unlikely(IS_ERR_OR_NULL(root))) {
		ofs_err("Pointer root is invalid!\n");
		return -EINVAL;
	}
#endif

	if (ofs_compare_oid(folder, OFS_NULL_OID) == 0)
		folder = ofs_get_oid(root->rootoi);
	if (ofs_compare_oid(target, OFS_NULL_OID) == 0)
		target = ofs_get_oid(root->rootoi);

	/* all process under read-lock */
	down_read(&root->rwsem);
	if (unlikely(!root->is_registered)) {
		ofs_err("Filesystem is not registered!\n");
		rc = -EPERM;
		goto out_up_read;
	}

	/* find the folder */
	oifdr = ofs_get_folder_hlp(root, folder, &pathfdr);
	if (unlikely(IS_ERR_OR_NULL(oifdr))) {
		rc = PTR_ERR(oifdr);
		ofs_dbg("Can't get folder! (errno: %d)\n", rc);
		goto out_up_read;
	}

	/* create symlink */
	rc = ofs_symlink_magic_hlp(root, oifdr, &pathfdr, target,
				   name, result);
	ofs_put_folder(oifdr, &pathfdr);

out_up_read:
	up_read(&root->rwsem);
	return rc;
}
EXPORT_SYMBOL(ofs_symlink_magic);

/*
 * @brief ofs magic api: Create a magic symlink (by a path and name string)
 *        in a magic ofs that has been registered.
 * @param root: magic ofs that has been registered
 * @param pathname: magic symlink path and name
 * @param mode: authority
 * @param folder: ofs inode descriptor to the folder. (It is a singularity or
 *                a magic directory.) If OFS_NULL_OID, use the default folder
 *                (the root directory of this filesystem).
 * @param target: target of the symlink
 * @param result: buffer to return the ofs inode descriptor of the new magic
 *		  symlink, if successed.
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * When make a magic symbol link, its folder is also magic.
 * * Support security_hooks.
 * * Support fsnotify.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_symlink_magic()
 */
/* int ofs_symlink_magic_pn(struct ofs_root *root, const char *pathname, */
/* 			 oid_t folder, oid_t target, oid_t *result) */
/* { */
/* 	struct ofs_inode *oifdr; */
/* 	struct path pathfdr, pathd; */
/* 	size_t dlen; */
/* 	char *pn, *basename, *dirname; */
/* 	int rc = 0; */

/* #ifdef CONFIG_OFS_CHKPARAM */
/* 	if (unlikely(IS_ERR_OR_NULL(result))) { */
/* 		ofs_err("Pointer result is invalid!\n"); */
/* 		return -EINVAL; */
/* 	} */
/* 	if (unlikely(IS_ERR_OR_NULL(pathname))) { */
/* 		ofs_err("Pointer pathname is invalid!\n"); */
/* 		return -EINVAL; */
/* 	} */
/* 	if (unlikely(IS_ERR_OR_NULL(root))) { */
/* 		ofs_err("Pointer root is invalid!\n"); */
/* 		return -EINVAL; */
/* 	} */
/* #endif */

/* 	if (ofs_compare_oid(folder, OFS_NULL_OID) == 0) */
/* 		folder = ofs_get_oid(root->rootoi); */
/* 	if (ofs_compare_oid(target, OFS_NULL_OID) == 0) */
/* 		target = ofs_get_oid(root->rootoi); */

/* 	/\* analyze the path and name string *\/ */
/* 	pn = ofs_get_pathname(pathname, &dirname, &basename); */
/* 	if (unlikely(IS_ERR_OR_NULL(pn))) { */
/* 		if (pn == NULL) */
/* 			rc = -ENOMEM; */
/* 		else */
/* 			rc = PTR_ERR(pn); */
/* 		ofs_dbg("Failed to get pathname! (errno:%d)\n", rc); */
/* 		goto out_return; */
/* 	} */
/* 	dlen = strlen(dirname); */

/* 	/\* all process under read-lock *\/ */
/* 	down_read(&root->rwsem); */
/* 	if (unlikely(!root->is_registered)) { */
/* 		ofs_err("Filesystem is not registered!\n"); */
/* 		rc = -EPERM; */
/* 		goto out_up_read; */
/* 	} */

/* 	/\* find the folder *\/ */
/* 	oifdr = ofs_get_folder_hlp(root, folder, &pathfdr); */
/* 	if (unlikely(IS_ERR_OR_NULL(oifdr))) { */
/* 		rc = PTR_ERR(oifdr); */
/* 		ofs_dbg("Can't get folder! (errno: %d)\n", rc); */
/* 		goto out_up_read; */
/* 	} */
/* 	if (dlen) { */
/* 		rc = vfs_path_lookup(pathfdr.dentry, pathfdr.mnt, dirname, */
/* 				     LOOKUP_FOLLOW, &pathd); */
/* 		if (rc) { */
/* 			ofs_dbg("Failed to vfs_path_lookup(). errno=%d\n", */
/*				rc); */
/* 			goto out_put_folder; */
/* 		} */
/* 		ofs_put_folder(oifdr, &pathfdr); */
/* 		pathfdr = pathd; */
/* 		oifdr = OFS_INODE(pathfdr.dentry->d_inode); */
/* 	} */

/* 	/\* create symlink *\/ */
/* 	rc = ofs_symlink_magic_hlp(root, oifdr, &pathfdr, target, */
/* 				   basename, result); */

/* out_put_folder: */
/* 	ofs_put_folder(oifdr, &pathfdr); */
/* out_up_read: */
/* 	up_read(&root->rwsem); */
/* 	ofs_put_pathname(pn); */
/* out_return: */
/* 	return rc; */
/* } */
/* EXPORT_SYMBOL(ofs_symlink_magic_pn); */

static
int ofs_create_magic_hlp(struct ofs_inode *oifdr, struct path *pathfdr,
			 const char *name, umode_t mode, bool is_singularity,
			 oid_t *result)
{
	struct dentry *dfdr = pathfdr->dentry, *newd;
	struct inode *ifdr = &oifdr->inode, *newi;
	struct ofs_inode *newoi;
	struct ofs_rbtree *rbtree;
	size_t nlen = strlen(name);
	int rc;

	if (unlikely(nlen > NAME_MAX))
		return -ENAMETOOLONG;
	if (unlikely(nlen == 0))
		return -EINVAL;

	rc = mnt_want_write(pathfdr->mnt);
	if (unlikely(rc)) {
		ofs_dbg("Failed to mnt_want_write()! (errno: %d)\n", rc);
		goto out_return;
	}
	mutex_lock_nested(&ifdr->i_mutex, I_MUTEX_PARENT);
	if (dfdr->d_inode != ifdr) { /* d_is_negative(dfdr) */
		ofs_dbg("The folder is death!\n");
		rc = -EOWNERDEAD;
		goto out_mutex_unlock;
	}

	/* Find the new dentry */
	newd = lookup_one_len(name, dfdr, nlen); /* dget() the newd. */
	if (unlikely(IS_ERR_OR_NULL(newd))) {
		if (newd == NULL)
			rc = -ENOENT;
		else
			rc = PTR_ERR(newd);
		ofs_dbg("Failed to lookup_one_len()! (errno: %d)\n", rc);
		goto out_mutex_unlock;
	}
	rc = security_path_mknod(pathfdr, newd, mode, 0);
	if (rc) {
		ofs_dbg("Failed in security_path_mknod() (errno:%d)\n", rc);
		goto out_put_newd;
	}

	/* create */
	oifdr->state |= OI_CREATING_MAGIC_CHILD;
	if (is_singularity)
		oifdr->state |= OI_CREATING_SINGULARITY;
	barrier();
	/* Call vfs_*** to support fsnotify & security hooks*/
	rc = vfs_create(ifdr, newd, mode, true); /* dget() the newd. */
	oifdr->state &= ~(OI_CREATING_MAGIC_CHILD | OI_CREATING_SINGULARITY);
	if (unlikely(rc)) {
		ofs_dbg("Failed to vfs_create()! (errno: %d)\n", rc);
		goto out_put_newd;
	}

	/* add to rbtree */
	newi = newd->d_inode;
	newoi = OFS_INODE(newi);
	spin_lock(&newi->i_lock);
	newoi->magic = newd;
	newoi->state |= OI_MAGIC;
	spin_unlock(&newi->i_lock);
	rbtree = ofs_get_rbtree(newoi);
	ofs_rbtree_insert(rbtree, newoi);
	*result = ofs_get_oid(newoi);
	rc = 0;

out_put_newd:
	dput(newd); /* dget() the newd twice in total, so dput() it once. */
out_mutex_unlock:
	mutex_unlock(&ifdr->i_mutex);
	mnt_drop_write(pathfdr->mnt);
out_return:
	return rc;
}

/**
 * @brief ofs magic api: Create a magic regfile or singularity
 *        (by a single name string) in a magic ofs that has been registered.
 * @param root: magic ofs that has been registered
 * @param name: regfile name
 * @param mode: authority
 * @param is_magic: Is a magic file ?
 * @param folder: ofs inode descriptor to the folder. (It is a singularity or
 *                a magic directory.) If OFS_NULL_OID, use the default folder
 *                (the root directory of this filesystem).
 * @param result: buffer to return the ofs inode descriptor of the new magic
 *		  regfile, if successed.
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * When make a magic ofs inode, its folder is also magic.
 * * Support security_hooks.
 * * Support fsnotify.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_create_magic_pn()
 */
int ofs_create_magic(struct ofs_root *root, const char *name, umode_t mode,
		     bool is_singularity, oid_t folder, oid_t *result)
{
	struct ofs_inode *oifdr;
	struct path pathfdr;
	int rc = 0;

#ifdef CONFIG_OFS_CHKPARAM
	if (unlikely(IS_ERR_OR_NULL(result))) {
		ofs_err("Pointer result is invalid!\n");
		return -EINVAL;
	}
	if (unlikely(IS_ERR_OR_NULL(name))) {
		ofs_err("Pointer name is invalid!\n");
		return -EINVAL;
	}
	if (unlikely(IS_ERR_OR_NULL(root))) {
		ofs_err("Pointer root is invalid!\n");
		return -EINVAL;
	}
#endif

	if (mode == 0)
		mode = OFS_DEFAULT_MODE;
	if (ofs_compare_oid(folder, OFS_NULL_OID) == 0)
		folder = ofs_get_oid(root->rootoi);

	/* all process under read-lock */
	down_read(&root->rwsem);
	if (unlikely(!root->is_registered)) {
		ofs_err("Filesystem is not registered!\n");
		rc = -EPERM;
		goto out_up_read;
	}

	/* find the folder */
	oifdr = ofs_get_folder_hlp(root, folder, &pathfdr);
	if (unlikely(IS_ERR_OR_NULL(oifdr))) {
		rc = PTR_ERR(oifdr);
		ofs_dbg("Can't get the folder! (errno: %d)\n", rc);
		goto out_up_read;
	}

	/* create */
	rc = ofs_create_magic_hlp(oifdr, &pathfdr, name, mode,
				  is_singularity, result);
	ofs_put_folder(oifdr, &pathfdr);

out_up_read:
	up_read(&root->rwsem);
	return rc;
}
EXPORT_SYMBOL(ofs_create_magic);

/*
 * @brief ofs magic api: Create a magic regfile or singularity
 *        (by a single name string) in a magic ofs that has been registered.
 * @param root: magic ofs that has been registered
 * @param pathname: regfile path and name
 * @param mode: authority
 * @param is_magic: Is a magic file ?
 * @param folder: ofs inode descriptor to the folder. (It is a singularity or
 *                a magic directory.) If OFS_NULL_OID, use the default folder
 *                (the root directory of this filesystem).
 * @param result: buffer to return the ofs inode descriptor of the new magic
 *		  regfile, if successed.
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * When make a magic ofs inode, its folder is also magic.
 * * Support security_hooks.
 * * Support fsnotify.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_create_magic()
 */
/* int ofs_create_magic_pn(struct ofs_root *root, oid_t folder, */
/* 			const char *pathname, umode_t mode, */
/* 			bool is_singularity, oid_t *result) */
/* { */
/* 	struct ofs_inode *oifdr; */
/* 	struct path pathfdr, pathd; */
/* 	size_t dlen; */
/* 	char *pn, *basename, *dirname; */
/* 	int rc; */

/* #ifdef CONFIG_OFS_CHKPARAM */
/* 	if (unlikely(IS_ERR_OR_NULL(result))) { */
/* 		ofs_err("Pointer result is invalid!\n"); */
/* 		return -EINVAL; */
/* 	} */
/* 	if (unlikely(IS_ERR_OR_NULL(pathname))) { */
/* 		ofs_err("Pointer pathname is invalid!\n"); */
/* 		return -EINVAL; */
/* 	} */
/* 	if (unlikely(IS_ERR_OR_NULL(root))) { */
/* 		ofs_err("Pointer root is invalid!\n"); */
/* 		return -EINVAL; */
/* 	} */
/* #endif */

/* 	if (mode == 0) */
/* 		mode = OFS_DEFAULT_MODE; */
/* 	if (ofs_compare_oid(folder, OFS_NULL_OID) == 0) */
/* 		folder = ofs_get_oid(root->rootoi); */

/* 	/\* analyze the path and name string *\/ */
/* 	pn = ofs_get_pathname(pathname, &dirname, &basename); */
/* 	if (unlikely(IS_ERR_OR_NULL(pn))) { */
/* 		if (pn == NULL) */
/* 			rc = -ENOMEM; */
/* 		else */
/* 			rc = PTR_ERR(pn); */
/* 		ofs_dbg("Failed to get pathname! (errno:%d)\n", rc); */
/* 		return rc; */
/* 	} */
/* 	dlen = strlen(dirname); */

/* 	/\* all process under read-lock *\/ */
/* 	down_read(&root->rwsem); */
/* 	if (unlikely(!root->is_registered)) { */
/* 		ofs_err("Filesystem is not registered!\n"); */
/* 		rc = -EPERM; */
/* 		goto out_up_read; */
/* 	} */

/* 	/\* find the folder *\/ */
/* 	oifdr = ofs_get_folder_hlp(root, folder, &pathfdr); */
/* 	if (unlikely(IS_ERR_OR_NULL(oifdr))) { */
/* 		rc = PTR_ERR(oifdr); */
/* 		ofs_dbg("Can't get the folder! (errno: %d)\n", rc); */
/* 		goto out_up_read; */
/* 	} */
/* 	if (dlen) { */
/* 		rc = vfs_path_lookup(pathfdr.dentry, pathfdr.mnt, dirname, */
/* 				     LOOKUP_FOLLOW, &pathd); */
/* 		if (rc) { */
/* 			ofs_dbg("Failed to vfs_path_lookup(). errno=%d\n", */
/* 				    rc); */
/* 			goto out_put_folder; */
/* 		} */
/* 		ofs_put_folder(oifdr, &pathfdr); */
/* 		pathfdr = pathd; */
/* 		oifdr = OFS_INODE(pathfdr.dentry->d_inode); */
/* 	} */

/* 	/\* create *\/ */
/* 	rc = ofs_create_magic_hlp(oifdr, &pathfdr, basename, mode, */
/* 				  is_singularity, result); */

/* out_put_folder: */
/* 	ofs_put_folder(oifdr, &pathfdr); */
/* out_up_read: */
/* 	up_read(&root->rwsem); */
/* 	ofs_put_pathname(pn); */
/* 	return rc; */
/* } */
/* EXPORT_SYMBOL(ofs_create_magic_pn); */

static
int ofs_rmdir_magic_hlp(struct ofs_inode *oitgt, struct path *pathtgt,
			struct ofs_inode *oip, struct path *pathp)
{
	int rc;
	struct inode *ip = &oip->inode, *itgt = &oitgt->inode;
	struct dentry *dp = pathp->dentry, *dtgt = pathtgt->dentry;

	/* To prevent from rmdir()ing a singularity. Because a singularity is a
	   S_IFREG inode whose dentry is with DCACHE_DIRECTORY_TYPE*/
	if (!S_ISDIR(oitgt->inode.i_mode)) {
		rc = -EPERM;
		ofs_err("The ofs inode is not a directory\n");
		goto out_return;
	}

	rc = mnt_want_write(pathp->mnt);
	if (unlikely(rc)) {
		ofs_dbg("Failed to mnt_want_write()! (errno: %d)\n", rc);
		goto out_return;
	}

	/* remove target */
	mutex_lock_nested(&ip->i_mutex, I_MUTEX_PARENT);
	/* Get dentry and dp before mutex_lock. So it needs check. */
	if (dp->d_inode != ip) {
		ofs_dbg("The parent is death!\n");
		rc = -EOWNERDEAD;
		goto out_mutex_unlock;
	}
	if ((dtgt->d_inode != itgt)) { /* d_is_negative(dtgt) */
		rc = -ENOENT;
		ofs_dbg("The target directory is dead!\n");
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
	oip->state |= OI_REMOVING_MAGIC_CHILD;
	barrier(); /* To prevent compiler from optimizing. */
	rc = vfs_rmdir(ip, dtgt);
	oip->state &= ~OI_REMOVING_MAGIC_CHILD;

out_mutex_unlock:
	mutex_unlock(&ip->i_mutex);
	mnt_drop_write(pathp->mnt);
out_return:
	return rc;
}

/**
 * @brief ofs magic api: remove a magic directory
 * @param root: magic ofs that has been registered
 * @param target: ofs inode descriptor to the directory.
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * Support security_hooks.
 * * Support fsnotify.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * * The magic directory must be empty.
 */
int ofs_rmdir_magic(struct ofs_root *root, oid_t target)
{
	struct path pathtgt, pathp = {NULL, NULL};
	struct ofs_inode *oitgt, *oip;
	int rc;

#ifdef CONFIG_OFS_CHKPARAM
	if (ofs_compare_oid(target, OFS_NULL_OID) == 0) {
		ofs_err("Can't remove root directory.");
		return -EBUSY;
	}
	if (unlikely(IS_ERR_OR_NULL(root))) {
		ofs_err("Pointer root is invalid!\n");
		return -EFAULT;
	}
#endif

	/* all process under read-lock */
	down_read(&root->rwsem);
	if (unlikely(!root->is_registered)) {
		ofs_err("Filesystem is not registered!\n");
		rc = -EPERM;
		goto out_up_read;
	}

	/* get target path to remove */
	oitgt = ofs_get_oi_hlp(root, target, &pathtgt);
	if (unlikely(IS_ERR_OR_NULL(oitgt))) {
		rc = PTR_ERR(oitgt);
		ofs_dbg("Can't get the target! (errno: %d)\n", rc);
		goto out_up_read;
	}

	/* get parent folder */
	oip = ofs_get_parent_hlp(root, &pathtgt, &pathp);
	if (unlikely(IS_ERR_OR_NULL(oip))) {
		rc = PTR_ERR(oip);
		ofs_dbg("Can't get the parent! (errno: %d)\n", rc);
		goto out_put_oi;
	}

	/* rmdir */
	rc = ofs_rmdir_magic_hlp(oitgt, &pathtgt, oip, &pathp);

out_put_oi:
	ofs_put_oi(oitgt, &pathtgt);
	if (pathp.dentry)
		ofs_put_parent(oip, &pathp);
out_up_read:
	up_read(&root->rwsem);
	return rc;
}
EXPORT_SYMBOL(ofs_rmdir_magic);

static
int ofs_unlink_magic_hlp(struct ofs_inode *oitgt, struct path *pathtgt,
			 struct ofs_inode *oip, struct path *pathp)
{
	int rc;
	struct inode *idelegated;
	struct inode *ip = &oip->inode, *itgt = &oitgt->inode;
	struct dentry *dp = pathp->dentry, *dtgt = pathtgt->dentry;

	rc = mnt_want_write(pathp->mnt);
	if (unlikely(rc)) {
		ofs_dbg("Failed to mnt_want_write()! (errno: %d)\n", rc);
		goto out_return;
	}

	/* remove target */
retry_deleg:
	mutex_lock_nested(&ip->i_mutex, I_MUTEX_PARENT);
	/* Get dtgt and dp before mutex_lock. So it needs check. */
	if (dp->d_inode != ip) {
		ofs_dbg("The parent is death!\n");
		rc = -EOWNERDEAD;
		goto out_mutex_unlock;
	}
	if ((dtgt->d_inode != itgt)) { /* d_is_negative(dtgt) */
		rc = -ENOENT;
		ofs_dbg("The target is dead!\n");
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

	idelegated = NULL;
	oip->state |= OI_REMOVING_MAGIC_CHILD;
	barrier(); /* To prevent compiler from optimizing. */
	rc = vfs_unlink(ip, dtgt, &idelegated);
	oip->state &= ~OI_REMOVING_MAGIC_CHILD;
	if (idelegated) {
		mutex_unlock(&ip->i_mutex);
		rc = break_deleg_wait(&idelegated);
		if (rc) {
			ofs_dbg("Failed to break_deleg_wait(). errno:%d\n", rc);
			goto out_mnt_drop_write;
		}
		goto retry_deleg;
	}

out_mutex_unlock:
	mutex_unlock(&ip->i_mutex);
out_mnt_drop_write:
	mnt_drop_write(pathp->mnt);
out_return:
	return rc;
}

/**
 * @brief ofs magic api: unlink a magic ofs inode
 * @param root: magic ofs that has been registered
 * @param target: ofs inode descriptor to unlink.
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * Support security_hooks.
 * * Support fsnotify.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 */
int ofs_unlink_magic(struct ofs_root *root, oid_t target)
{
	struct path pathtgt, pathp = {NULL, NULL};
	struct ofs_inode *oitgt, *oip;
	int rc = 0;

#ifdef CONFIG_OFS_CHKPARAM
	if (ofs_compare_oid(target, OFS_NULL_OID) == 0) {
		ofs_err("Can't remove root directory.");
		return -EISDIR;
	}
	if (unlikely(IS_ERR_OR_NULL(root))) {
		ofs_err("Pointer root is invalid!\n");
		return -EFAULT;
	}
#endif

	/* all process under read-lock */
	down_read(&root->rwsem);
	if (unlikely(!root->is_registered)) {
		ofs_err("Filesystem is not registered!\n");
		rc = -EPERM;
		goto out_up_read;
	}

	/* get target to remove */
	oitgt = ofs_get_oi_hlp(root, target, &pathtgt);
	if (unlikely(IS_ERR_OR_NULL(oitgt))) {
		rc = PTR_ERR(oitgt);
		ofs_dbg("Can't get the oid! (errno: %d)\n", rc);
		goto out_up_read;
	}
	ofs_dbg("target oitgt<%p>->inode<%p>\n", oitgt, &oitgt->inode);

	/* get parent folder */
	oip = ofs_get_parent_hlp(root, &pathtgt, &pathp);
	if (unlikely(IS_ERR_OR_NULL(oip))) {
		rc = PTR_ERR(oip);
		ofs_dbg("Can't get the parent! (errno: %d)\n", rc);
		goto out_put_oi;
	}

	/* prepare parent */
	rc = ofs_unlink_magic_hlp(oitgt, &pathtgt, oip, &pathp);

out_put_oi:
	ofs_put_oi(oitgt, &pathtgt);
	if (pathp.dentry)
		ofs_put_parent(oip, &pathp);
out_up_read:
	up_read(&root->rwsem);
	return rc;
}
EXPORT_SYMBOL(ofs_unlink_magic);

static inline
int ofs_may_delete_singularity(struct inode *ip, struct inode *itgt,
			       struct dentry *dtgt)
{
	int rc;

	/* Not in syscall context */
	/* audit_inode_child(ip, dtgt, AUDIT_TYPE_CHILD_DELETE); */
	rc = inode_permission(ip, MAY_WRITE | MAY_EXEC);
	if (rc)
		return rc;
	if (IS_APPEND(ip) || IS_APPEND(itgt) || IS_IMMUTABLE(itgt) ||
	    IS_SWAPFILE(itgt) || IS_ROOT(dtgt)) {
		BUG();
		return -EPERM;
	}
	return 0;
}

/**
 * @brief ofs magic api: helper function to remove a singularity
 * @param oitgt: target singularity
 * @param pathtgt: path of target
 * @param oip: parent ofs_inode of target
 * @param pathp: parent path
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * If there are other dentrys (hard links) of the singularity,
 *   can't remove it.
 * * Can't rm a singularity when it has child.
 * * Support security_hooks.
 * * Support fsnotify.
 * @sa ofs_rm_singularity()
 * @sa ofs_rm_magic()
 */
static
int ofs_rm_singularity_hlp(struct ofs_inode *oitgt, struct path *pathtgt,
			   struct ofs_inode *oip, struct path *pathp)
{
	struct inode *idelegated;
	struct inode *ip = &oip->inode, *itgt = &oitgt->inode;
	struct dentry *dp = pathp->dentry, *dtgt = pathtgt->dentry;
	int rc;

	/* A singularity is a S_IFREG inode with
	   a DCACHE_DIRECTORY_TYPE dentry. */
	if (unlikely(!S_ISREG(itgt->i_mode))) {
		rc = -EPERM;
		ofs_dbg("The ofs inode is not a singularity.\n");
		goto out_return;
	}

	rc = mnt_want_write(pathp->mnt);
	if (unlikely(rc)) {
		ofs_dbg("Failed to mnt_want_write()! (errno: %d)\n", rc);
		goto out_return;
	}

retry_deleg:
	mutex_lock_nested(&ip->i_mutex, I_MUTEX_PARENT);
	/* Get dtgt and dp before mutex_lock. So it needs check. */
	if (IS_DEADDIR(ip) || (dp->d_inode != ip)) {
		rc = -EOWNERDEAD;
		ofs_dbg("The parent is dead!\n");
		goto out_mutex_unlock_parent;
	}
	if (dtgt->d_parent != dp) {
		ofs_dbg("Child changed!\n");
		rc = -ECHILD;
		goto out_mutex_unlock_parent;
	}

	rc = security_path_unlink(pathp, dtgt);
	if (rc) {
		ofs_dbg("Failed in security_path_unlink() (errno: %d)\n", rc);
		goto out_mutex_unlock_parent;
	}

	rc = ofs_may_delete_singularity(ip, itgt, dtgt);
	if (rc) {
		ofs_dbg("Failed in ofs_may_delete_singularity()! (errno:%d)\n",
			rc);
		goto out_mutex_unlock_parent;
	}

	mutex_lock_nested(&itgt->i_mutex, I_MUTEX_NORMAL);
	if (dtgt->d_inode != itgt) { /* d_is_negative(dtgt) */
		rc = -ENOENT;
		ofs_dbg("The singularity is already removed!\n");
		goto out_mutex_unlock_target;
	}
	if (!(oitgt->state & OI_SINGULARITY)) {
		rc = -EPERM;
		ofs_err("The ofs inode is not a singularity!\n");
		goto out_mutex_unlock_target;
	}
	if (itgt->i_nlink > 2) {
		rc = -EBUSY;
		ofs_err("The singularity has other hard link.\n");
		goto out_mutex_unlock_target;
	}
	if (!simple_empty(dtgt)) {
		rc = -ENOTEMPTY;
		ofs_err("The singularity has child.\n");
		goto out_mutex_unlock_target;
	}
	BUG_ON(d_mountpoint(dtgt));

	rc = __ksym__security_inode_unlink(ip, dtgt);
	if (rc) {
		ofs_dbg("Failed to security_inode_unlink_ksym()! (errno:%d)\n",
			rc);
		goto out_mutex_unlock_target;
	}
	rc = try_break_deleg(itgt, &idelegated);
	if (rc && idelegated) {
		mutex_unlock(&itgt->i_mutex);
		mutex_unlock(&ip->i_mutex);
		rc = break_deleg_wait(&idelegated);
		if (rc) {
			ofs_dbg("Failed in break_deleg_wait()! "
				"(errno %d)\n", rc);
			goto out_mnt_drop_write;
		}
		goto retry_deleg;
	}

	idelegated = NULL;
	oip->state |= (OI_REMOVING_MAGIC_CHILD | OI_REMOVING_SINGULARITY);
	barrier(); /* To prevent compiler from optimizing. */
	spin_lock(&dtgt->d_lock);
	__d_set_type(dtgt, DCACHE_FILE_TYPE); /* back to regfile */
	dentry_rcuwalk_barrier(dtgt);
	spin_unlock(&dtgt->d_lock);
	itgt->__i_nlink --;
	rc = ip->i_op->unlink(ip, dtgt);
	oip->state &= ~(OI_REMOVING_MAGIC_CHILD | OI_REMOVING_SINGULARITY);
	if (!rc) {
		itgt->i_flags |= S_DEAD;
	} else {
		ofs_dbg("Failed in callback unlink()! (errno:%d)\n", rc);
		BUG();
	}

out_mutex_unlock_target:
	mutex_unlock(&itgt->i_mutex);
	if (!rc) {
		fsnotify_link_count(itgt);
		d_delete(dtgt);
	}
out_mutex_unlock_parent:
	mutex_unlock(&ip->i_mutex);
out_mnt_drop_write:
	mnt_drop_write(pathp->mnt);
out_return:
	return rc;
}

/**
 * @brief ofs magic api: remove a singularity
 * @param root: magic ofs that has been registered
 * @param target: ofs inode descriptor to remove.
 * @retval 0: OK
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @sa ofs_rm_singularity_hlp()
 */
int ofs_rm_singularity(struct ofs_root *root, oid_t target)
{
	struct path pathtgt, pathp = {NULL, NULL};
	struct ofs_inode *oitgt, *oip;
	int rc = 0;

#ifdef CONFIG_OFS_CHKPARAM
	if (ofs_compare_oid(target, OFS_NULL_OID) == 0) {
		ofs_err("Can't remove root directory.");
		return -EISDIR;
	}
	if (unlikely(IS_ERR_OR_NULL(root))) {
		ofs_err("Pointer root is invalid!\n");
		return -EFAULT;
	}
#endif

	/* all process under read-lock */
	down_read(&root->rwsem);
	if (unlikely(!root->is_registered)) {
		ofs_dbg("Filesystem is not registered!\n");
		rc = -EPERM;
		goto out_up_read;
	}

	/* get target to remove */
	oitgt = ofs_get_oi_hlp(root, target, &pathtgt);
	if (unlikely(IS_ERR_OR_NULL(oitgt))) {
		rc = PTR_ERR(oitgt);
		ofs_dbg("Can't get the oid! (errno: %d)\n", rc);
		goto out_up_read;
	}

	/* get parent folder */
	oip = ofs_get_parent_hlp(root, &pathtgt, &pathp);
	if (unlikely(IS_ERR_OR_NULL(oip))) {
		rc = PTR_ERR(oip);
		ofs_dbg("Can't get the parent! (errno: %d)\n", rc);
		goto out_put_oi;
	}

	/* rm singularity */
	rc = ofs_rm_singularity_hlp(oitgt, &pathtgt, oip, &pathp);
out_put_oi:
	ofs_put_oi(oitgt, &pathtgt);
	if (pathp.dentry)
		ofs_put_parent(oip, &pathp);
out_up_read:
	up_read(&root->rwsem);
	return rc;
}
EXPORT_SYMBOL(ofs_rm_singularity);

static
int ofs_rm_recursively_hlp(struct ofs_inode *oitgt, struct path *pathtgt,
			   struct ofs_inode *oip, struct path *pathp)
{
	struct dentry *dtgt = pathtgt->dentry, *dchild;
	struct inode *itgt = &oitgt->inode, *ichild;
	struct path pathc;
	struct ofs_inode *oichild;
	struct list_head *next;
	int rc;

	/* A folder is a normal directory, a magic directory and
	   a singularity. */
	if (oitgt->magic) {
		if (!S_ISDIR(itgt->i_mode) && !(oitgt->state & OI_SINGULARITY))
			return ofs_unlink_magic_hlp(oitgt, pathtgt, oip, pathp);
	} else {
		if (!S_ISDIR(itgt->i_mode))
			return ofs_unlink_normal_hlp(oip, pathp, dtgt);
	}

	rc = 0;
	spin_lock(&dtgt->d_lock);
	next = dtgt->d_subdirs.next;
	/* Don't use list_for_each_entry, because it need change `next' at
	   beginning. */
	while (next != &dtgt->d_subdirs) {
		dchild = list_entry(next, struct dentry, d_child);
		next = next->next;
		spin_lock_nested(&dchild->d_lock, DENTRY_D_LOCK_NESTED);
		if (ofs_simple_positive(dchild)) {
			dchild->d_lockref.count++;
			ichild = dchild->d_inode;
			spin_unlock(&dchild->d_lock);
			spin_unlock(&dtgt->d_lock);
			pathc.dentry = dchild;
			pathc.mnt = mntget(pathtgt->mnt);
			oichild = OFS_INODE(ichild);
			rc = ofs_rm_recursively_hlp(oichild, &pathc,
						    oitgt, pathtgt);
			ofs_put_oi(oichild, &pathc);
			spin_lock(&dtgt->d_lock);
			if (rc)
				break;
		} else {
			spin_unlock(&dchild->d_lock);
		}
	}
	spin_unlock(&dtgt->d_lock);

	if (!rc) {
		if (oitgt->magic) {
			if (S_ISDIR(itgt->i_mode))
				rc = ofs_rmdir_magic_hlp(oitgt, pathtgt,
							 oip, pathp);
			else
				rc = ofs_rm_singularity_hlp(oitgt, pathtgt,
							    oip, pathp);
		} else {
			rc = ofs_rmdir_normal_hlp(oip, pathp, dtgt);
		}
	}
	return rc;
}

static
int ofs_rm_magic_hlp(struct ofs_inode *oitgt, struct path *pathtgt,
		     struct ofs_inode *oip, struct path *pathp, bool r)
{
	if (r) {
		return ofs_rm_recursively_hlp(oitgt, pathtgt, oip, pathp);
	} else {
		if (S_ISDIR(oitgt->inode.i_mode))
			return ofs_rmdir_magic_hlp(oitgt, pathtgt, oip, pathp);
		else if (S_ISREG(oitgt->inode.i_mode) &&
			 (oitgt->state & OI_SINGULARITY))
			return ofs_rm_singularity_hlp(oitgt, pathtgt, oip,
						      pathp);
		else
			return ofs_unlink_magic_hlp(oitgt, pathtgt, oip, pathp);
	}
}

int ofs_rm_magic(struct ofs_root *root, oid_t target, bool r)
{
	struct path pathtgt, pathp = {NULL, NULL};
	struct ofs_inode *oitgt, *oip;
	int rc = 0;

#ifdef CONFIG_OFS_CHKPARAM
	if (ofs_compare_oid(target, OFS_NULL_OID) == 0) {
		ofs_err("Can't remove root directory.");
		return -EISDIR;
	}
	if (unlikely(IS_ERR_OR_NULL(root))) {
		ofs_err("Pointer root is invalid!\n");
		return -EFAULT;
	}
#endif

	/* all process under read-lock */
	down_read(&root->rwsem);
	if (unlikely(!root->is_registered)) {
		ofs_dbg("Filesystem is not registered!\n");
		rc = -EPERM;
		goto out_up_read;
	}

	/* get target to remove */
	oitgt = ofs_get_oi_hlp(root, target, &pathtgt);
	if (unlikely(IS_ERR_OR_NULL(oitgt))) {
		rc = PTR_ERR(oitgt);
		ofs_dbg("Can't get the target! (errno: %d)\n", rc);
		goto out_up_read;
	}

	/* get parent folder */
	oip = ofs_get_parent_hlp(root, &pathtgt, &pathp);
	if (unlikely(IS_ERR_OR_NULL(oip))) {
		rc = PTR_ERR(oip);
		ofs_dbg("Can't get the parent! (errno: %d)\n", rc);
		goto out_put_oi;
	}

	/* rm */
	rc = ofs_rm_magic_hlp(oitgt, &pathtgt, oip, &pathp, r);

out_put_oi:
	ofs_put_oi(oitgt, &pathtgt);
	if (pathp.dentry)
		ofs_put_parent(oip, &pathp);
out_up_read:
	up_read(&root->rwsem);
	return rc;
}
EXPORT_SYMBOL(ofs_rm_magic);

int ofs_rename_magic_hlp(struct ofs_inode *oitgt, struct path *pathtgt,
			 struct ofs_inode *oip, struct path *pathp,
			 struct ofs_inode *oinewfdr, struct path *pathnewfdr,
			 const char *newname, unsigned int flags)
{
	struct inode *idelegated;
	struct dentry *trap, *newd;
	struct dentry *dp = pathp->dentry;
	struct dentry *dnewfdr = pathnewfdr->dentry;
	struct dentry *dtgt = pathtgt->dentry;
	struct inode *ip = &oip->inode;
	struct inode *inewfdr = &oinewfdr->inode;
	size_t nlen = strlen(newname);
	int rc;

	if (unlikely(nlen > NAME_MAX))
		return -ENAMETOOLONG;
	if (unlikely(nlen == 0))
		return -EINVAL;
	if (flags & ~(RENAME_NOREPLACE | RENAME_EXCHANGE))
		return -EINVAL;
	if (flags & (RENAME_NOREPLACE | RENAME_EXCHANGE))
		return -EINVAL;

	rc = mnt_want_write(pathp->mnt);
	if (unlikely(rc)) {
		ofs_dbg("Failed to mnt_want_write()! (errno: %d)\n", rc);
		goto out_return;
	}

retry_deleg:
	trap = lock_rename(dnewfdr, dp);
	/* Get dtgt and dp before mutex_lock. So it needs check. */
	if ((dp->d_inode != ip)) { /* d_is_negative(dp) */
		rc = -EOWNERDEAD;
		ofs_dbg("The parent is dead!\n");
		goto out_unlock_rename;
	}
	if ((dnewfdr->d_inode != inewfdr)) { /* d_is_negative(dnewfdr) */
		rc = -ENOENT;
		ofs_dbg("The folder is dead!\n");
		goto out_unlock_rename;
	}
	if (dtgt->d_parent != dp) {
		ofs_dbg("Child changed!\n");
		rc = -ECHILD;
		goto out_unlock_rename;
	}

	newd = lookup_one_len(newname, dnewfdr, nlen); /* dget() the newd. */
	if (unlikely(IS_ERR_OR_NULL(newd))) {
		if (newd == NULL)
			rc = -ENOENT;
		else
			rc = PTR_ERR(newd);
		ofs_dbg("Failed to lookup_one_len()! (errno: %d)\n", rc);
		goto out_unlock_rename;
	}
	if ((flags & RENAME_NOREPLACE) && d_is_positive(newd)) {
		ofs_dbg("Target exists.\n");
		rc = -EEXIST;
		goto out_put_newd;
	}
	if ((flags & RENAME_EXCHANGE) && (d_is_negative(newd))) {
		ofs_dbg("Target doesn't exists.\n");
		rc = -ENOENT;
		goto out_put_newd;
	}
	if (!d_is_dir(dtgt)) {
		if (!(flags & RENAME_EXCHANGE) && (d_is_dir(newd))) {
			rc = -ENOTDIR;
			goto out_put_newd;
		}
	}
	if ((dtgt == trap) || (newd == trap)) {
		ofs_dbg("Source and target should not be ancestor "
			"of each other.\n");
		rc = -EINVAL;
		goto out_put_newd;
	}

	rc = security_path_rename(pathp, dtgt, pathnewfdr, newd, flags);
	if (rc) {
		ofs_dbg("Failed in security_path_rename() (errno: %d)\n", rc);
		goto out_put_newd;
	}

	idelegated = NULL;
	oip->state |= OI_RENAMING_MAGIC_CHILD;
	barrier(); /* To prevent compiler from optimizing. */
	rc = vfs_rename(ip, dtgt, inewfdr, newd, &idelegated, flags);
	if (rc) {
		ofs_dbg("Failed to vfs_rename()! (errno:%d)\n", rc);
		goto out_put_newd;
	}
	oip->state &= ~OI_RENAMING_MAGIC_CHILD;
	if (idelegated) {
		dput(newd);
		unlock_rename(dnewfdr, dp);
		rc = break_deleg_wait(&idelegated);
		if (rc) {
			ofs_dbg("Failed to break_deleg_wait(). "
				"errno:%d\n", rc);
			goto out_mnt_drop_write;
		}
		goto retry_deleg;
	}

out_put_newd:
	dput(newd);
out_unlock_rename:
	unlock_rename(dnewfdr, dp);
out_mnt_drop_write:
	mnt_drop_write(pathp->mnt);
out_return:
	return rc;
}

int ofs_rename_magic(struct ofs_root *root, oid_t target, oid_t newfdr,
		     const char *newname, unsigned int flags)
{
	struct ofs_inode *oitgt, *oinewfdr, *oip;
	struct path pathnewfdr, pathtgt, pathp = {NULL, NULL};
	int rc = 0;

#ifdef CONFIG_OFS_CHKPARAM
	if (unlikely(IS_ERR_OR_NULL(newname))) {
		ofs_err("Pointer name is invalid!\n");
		return -EINVAL;
	}
	if (unlikely(IS_ERR_OR_NULL(root))) {
		ofs_err("Pointer root is invalid!\n");
		return -EINVAL;
	}
	if (ofs_compare_oid(target, OFS_NULL_OID) == 0) {
		ofs_err("Can't remove root directory.");
		return -EISDIR;
	}
#endif

	if (ofs_compare_oid(newfdr, OFS_NULL_OID) == 0)
		newfdr = ofs_get_oid(root->rootoi);

	/* all process under read-lock */
	down_read(&root->rwsem);
	if (unlikely(!root->is_registered)) {
		ofs_err("Filesystem is not registered!\n");
		rc = -EPERM;
		goto out_up_read;
	}

	oitgt = ofs_get_oi_hlp(root, target, &pathtgt);
	if (unlikely(IS_ERR_OR_NULL(oitgt))) {
		rc = PTR_ERR(oitgt);
		ofs_dbg("Can't get the oid! (errno: %d)\n", rc);
		goto out_up_read;
	}

	oip = ofs_get_parent_hlp(root, &pathtgt, &pathp);
	if (unlikely(IS_ERR_OR_NULL(oip))) {
		rc = PTR_ERR(oip);
		ofs_dbg("Can't get the parent! (errno: %d)\n", rc);
		goto out_put_oi;
	}

	/* find the folder */
	oinewfdr = ofs_get_folder_hlp(root, newfdr, &pathnewfdr);
	if (unlikely(IS_ERR_OR_NULL(oinewfdr))) {
		rc = PTR_ERR(oinewfdr);
		ofs_dbg("Can't get the folder! (errno: %d)\n", rc);
		goto out_put_oi;
	}

	rc = ofs_rename_magic_hlp(oitgt, &pathtgt, oip, &pathp, oinewfdr,
				  &pathnewfdr, newname, flags);
	ofs_put_folder(oinewfdr, &pathnewfdr);

out_put_oi:
	ofs_put_oi(oitgt, &pathtgt);
	if (pathp.dentry)
		ofs_put_parent(oip, &pathp);
out_up_read:
	up_read(&root->rwsem);
	return rc;
}

/**
 * @brief ofs magic api: Change the operations of a magic ofs inode.
 * @param oitgt: ofs inode
 * @param iop: the new inode operations.
 * @param fop: the new file operations.
 * @retval 0: if successed.
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * TODO
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 */
int ofs_set_operations(struct ofs_inode *oitgt,
		       const struct ofs_operations *ops)
{
#ifdef CONFIG_OFS_CHKPARAM
	if (unlikely(IS_ERR_OR_NULL(oitgt))) {
		ofs_dbg("Pointer oi is NULL.\n");
		return -EINVAL;
	}
	if (unlikely(!(oitgt->magic))) {
		ofs_dbg("itgt<%p> is not magic.\n", &oitgt->inode);
		return -EPERM;
	}
#endif

	/* set operations */
	/* TODO */
	return -ENOSYS;
}
EXPORT_SYMBOL(ofs_set_operations);

/******** ofs namespace ********/
int ofs_follow_link_magic(struct ofs_inode *oi, struct ofs_nameidata *nd)
{
	/* TODO */
	return -ENOSYS;
}
EXPORT_SYMBOL(ofs_follow_link_magic);

void ofs_put_link_magic(struct ofs_inode *oi, struct ofs_nameidata *nd)
{
	/* TODO */
}
EXPORT_SYMBOL(ofs_put_link_magic);
