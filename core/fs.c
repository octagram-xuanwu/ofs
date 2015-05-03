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
 * C source about filesystem of ofs. 1 tab == 8 spaces.
 */

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********    header files   ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#include "ofs.h"
#include <linux/version.h>
#include <linux/kconfig.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vfs.h>
#include <linux/fs.h>
#include <linux/dcache.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/file.h>
#include <linux/seq_file.h>
#include <linux/parser.h>
#include <linux/percpu-defs.h>
#include <linux/backing-dev.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/spinlock.h>
#include <linux/limits.h>
#include <linux/fs_struct.h>
#include <asm/uaccess.h>

#include "fs.h"
#include "rbtree.h"
#include "log.h"

#ifdef CONFIG_OFS_SYSFS
#include "omsys.h"
#endif

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       macro       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/**
 * @brief ofs magic
 */
#define OFS_MAGIC_NUM       	0x706673 /* ofs */

/**
 * @brief a batch of ofs inode number for per-cpu
 */
#define OFS_INO_BATCH 1024

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       types       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/**
 * @brief All mount option
 */
enum ofs_option {
	OPT_MODE,	/**< option "mode=%o" */
	OPT_UID,	/**< option "uid=%d" */
	OPT_GID,	/**< option "gid=%d" */
	OPT_MAGIC,	/**< option "magic" */
	OPT_ERR,	/**< error option */
};

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********         function prototypes         ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ********  mount ******** ********/
static
struct dentry *ofs_mount(struct file_system_type *fst, int flags,
			 const char *dev_name, void *data);

static
void ofs_kill_sb(struct super_block *psb);

/******** ******** super ******** ********/
static
struct inode *ofs_sops_alloc_inode(struct super_block *sb);

static
void ofs_destroy_inode_rcu_callback(struct rcu_head *head);

static
void ofs_sops_destroy_inode(struct inode *inode);

static
int ofs_sops_drop_inode(struct inode *inode);

static
void ofs_sops_put_super(struct super_block *sb);

static
int ofs_sops_statfs(struct dentry *dentry, struct kstatfs *buf);

static
int ofs_sops_remount_fs(struct super_block *sb, int *flags, char *data);
static
int ofs_sops_show_options(struct seq_file *seq, struct dentry *rootd);

/******** ******** dentry_operations ******** ********/
static
int ofs_dops_delete_dentry(const struct dentry *dentry);

/******** ******** memory mapping ******** ********/
static
int ofs_set_page_dirty_no_writeback(struct page *page);

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********   .sdata & .data  ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** mount ******** ********/
static const match_table_t ofs_tokens = {
	{OPT_MODE, "mode=%o"},
	{OPT_UID, "uid=%u"},
	{OPT_GID, "gid=%u"},
	{OPT_MAGIC, "magic"},
	{OPT_ERR, NULL},
};

struct file_system_type ofs_fstype = {
	.owner = THIS_MODULE,
	.name = "ofs",
	.mount = ofs_mount,
	.kill_sb = ofs_kill_sb,
};

/******** ******** super ******** ********/
struct kmem_cache *ofs_inode_cache;
struct kmem_cache *ofs_file_cache;

static const struct super_operations ofs_sops = {
	.alloc_inode = ofs_sops_alloc_inode,
	.destroy_inode = ofs_sops_destroy_inode,
	.drop_inode = ofs_sops_drop_inode,
	.put_super = ofs_sops_put_super,
	.statfs = ofs_sops_statfs,
	.remount_fs = ofs_sops_remount_fs,
	.show_options = ofs_sops_show_options,
};

DEFINE_PER_CPU(ino_t, ofs_ino) = 0;

/******** ******** dentry_operations ******** ********/
const struct dentry_operations ofs_dops = {
	.d_delete = ofs_dops_delete_dentry,
};

/******** ******** backing device infomations ******** ********/
struct backing_dev_info ofs_backing_dev_info = {
	.name = "ofs",
	.ra_pages = 0,
	.capabilities =
                BDI_CAP_NO_ACCT_AND_WRITEBACK | BDI_CAP_MAP_DIRECT |
                BDI_CAP_MAP_COPY | BDI_CAP_READ_MAP | BDI_CAP_WRITE_MAP |
                BDI_CAP_EXEC_MAP,
};

