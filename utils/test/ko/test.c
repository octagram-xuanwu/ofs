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
 * Example and test code for ofs
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

#include "ofs.h"
#include <linux/version.h>
#include <linux/kconfig.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0)	/* #0 */
#error "ofs need kernel version >= 3.3.0"
#endif /* #0 */

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Octagram Sun");

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       macro       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** print ******** ********/
#define ofs_tst_crt(fmt, x...) \
	printk(KERN_CRIT "[OFS_TEST CRT] <%s> " fmt, __func__, ##x)
#define ofs_tst_inf(fmt, x...) \
	printk(KERN_INFO "[OFS_TEST INF] <%s> " fmt, __func__, ##x)
#define ofs_tst_err(fmt, x...) \
	printk(KERN_ERR "[OFS_TEST ERR] <%s> " fmt, __func__, ##x)
#define ofs_tst_wrn(fmt, x...) \
	printk(KERN_WARNING "[OFS_TEST WRN] <%s> " fmt, __func__, ##x)
#ifdef CONFIG_OFS_EXAMPLE_DEBUG /* #*1*# */
#define ofs_tst_dbg(fmt, x...) \
	printk(KERN_DEBUG "[OFS_TEST DBG] <%s> " fmt, __func__, ##x)
#else /* #*1*# */
#define ofs_tst_dbg(fmt, x...)
#endif /* #*1*# */

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********        function declarations        ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********      .data        ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
struct ofs_root *ofs_root;
oid_t d1mid, d2mid, d3mid, d4mid, f1mid, f2mid, s1mid, s2mid;

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********      function implementations       ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** module init & exit ******** ********/
static int __init ofs_test_init(void)
{
	int rc;

	/* register magic mount "ofstest" */
	ofs_root = ofs_register("ofstest");
	if (IS_ERR_OR_NULL(ofs_root)) {
		ofs_tst_err("can't register magic \"%s\"", "ofs");
		return PTR_ERR(ofs_root);
	}

	/* entry: /dir1 */
	rc = ofs_mkdir_magic(ofs_root, "dir1", 0775, OFS_NULL_OID, &d1mid);
	if (rc)
		ofs_tst_err("can't mkdir \"dir1\"");
	ofs_tst_dbg("dir1={%p, %lu}\n", d1mid.i_sb, d1mid.i_inod);

	/* entry: /dir1/dir2 */
	rc = ofs_mkdir_magic(ofs_root, "dir2", 0775, d1mid, &d2mid);
	if (rc)
		ofs_tst_err("can't mkdir \"dir2\"");
	ofs_tst_dbg("dir2={%p, %lu}\n", d2mid.i_sb, d2mid.i_ino);

	/* entry: /sym1 ---> /dir1/dir2 */
	rc = ofs_symlink_magic(ofs_root, "sym1", OFS_NULL_OID, d2mid, &s1mid);
	if (rc)
		ofs_tst_err("can't symlink \"sym1\"");
	ofs_tst_dbg("symlink1={%p, %lu}\n", s1mid.i_sb, s1mid.i_ino);

	/* entry: /sym1/dir3 ===> /dir1/dir2/dir3 */
	rc = ofs_mkdir_magic(ofs_root, "dir3", 0775, s1mid, &d3mid);
	if (rc)
		ofs_tst_err("can't mkdir \"dir3\"");
	ofs_tst_dbg("dir3={%p, %lu}\n", d3mid.i_sb, d3mid.i_ino);

	/* entry: /dir1/sym2 ---> /dir1/dir2/dir3 */
	rc = ofs_symlink_magic(ofs_root, "sym2", d1mid, d3mid, &s2mid);
	if (rc)
		ofs_tst_err("can't symlink \"sym2\"");
	ofs_tst_dbg("symlink2={%p, %lu}\n", s2mid.i_sb, s2mid.i_ino);

	/* entry: /sym1/singularity ===> /dir1/dir2/singularity */
	rc = ofs_create_magic(ofs_root, "singularity", 0777,
			      true, s1mid, &f1mid);
	if (rc)
		ofs_tst_err("can't create \"regfile1\"");
	ofs_tst_dbg("regfile1={%p, %lu}\n", f1mid.i_sb, f1mid.i_ino);

	/* entry: /dir1/sym2/regfile ===> /dir1/dir2/dir3/regfile */
	rc = ofs_create_magic(ofs_root, "regfile", 0775,
			      false, s2mid, &f2mid);
	if (rc)
		ofs_tst_err("can't create \"regfile2\"");
	ofs_tst_dbg("regfile2={%p, %lu}\n", f2mid.i_sb, f2mid.i_ino);

	/* entry: /sym1/dir4 ===> /dir1/dir2/dir4 */
	rc = ofs_mkdir_magic(ofs_root, "dir4", 0775, s1mid, &d4mid);
	if (rc)
		ofs_tst_err("can't mkdir \"dir4\"");
	ofs_tst_dbg("dir4={%p, %lu}\n", d4mid.i_sb, d4mid.i_ino);

	return 0;
}

static void __exit ofs_test_exit(void)
{
	int rc;
	/* rm -r /dir1 */
	rc = ofs_rm_magic(ofs_root, d1mid, true);
	if (rc) {
		ofs_tst_err("Failed in ofs_unlink() errno:%d", rc);
	}

	/* unregister magic */
	ofs_unregister(ofs_root);
}

module_init(ofs_test_init);
module_exit(ofs_test_exit);
