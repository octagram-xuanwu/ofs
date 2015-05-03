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
 * C header of all apis. 1 tab == 8 spaces.
 */

#ifndef __OFS_H__
#define __OFS_H__

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********    header files   ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#include "ofs_config.h"
#include <linux/version.h>
#include <linux/kconfig.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/vfs.h>
#include <linux/dcache.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/file.h>
#include <linux/limits.h>
#include <linux/spinlock.h>
#include <linux/rwsem.h>
#include "rbtree.h"

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********      macros       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
#error "ofs need kernel version >=3.19.0"
#endif

/******** ******** ofs inode ******** ********/
/**
 * @brief default mode
 */
#define OFS_DEFAULT_MODE    	(S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)

/**
 * @brief null ofs_id
 */
#define OFS_NULL_OID		((oid_t){NULL, 0})

#define OI_CREATING_MAGIC_CHILD		(1U << 0)
#define OI_REMOVING_MAGIC_CHILD		(1U << 1)
#define OI_CREATING_SINGULARITY		(1U << 2)
#define OI_REMOVING_SINGULARITY		(1U << 3)
#define OI_LINKING_MAGIC_CHILD		(1U << 4)
#define OI_RENAMING_MAGIC_CHILD		(1U << 5)

#define OI_SINGULARITY			(1U << 8)
#define OI_MAGIC			(1U << 9)
/**
 * @brief Get the pointer of struct ofs_inode where the inode locates at.
 * @param i: Pointer of inode
 */
#define OFS_INODE(i)		container_of((i), struct ofs_inode, inode)

#define FILE_TO_OFS_INODE(f)	OFS_INODE((f)->f_path.dentry->d_inode)

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       types       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/**
 * @brief Mount options
 */
struct ofs_mount_opts {
	umode_t mode;	/**< The mode and authority of root 
			  *  directory of filesystem
			  */
	kuid_t kuid;	/**< The uid of root directory of
			  *  filesystem in kernel
			  */
	kgid_t kgid;	/**< The gid of root directory of
			  *  filesystem in kernel
			  */
	bool is_magic;	/**< If the ofs is magic, marks true,
			  *  otherwise marks false
			  */
};

struct rbtree;
struct rbtree_node;

/**
 * @brief red-black tree
 */
struct ofs_rbtree {
	struct rbtree tree;	/**< rbtree */
	rwlock_t lock;		/**< read-write spinlock */
};

/**
 * @brief ofs inode descriptor
 */
struct ofs_id {
	struct super_block *i_sb;	/**< owner (place first to align with
					 *   void *priv in struct ofs_inode)
					 */
	ino_t i_ino;			/**< inode number */
};
typedef struct ofs_id oid_t;

/**
 * @brief inode of ofs
 */
struct ofs_inode {
	struct inode inode;		/**< inode */
	struct rbtree_node rbnode;	/**< link in the red-black tree */
	struct dentry *magic;		/**< The unique dentry who owns
					 *   the magic name
					 *   Protected by inode.i_lock.
					 *   It needs not lock when checking
					 *   whether a ofs inode is magic.
					 */
	unsigned int state;		/**< ofs inode state 
					 *   Protected by inode.i_mutex.
					 */
	const struct ofs_operations *ofsops;	/**< ofs operations */
	union {
		void *priv;		/**< private data */
		oid_t symlink;		/**< target of magic symlink */
	};
};

struct ofs_file {
	struct dentry *cursor;
	void *priv;
};

/**
 * @brief ofs namespace data when walking path in kernel.
 */
struct ofs_nameidata {
	unsigned int depth;	/**< depth of nesting symlink */
	oid_t *saved_links[MAX_NESTED_LINKS + 1];	/**< symlink of
							  *  every depth
							  */
};

#ifdef CONFIG_OFS_SYSFS
struct omobject;
#endif
#ifdef CONFIG_OFS_DEBUGFS
struct ofs_dbg;
#endif

/**
 * @brief filesystem root
 */
struct ofs_root {
	struct ofs_inode *rootoi;	/**< ofs inode of root directory */
	struct ofs_mount_opts mo;	/**< mounting options */
	struct super_block *sb;		/**< super block */
	struct rw_semaphore rwsem;	/**< rwsem protects: is_registered
					 *  and all code under the case
					 *  being registered.
					 */
	bool is_registered;		/**< Mark Whether a magic ofs
					  *  is registered. A magic
					  *  ofs can be registered by
					  *  function @ref ofs_register().
					  *  If is_registered == true,
					  *  mo.is_magic == true too.
					  */
	struct path magic_root;		/**< When a magic ofs is
					  *  registered, magic_root is the
					  *  root path of the filesystem
					  *  in kernel.
					  */
#ifdef CONFIG_OFS_SYSFS
	struct omobject *omobj;		/**< magic mount object in sysfs */
#endif
#ifdef CONFIG_OFS_DEBUGFS
	struct ofs_dbg *dbg;		/**< debugfs entry */
#endif
	char magic[];			/**< unique magic string to identify the
					  *  ofs
					  */
};

struct ofs_operations {
	struct module *owner;
	int (*open) (struct ofs_inode *, struct file *);
	int (*release) (struct ofs_inode *, struct file *);
	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
	ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
	loff_t (*llseek) (struct file *, loff_t, int);
	long (*ioctl) (struct file *, unsigned int, unsigned long);
	int (*mmap) (struct file *, struct vm_area_struct *);
};

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       .data       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********         function prototypes         ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** apis ******** ********/
/******** ofs mount apis ********/
extern
struct ofs_root *ofs_register(char *magic);

