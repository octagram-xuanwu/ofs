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
 * Source about symlink
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
#include "symlink.h"

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       macro       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       types       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********         function prototypes         ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
static
char *ofs_get_page_link(struct inode *inode, struct page **ppage);

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********   .sdata & .data  ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** symlink operations ******** ********/
/* inode_operations */
const struct inode_operations ofs_symlink_iops = {
	.readlink = ofs_symlink_iops_readlink,
	.follow_link = ofs_symlink_iops_follow_link,
	.put_link = ofs_symlink_iops_put_link,
};

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ********      function implementations       ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/******** ******** symlink operations ******** ********/
/**
 * @brief get symlink
 * @param inode: inode of the symlink
 * @param ppage: used to return the page pointer
 * @return target string of the symlink and the page
 * @retval string: target of the symlink if successed
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 */
static char *ofs_get_page_link(struct inode *inode, struct page **ppage)
{
	char *kaddr;
	struct page *page;
	struct address_space *mapping = inode->i_mapping;
	page = read_mapping_page(mapping, 0, NULL);
	if (IS_ERR(page))
		return (char *)page;
	*ppage = page;
	kaddr = kmap(page);
	nd_terminate_link(kaddr, inode->i_size, PAGE_SIZE - 1);
	return kaddr;
}

/******** inode_operations ********/
/**
 * @brief ofs inode operation for symlink: readlink
 * @param dentry: dentry of target symlink
 * @param buffer: user buffer to receive the symlink string
 * @param buflen: buffer size
 * @return error code
 * @retval 0: if successed.
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * TODO
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 */
int ofs_symlink_iops_readlink(struct dentry *dentry, char __user * buf,
			      int buflen)
{
	return generic_readlink(dentry, buf, buflen);
}

/**
 * @brief ofs inode operation for symlink: follow_link
 * @param dentry: dentry of target symlink
 * @param nd: nameidata to process the path in kernel
 * @return the pointer of data. It is usually used as the 3rd parameter of
 *         @ref ofs_symlink_iops_put_link() to do some cleaning work.
 * @retval pointer: (void *) if successed.
 * @retval errno: Indicate the error code
 *                (see <b><a href="/usr/include/asm-generic/errno-base.h">
 *                 /usr/include/asm-generic/errno-base.h</a></b>).
 * @note
 * * TODO
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_symlink_iops_put_link()
 */
void *ofs_symlink_iops_follow_link(struct dentry *dentry, struct nameidata *nd)
{
	struct page *page = NULL;
	char *symlink;
	symlink = ofs_get_page_link(dentry->d_inode, &page);
	if (IS_ERR_OR_NULL(symlink))
		return symlink;
	nd_set_link(nd, symlink);
	return page;
}

/**
 * @brief ofs inode operation for symlink: put_link
 * @param dentry: dentry of target symlink
 * @param nd: nameidata to process the path in kernel
 * @param cookie: pointer to data. It is usually assigned to the return of
 *                @ref ofs_symlink_iops_follow_link() to do
 *                some cleaning work.
 * @note
 * * TODO
 * @remark
 * * The function can't be used in ISR (Interrupt Service Routine).
 * @sa ofs_symlink_iops_follow_link
 */
void ofs_symlink_iops_put_link(struct dentry *dentry, struct nameidata *nd,
			       void *cookie)
{
	struct page *page = cookie;

	if (page) {
		kunmap(page);
		page_cache_release(page);
	}
}