/******** ******** memory mapping ******** ********/
static struct address_space_operations ofs_aops = {
	.readpage = simple_readpage,
	.write_begin = simple_write_begin,
	.write_end = simple_write_end,
	.set_page_dirty = ofs_set_page_dirty_no_writeback,
};

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********      function implementations       ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** mount ******** ********/

/**
 * @brief Transform the mount options string to the
 *        <b><em>struct @ref ofs_mount_opts</em></b>
 * @param mo: mount options structure
 * @param data: mount options string
 * @param remount: indicating whether it is remounting.
 * @retval 0: OK.
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 */
static
int ofs_parse_options(struct ofs_mount_opts *mo, char *data, bool remount)
{
	substring_t args[MAX_OPT_ARGS];
	int token;
	int rc;
	char *p;
	umode_t mode;
	uid_t uid = 0;
	gid_t gid = 0;

	ofs_dbg("*1* data==\"%s\"; mo=={mode==0%o,uid==%u,gid==%u%s};\n",
		data, mo->mode,
		from_kuid_munged(&init_user_ns, mo->kuid),
		from_kgid_munged(&init_user_ns, mo->kgid),
		mo->is_magic? ",magic": "");

	while ((p = strsep(&data, ",")) != NULL) {
		if (!*p)
			continue;

		token = match_token(p, ofs_tokens, args);
		switch (token) {
		case OPT_MODE:
			if (remount)
				continue;
			rc = match_octal(&args[0], (int *)&mode);
			if (rc < 0)
				return -EINVAL;
			mo->mode = mode & 07777;
			break;
		case OPT_UID:
			if (remount)
				continue;
			rc = match_int(&args[0], &uid);
			if (rc < 0)
				return -EINVAL;
			mo->kuid = make_kuid(current_user_ns(), uid);
			if (!uid_valid(mo->kuid))
				return -EINVAL;
			break;
		case OPT_GID:
			if (remount)
				continue;
			rc = match_int(&args[0], &gid);
			if (rc < 0)
				return -EINVAL;
			mo->kgid = make_kgid(current_user_ns(), gid);
			if (!gid_valid(mo->kgid))
				return -EINVAL;
			break;
		case OPT_MAGIC:
			if (remount)
				continue;
			mo->is_magic = true;
			break;
		}
	}

	ofs_dbg("*2* mo=={mode==0%o,uid==%u,gid==%u%s}\n",
		mo->mode,
		from_kuid_munged(&init_user_ns, mo->kuid),
		from_kgid_munged(&init_user_ns, mo->kgid),
		mo->is_magic? ",magic": "");

	return 0;
}