extern
int ofs_unregister(struct ofs_root *root);

extern
struct ofs_root *ofs_lookup_root(char *magic);

extern
int ofs_put_root(struct ofs_root *root);

/******** magic ofs inode apis ********/
/**
 * @brief get ofs_id
 * @param oitgt: ofs inode
 * @return ofs_id
 */
static __always_inline
oid_t ofs_get_oid(struct ofs_inode *oitgt)
{
	return (oid_t){oitgt->inode.i_sb, oitgt->inode.i_ino};
}

/**
 * @brief compare two ofs_id
 * @param oid1: ofs inode descriptor 1
 * @param oid2: ofs inode descriptor 2
 * @return result
 * @retval -1: oid1 < oid2
 * @retval 0: oid1 = oid2
 * @retval 1: oid1 > oid2
 */
static __always_inline
int ofs_compare_oid(const oid_t oid1, const oid_t oid2)
{
	if (oid1.i_ino > oid2.i_ino) {
		return 1;
	} else if (oid1.i_ino < oid2.i_ino) {
		return -1;
	}
	return 0;
}

struct dentry *ofs_get_magic_alias(struct ofs_inode *oitgt);

/**
 * @brief put the dentry that owns the magic name
 * @param oitgt: magic inode
 * @node
 * * Use this function and @ref ofs_get_magic_alias() in pairs. The function
 *   does some cleaning work.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa  ofs_get_magic_alias()
 */
static __always_inline
void ofs_put_magic_alias(struct ofs_inode *oitgt)
{
	dput(oitgt->magic);
}

extern
struct ofs_inode *ofs_get_folder(struct ofs_root *root, oid_t folder,
				 struct path *result);
/**
 * @brief put the path and ofs inode of a folder
 * @parem oifdr: folder ofs inode to put
 * @param pathfdr: folder path to put
 * @note
 * * Use the function and @ref ofs_get_folder() in pairs.
 *   The function does some cleaning work.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_get_folder()
 * @sa ofs_get_folder_hlp()
 */
static __always_inline
void ofs_put_folder(struct ofs_inode *oifdr, struct path *pathfdr)
{
	path_put(pathfdr);
}

extern
struct ofs_inode *ofs_get_parent(struct ofs_root *root, oid_t target,
				 struct path *result);

/**
 * @brief put the parent path and ofs inode
 * @param oip: parent ofs inode to put
 * @param pathp: parent path to put
 * @note
 * * Use the function and @ref ofs_get_parent() in pairs.
 *   The function does some cleaning work.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_get_parent()
 * @sa ofs_get_parent_hlp()
 */
static __always_inline
void ofs_put_parent(struct ofs_inode *oip, struct path *pathp)
{
	path_put(pathp);
}

extern
struct ofs_inode *ofs_get_oi(struct ofs_root *root, oid_t target,
			     struct path *result);
/**
 * @brief put the path and ofs inode
 * @param oitgt: ofs inode to put
 * @param pathtgt: path to put
 * @note
 * * Use the function and @ref ofs_get_path() in pairs.
 *   The function does some cleaning work.
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_get_oi()
 * @sa ofs_get_oi_hlp()
 */
static __always_inline
void ofs_put_oi(struct ofs_inode *oitgt, struct path *pathtgt)
{
	path_put(pathtgt);
}

extern
int ofs_mkdir_magic(struct ofs_root *root, const char *name, umode_t mode,
		    oid_t folder, oid_t *result);

extern
int ofs_symlink_magic(struct ofs_root *root, const char *name, oid_t folder,
		      oid_t target, oid_t *result);

extern
int ofs_create_magic(struct ofs_root *root, const char *name, umode_t mode,
		     bool is_singularity, oid_t folder, oid_t *result);

extern
int ofs_rmdir_magic(struct ofs_root *root, oid_t target);

extern
int ofs_unlink_magic(struct ofs_root *root, oid_t target);

extern
int ofs_rm_singularity(struct ofs_root *root, oid_t target);

extern
int ofs_rm_magic(struct ofs_root *root, oid_t target, bool r);

extern
int ofs_rename_magic(struct ofs_root *root, oid_t target, oid_t newfdr,
		     const char *newname, unsigned int flags);

extern
int ofs_set_operations(struct ofs_inode *oitgt,
		       const struct ofs_operations *ops);

/******** normal ofs inode apis ********/
extern
int ofs_rmdir_normal(struct ofs_root *root, struct inode *ip,
		     struct dentry *dtgt);

extern
int ofs_unlink_normal(struct ofs_root *root, struct inode *ip,
		      struct dentry *dtgt);

/******** ofs namespace ********/
extern
int ofs_follow_link(struct ofs_inode *oi, struct ofs_nameidata *nd);
extern
void ofs_put_link(struct ofs_inode *oi, struct ofs_nameidata *nd);

/**
 * @brief get target from namespace data
 * @param nd: namespace data
 * @return target ofs_id
 */
static __always_inline
oid_t ofs_nd_get_link(struct ofs_nameidata *nd)
{
	return *(nd->saved_links[nd->depth]);
}

/**
 * @brief set target to namespace data
 * @param nd: namespace data
 * @param oid: target
 */
static __always_inline
void ofs_nd_set_link(struct ofs_nameidata *nd, oid_t *target)
{
	nd->saved_links[nd->depth] = target;
}

#endif /* __OFS_H__ */
