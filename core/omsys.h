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
 * This header is sysfs interface to the ofs. 1 tab == 8 spaces.
 */

#ifndef __OFS_OMSYS_H__
#define __OFS_OMSYS_H__

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********      macros       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#define OFS_ATTR(_name, _show, _store)			\
	static struct kobj_attribute ofs_attr_##_name =	\
		__ATTR(_name, 0644, _show, _store)

#define OMSYS_ATTR(_name, _mode, _show, _store)		\
	struct omsys_attribute omsys_attr_##_name =	\
		__ATTR(_name, _mode, _show, _store)

#define OMSYS_ATTR_RW(_name)	\
	struct omsys_attribute omsys_attr_##_name = __ATTR_RW(_name)

#define OMSYS_ATTR_RO(_name)	\
	struct omsys_attribute omsys_attr_##_name = __ATTR_RO(_name)

#define TO_MATTR(_attr) container_of(_attr, struct omsys_attribute, attr)

#define TO_OMOBJ(obj) container_of(obj, struct omobject, mm.kobj)
/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       types       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
struct ofs_root;

struct omsys_attribute {
	struct attribute attr; /**< attribute file */
	ssize_t (*show)(struct ofs_root *root,
			char *buf);	/**< syscall read */
	ssize_t (*store)(struct ofs_root *root, const char *buf,
			 size_t count);	/**< syscal write */
};

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********         function prototypes         ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
extern
int ofs_construct_sysfs(void);

extern
void ofs_destruct_sysfs(void);

extern
int omsys_register(struct ofs_root *root,
		   const struct kset_uevent_ops *uevent_ops);

extern
void omsys_unregister(struct ofs_root *root);

extern
int omsys_create_file(struct ofs_root *root, struct omsys_attribute *attr);

extern
void omsys_remove_file(struct ofs_root *root, struct omsys_attribute *attr);

#endif /* omsys.h */