/**
 * @brief Initialize a super block that is newly allocated by <i>sget()</i>
 * @param sb: super block
 * @param magic: Magic mount string, maybe unused when normal mounting.
 * @param mo: mount options
 * @retval 0: OK.
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 */
static
int ofs_fill_superblock(struct super_block *sb, char *magic,
			const struct ofs_mount_opts *mo)
{
	struct ofs_root *newroot = NULL;
	struct inode *rooti = NULL;
	struct dentry *rootd = NULL;
	struct ofs_rbtree *rbtree;
	size_t len = 0;
	int rc = 0;

	ofs_dbg("*1* sb<%p>; magic==\"%s\"; mo=={mode==0%o, uid==%u, "
		"gid==%u, is_magic==%d};\n",
		sb, magic, mo->mode,
		from_kuid_munged(&init_user_ns, mo->kuid),
		from_kgid_munged(&init_user_ns, mo->kgid), mo->is_magic);

	if (mo->is_magic)
		len = strlen(magic) + 1;

	/* Round up to L1_CACHE_BYTES to resist false sharing */
	newroot = kzalloc(max((int)(sizeof(struct ofs_root) + len),
			    L1_CACHE_BYTES), GFP_KERNEL);
	if (IS_ERR_OR_NULL(newroot)) {
		rc = -ENOMEM;
		goto out_return;
	}
	memcpy(&newroot->mo, mo, sizeof(struct ofs_mount_opts));
	newroot->sb = sb;
	newroot->is_registered = false;
	init_rwsem(&newroot->rwsem);
#ifdef CONFIG_OFS_SYSFS
	newroot->omobj = NULL;
#endif
#ifdef CONFIG_OFS_DEBUGFS
	newroot->dbg = NULL;
#endif
	if (newroot->mo.is_magic)
		strcpy(newroot->magic, magic);

	sb->s_fs_info = (void *)newroot;
	sb->s_maxbytes = MAX_LFS_FILESIZE;
	sb->s_blocksize = PAGE_CACHE_SIZE;
	sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
	sb->s_magic = OFS_MAGIC_NUM;
	sb->s_op = &ofs_sops;
	sb->s_d_op = &ofs_dops;
	sb->s_time_gran = 1;
	sb->s_root = NULL;

	rooti = ofs_new_inode(sb, NULL, S_IFDIR | newroot->mo.mode,
			      0, mo->is_magic);
	if (IS_ERR_OR_NULL(rooti)) {
		rc = -ENOMEM;
		goto out_kfree;
	}
	rooti->i_uid = newroot->mo.kuid;
	rooti->i_gid = newroot->mo.kgid;
	newroot->rootoi = OFS_INODE(rooti);

	rootd = d_make_root(rooti); /* set rootd->d_lockref.count to 1. */
	if (IS_ERR_OR_NULL(rootd)) {
		rc = -ENOMEM;
		goto out_kfree;
	}
	sb->s_root = rootd;
	ofs_dbg("*2* sb<%p>; sb->s_fs_info<%p>; rooti<%p>; rootd<%p>;\n",
		sb, sb->s_fs_info, rooti, rootd);

	/* When magic mounting, the root directory 
	 * becomes a magic directory.
	 */
	if (mo->is_magic) {
		spin_lock(&newroot->rootoi->inode.i_lock);
		newroot->rootoi->magic = rootd;
		spin_unlock(&newroot->rootoi->inode.i_lock);
		rbtree = ofs_get_rbtree(newroot->rootoi);
		ofs_rbtree_insert(rbtree, newroot->rootoi);
	}

	return 0;

out_kfree:
	kfree(newroot);
	sb->s_fs_info = NULL;
out_return:
	return rc;
}

/**
 * @brief Test whether super block is existent.
 * @param sb: super block to test.
 * @param magic: test key.
 * @retval 1: existent
 * @retval 0: not existent
 */
int ofs_test_super(struct super_block *sb, void *magic)
{
	if (magic == NULL) /* If magic is NULL, the mount is normal. */
		return 0;
	if (((struct ofs_root *)sb->s_fs_info)->mo.is_magic) {
		return !strcmp(((struct ofs_root *)sb->s_fs_info)->magic,
			       (char *)magic);
	}
	return 0;
}

/**
 * @brief Used to initialize a new super block in <b><em>sget()</em></b>
 * @param sb: super block to test.
 * @param data: data.
 * @retval 0: OK.
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 */
int ofs_set_super(struct super_block *sb, void *data)
{
	sb->s_fs_info = NULL;	/* mark a new super block */
	return set_anon_super(sb, NULL);
}

/**
 * @brief mount filesystem
 * @param fst: filesystem type
 * @param flags: mount flags
 * @param dev_name: When magic user mounting, <b<em>>dev_name</em></b> is 
 *                  the magic string to identify the magic mount.
 *                  It is unused in other mount mode.
 * @param data: When kernel mounting, <b><em>data</em></b> is the magic string.
 *              In other mounting mode, <b><em>data</em></b> is the options
 *              (Passed by "-o option" of <b><i>mount</i></b> cmd).
 * @retval dentry: The root directory dentry of the filesystem if successed.
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * There are 3 mounting mode:
 * * magic user mounting mode: Caused by cmd
 *   "mount -t ofs -o magic magic_string mountpoint"
 * * normal user mounting mode: a normal filesystem.
 * * kernel mounting mode: internal mount in kernel. Caused by
 *   @ref ofs_register().
 * @sa ofs_register()
 */
static
struct dentry *ofs_mount(struct file_system_type *fst, int flags,
			 const char *dev_name, void *data)
{
	struct inode *rooti = NULL;
	char *magic = NULL;
	struct super_block *sb = NULL;
	struct ofs_root *root = NULL;
	struct ofs_mount_opts mo = { OFS_DEFAULT_MODE,
		make_kuid(current_user_ns(), 0),
		make_kgid(current_user_ns(), 0),
		false,
	};
	int ret = 0;

	ofs_dbg("dev_name==%s; data==\"%s\"; %s;\n",
		dev_name, (char *)data,
		flags & MS_KERNMOUNT ? "kernmount" : "usermount");

	if (flags & MS_KERNMOUNT) {
		magic = data;
		mo.is_magic = true;
	} else {		/* user mount */
		ret = ofs_parse_options(&mo, data, false);
		if (ret) {
			ofs_err("wrong mount options!\n");
			return ERR_PTR(ret);
		}
		if (mo.is_magic)
			magic = (char *)dev_name;
		else
			magic = NULL;
	}

	sb = sget(fst, ofs_test_super, ofs_set_super, flags, magic);
	if (IS_ERR(sb))
		return ERR_CAST(sb);

	root = (struct ofs_root *)sb->s_fs_info;
	if (root == NULL) {	/* new sb */
		ret = ofs_fill_superblock(sb, magic, &mo);
		if (ret) {
			deactivate_locked_super(sb);
			return ERR_PTR(ret);
		}
		sb->s_flags |= MS_ACTIVE;
		root = (struct ofs_root *)sb->s_fs_info;
	}

	if (!(flags & MS_KERNMOUNT)) {
		/* BUG: The uid, gid and mode will be overwrote! */
		/* FIXME: need lock */
		save_mount_options(sb, data);
		rooti = sb->s_root->d_inode;
		rooti->i_uid = root->mo.kuid;
		rooti->i_gid = root->mo.kgid;
		rooti->i_mode = (rooti->i_mode & (~07777)) | root->mo.mode;
	}

	return dget(sb->s_root);
}

/**
 * @brief Kill super block
 * @param sb: super block
 * @note
 * * When sb->s_active == 0, this function will be called.
 * * @ref ofs_sops_put_super() will be called in kill_litter_super.
 */
static
void ofs_kill_sb(struct super_block *sb)
{
	ofs_dbg("sb<%p>;\n", sb);
	kill_litter_super(sb);
}

/******** ******** super ******** ********/
/**
 * @brief Get the unique ofs inode number from a percpu variable
 * @return the inode number
 */
static ino_t get_next_ofs_ino(void)
{
	ino_t *p = &get_cpu_var(ofs_ino);
	ino_t res = *p;

#ifdef CONFIG_SMP
	if (unlikely((res & (OFS_INO_BATCH-1)) == 0)) {
		static atomic_t shared_last_ofs_ino;
		ino_t next = atomic_add_return(OFS_INO_BATCH,
					     &shared_last_ofs_ino);
		res = next - OFS_INO_BATCH;
	}
#endif
	*p = ++res;
	put_cpu_var(ofs_ino);
	return res;
}

/**
 * @brief Alloc a new ofs inode
 * @param sb: super block
 * @param iparent: parent inode
 * @param mode: mode
 * @param dev: device number
 * @retval pointer: the new inode
 * @retval NULL: can't alloc a new inode
 */
struct inode *ofs_new_inode(struct super_block *sb, const struct inode *iparent,
			    umode_t mode, dev_t dev, bool is_singularity)
{
	struct inode *newi;
	struct ofs_inode *newoi;

	ofs_dbg("sb<%p>; iparent<%p>; mode==0%o; dev==%d;\n",
		sb, iparent, mode, dev);

	newi = new_inode(sb);
	if (newi == NULL)
		return newi;
	newoi = OFS_INODE(newi);
	newi->i_ino = get_next_ofs_ino();
	newi->i_generation = get_seconds();
	inode_init_owner(newi, iparent, mode);
	newi->i_mapping->a_ops = &ofs_aops;
	newi->i_mapping->backing_dev_info = &ofs_backing_dev_info;
	mapping_set_gfp_mask(newi->i_mapping, GFP_HIGHUSER);
	mapping_set_unevictable(newi->i_mapping);
	newi->i_atime = newi->i_mtime = newi->i_ctime = CURRENT_TIME;
	switch (mode & S_IFMT) {
	case S_IFREG:
		if (is_singularity) {
			newoi->state |= OI_SINGULARITY;
			newi->i_op = &ofs_singularity_iops;
			newi->i_fop = &ofs_singularity_fops;
			newoi->ofsops = &ofs_singularity_ofsops;
			inc_nlink(newi);
		} else {
			newi->i_op = &ofs_regfile_iops;
			newi->i_fop = &ofs_regfile_fops;
		}
		break;
	case S_IFDIR:
		newi->i_op = &ofs_dir_iops;
		newi->i_fop = &ofs_dir_fops;
		inc_nlink(newi); /* Directory inodes start off with
				  * i_nlink == 2 (for "." entry).
				  */
		break;
	case S_IFLNK:
		newi->i_op = &ofs_symlink_iops;
		break;
	default:
		init_special_inode(newi, mode, dev);
		break;
	}
	return newi;
}

/**
 * @brief Used by kmem_cache_alloc(ofs_inode_cache, ...) to initialize the new
 *        ofs_inode
 * @param data: address of the new ofs_inode
 */
void ofs_inode_init_once(void *data)
{
	struct ofs_inode *oi = (struct ofs_inode *)data;

	/* Don't need lock when constructing. */
	rbtree_init_node(&oi->rbnode);
	oi->magic = NULL;
	oi->state = 0;
	oi->ofsops = NULL;
	oi->symlink = OFS_NULL_OID;
	inode_init_once(&oi->inode);
}

/**
 * @brief Used by kmem_cache_alloc(ofs_file_cache, ...) to initialize the new
 *        ofs_file
 * @param data: address of the new ofs_file
 */
void ofs_file_init_once(void *data)
{
	struct ofs_file *of = (struct ofs_file *)data;

	of->cursor = NULL;
	of->priv = NULL;
}

/**
 * @brief super operation to alloc a new ofs inode
 * @param sb: super block
 * @return new inode
 * @retval NULL: failed
 */
static
struct inode *ofs_sops_alloc_inode(struct super_block *sb)
{
	struct ofs_inode *newoi;

	newoi = kmem_cache_alloc(ofs_inode_cache, GFP_KERNEL);
	ofs_dbg("sb<%p>; newoi<%p>;\n", sb, newoi);
	if (newoi)
		return &newoi->inode;
	else
		return NULL;
}

/**
 * @brief rcu-callback to free a ofs inode
 * @param head: rcu head
 * @note
 * * This callback is called by @ref ofs_sops_destroy_inode() after a
 *   rcu grace period.
 */
static
void ofs_destroy_inode_rcu_callback(struct rcu_head *head)
{
	struct inode *inode;

	inode = container_of(head, struct inode, i_rcu);
	ofs_dbg("inode<%p>;\n", inode);
	kmem_cache_free(ofs_inode_cache, OFS_INODE(inode));
}

/**
 * @brief super operation to destroy ofs inode
 * @param inode: inode to destory
 */
static
void ofs_sops_destroy_inode(struct inode *inode)
{
	ofs_dbg("inode<%p>;\n", inode);
	call_rcu(&inode->i_rcu, ofs_destroy_inode_rcu_callback);
}

/**
 * @brief super operation to tell the caller whether to delete the inode.
 * @param inode: The inode need to drop
 * @retval 1: delete it
 * @retval 0: don't delete it
 * @note
 * * iput() will lock the inode->i_lock before call iput_final()
 * * If the return value is 1, <em>iput_final()</em> will delete the inode via
 *   calling the @ref ofs_sops_destroy_inode().
 * * If the return value is 0 and the super_block has flag MS_ACTIVE,
 *   <em>iput_final()</em> will add the inode to lru list.
 * * @ref ofs_mount() set the MS_ACTIVE flag to the super block when mount.
 */
static
int ofs_sops_drop_inode(struct inode *inode)
{
	struct ofs_inode *oi;

	ofs_dbg("inode<%p>;\n", inode);

	oi = OFS_INODE(inode);
	if (oi->magic) {
		struct ofs_rbtree *rbtree;

		spin_unlock(&oi->inode.i_lock);
		rbtree = ofs_get_rbtree(oi);
		ofs_rbtree_remove(rbtree, oi);
		spin_lock(&oi->inode.i_lock);
		oi->magic = NULL;
	}
	return 1; /* always delete inode. (Don't add inode to lru list) */
}

/**
 * @brief super operation to put super_block
 * @param sb: super block
 * @note
 * * This function will be called by kill_litter_super() after all inode iput()
 *   in fsnotify_unmount_inodes(). So, kfree() root here.
 */
static
void ofs_sops_put_super(struct super_block *sb)
{
	struct ofs_root *root;

	ofs_dbg("sb<%p>;\n", sb);
	root = (struct ofs_root *)(sb->s_fs_info);
	if (root) {
		BUG_ON(root->is_registered);
		kfree(root);
		sb->s_fs_info = NULL;
	}
}

/**
 * @brief super operation to remount a filesystem
 * @param sb: super block
 * @param flags: mount flags
 * @param data: mount options
 */
static
int ofs_sops_remount_fs(struct super_block *sb, int *flags, char *data)
{
	struct ofs_root *root = (struct ofs_root *)(sb->s_fs_info);
	int err = 0;

	ofs_dbg("sb<%p>; data<%s>;\n", sb, data);
	err = ofs_parse_options(&(root->mo), data, true);
	return err;
}

/**
 * @brief super operation to get the status of a filesystem
 * @param sb: super block
 * @note
 * * This function will be called by kill_litter_super() after all inode iput()
 *   in fsnotify_unmount_inodes(). So, kfree() root here.
 */
static
int ofs_sops_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	ofs_dbg("dentry<%p>; buf<%p>;\n", dentry, buf);
	return simple_statfs(dentry, buf);
}

/**
 * @brief super operation to show mount options
 * @param seq: seq file to cache the output
 * @param rootd: root dentry of filesystem
 * @retval 0: OK.
 */
static
int ofs_sops_show_options(struct seq_file *seq, struct dentry *rootd)
{
	struct ofs_root *root;

	root = (struct ofs_root *)(rootd->d_sb->s_fs_info);
	seq_printf(seq, ",mode=0%o", root->mo.mode);
	seq_printf(seq, ",uid=%u",
		   from_kuid_munged(&init_user_ns, root->mo.kuid));
	seq_printf(seq, ",gid=%u",
		   from_kgid_munged(&init_user_ns, root->mo.kgid));
	seq_printf(seq, "%s", root->mo.is_magic ? ",magic" : "");

	return 0;
}

/******** ******** dentry_operations ******** ********/

/**
 * @brief dentry operation to tell the caller whether to delete the dentry.
 * @param dentry: dentry to be deleted
 * @retval 1: delete it.
 * @retval 0: don't delete it.
 * @note
 * * VFS need this function return 1 to kill dentry. See <em>dput()</em> in
 *   <b><i>fs/dcache.c</em></b> for detail.
 */
static
int ofs_dops_delete_dentry(const struct dentry *dentry)
{
	ofs_dbg("dentry<%p>\n", dentry);
	return 1;
}

/******** ******** memory mapping ******** ********/
static
int ofs_set_page_dirty_no_writeback(struct page *page)
{
	if (!PageDirty(page)) /* defined in include/linux/page_flags.h */
		return !TestSetPageDirty(page);
	return 0;
}

