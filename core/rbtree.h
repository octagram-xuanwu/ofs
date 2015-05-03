/**
 * @file
 * @brief C header of the ofs (octagram filesystem, outlandish filesystem, or
 *        odd filesystem) kernel module
 * @author Octagram Sun <octagram@qq.com>
 * @version 0.1.0
 * @date 2015
 * @copyright Octagram Sun <octagram@qq.com>
 * @note
 * * C header of red-black tree. 1 tab == 8 spaces.
 * @note
 * * In addition to the requirements imposed on a binary search tree the
 *   following must be satisfied by a red–black tree:
 *   1) A node is either red or black.
 *   2) The root is black.
 *   3) All leaves (NIL) are black. (All leaves are same color as the root.)
 *   4) Every red node must have two black child nodes or no child.
 *   5) Every path from a given node to any of its descendant NIL nodes
 *      contains the same number of black nodes.
 */

#ifndef __OFS_RBTREE_H__
#define __OFS_RBTREE_H__

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********      include      ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#include <linux/kernel.h>

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********       types       ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
typedef unsigned long ptr_t;

enum {
	RBTREE_LEFT = 0, /* position: left */
	RBTREE_RIGHT = 1, /* position: right */
};

enum {
	RBTREE_RED = 0, /* color: red */
	RBTREE_BLACK = 2, /* color: black */
};

/**
 * @brief red-black tree node
 * @note
 * * Don't use bit-defination because this program may be used in big-endian
 *   machine.
 */
struct rbtree_node {
	struct rbtree_node *left __aligned(4); /**< left child */
	struct rbtree_node *right __aligned(4); /**< right child */
	union {
		struct rbtree_node **link; /**< point to parent's left
					    *   or right.
					    */
		ptr_t pos/*:1*/; /**< bit_0: if == RBTREE_RIGHT, this node is
				  *   right child. Otherwise, this node is
				  *   left child.
				  */
		ptr_t color/*:1*/; /**< bit_1: red or black */
		ptr_t lpc; /**< link, postion and color */
	};
} __aligned(4);

/**
 * @brief red-black tree
 */
struct rbtree {
	struct rbtree_node *root __aligned(4);
} __aligned(4);

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** inline functions & macro functions  ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
/**
 * @brief Initialize red-black tree
 */
static inline
void rbtree_init(struct rbtree *t)
{
	t->root = NULL;
}

/**
 * @brief Initialize red-black tree node
 */
static inline
void rbtree_init_node(struct rbtree_node *node)
{
	node->left = NULL;
	node->right = NULL;
	node->lpc = 0;
}

/**
 * @brief Get real pointer
 * @param link: node->link
 */
#define rbtree_link(link) \
	((struct rbtree_node **)(((ptr_t)(link)) & (~((ptr_t)3))))

/**
 * @brief Get link pointer and position. (Mask the color infomation.)
 * @param lpc: node->lpc
 */
#define rbtree_lnpos(lpc) \
	((ptr_t)lpc & (~((ptr_t)2)))

/**
 * @brief Get position: left or right.
 * @param pos: node->pos
 */
#define rbtree_position(pos) \
	((ptr_t)(((ptr_t)(pos)) & (ptr_t)1))

/**
 * @brief Check whether the position is right.
 * @param pos: node->pos
 */
#define rbtree_linking_r(pos)	rbtree_position(pos)

/**
 * @brief Check whether the position is left.
 * @param pos: node->pos
 */
#define rbtree_linking_l(pos)	(!rbtree_linking_r(pos))

/**
 * @brief Get a lpc with positon &(node->left) and color.
 * @param node: Get positon &(node->left)
 * @param color: color
 */
#define rbtree_lc(node, color)	\
	((ptr_t)(&((node)->left)) | (ptr_t)(color))

/**
 * @brief Get a lpc with positon &(node->right) and color.
 * @param node: Get positon &(node->right)
 * @param color: color
 */
#define rbtree_rc(node, color)	\
	((ptr_t)(&((node)->right)) | (ptr_t)(color) | RBTREE_RIGHT)

/**
 * @brief Get a lpc with positon &(node->left) and color red.
 * @param node: Get positon &(node->left)
 */
#define rbtree_lr(node)		rbtree_lc(node, RBTREE_RED)

/**
 * @brief Get a lpc with positon &(node->left) and color black.
 * @param node: Get positon &(node->left)
 */
#define rbtree_lb(node)		rbtree_lc(node, RBTREE_BLACK)

/**
 * @brief Get a lpc with positon &(node->right) and color red.
 * @param node: Get positon &(node->left)
 */
#define rbtree_rr(node)		rbtree_rc(node, RBTREE_RED)

/**
 * @brief Get a lpc with positon &(node->right) and color black.
 * @param node: Get positon &(node->left)
 */
#define rbtree_rb(node)		rbtree_rc(node, RBTREE_BLACK)

/**
 * @brief Cast a rbtree_node of a structure out to the containing structure
 * @param ptr: the pointer to rbtree_node
 * @param type: the type of the container struct this is embedded in
 * @member: the name of the member within the struct
 */
#define rbtree_entry(ptr, type, member)	container_of(ptr, type, member)


/**
 * @brief Get parent
 * @param link: node
 * @return parent
 */
static inline struct rbtree_node *rbtree_parent(struct rbtree_node *node)
{
	register ptr_t pos = rbtree_position(node->pos);
	register struct rbtree_node **link = rbtree_link(node->link) - pos;
	return container_of(link, struct rbtree_node, left);
}

/**
 * @brief Get color: red or black.
 * @param pos: node->color
 */
#define rbtree_color(color)		(((ptr_t)(color)) & (ptr_t)2)

/**
 * @brief Check whether the color is black.
 * @param pos: node->color
 */
#define rbtree_color_is_black(color)	rbtree_color(color)

/**
 * @brief Check whether the color is red.
 * @param pos: node->color
 */
#define rbtree_color_is_red(color)	(!rbtree_color(color))

/**
 * @brief Flip color
 * @param node: Node to be flipped color.
 */
static inline
void rbtree_flip_color(struct rbtree_node *node)
{
	node->color ^= (ptr_t)2;
}

/**
 *@brief Set color: black
 *@param node: node to be set color
 */
static inline
void rbtree_set_black(struct rbtree_node *node)
{
	node->color |= (ptr_t)2;
}

/**
 *@brief Set color: red
 *@param node: node to set color
 */
static inline
void rbtree_set_red(struct rbtree_node *node)
{
	node->color &= ~(ptr_t)(2);
}

/**
 *@brief link to parent and color
 *@param node: node to link
 *@param lpc: link, postition and color
 */
static inline
void rbtree_lpc(struct rbtree_node *node, ptr_t lpc)
{
	node->lpc = lpc;
	*rbtree_link(lpc) = node;
}

/**
 *@brief link nil to parent
 *@param link: link postition
 */
static inline
void rbtree_ln_nil(struct rbtree_node **link)
{
	*link = NULL;
}

/**
 * @brief Get the predecessor
 * @param node: node
 * @return predecessor
 */
static inline
struct rbtree_node *rbtree_predecessor(struct rbtree_node *node)
{
	register struct rbtree_node *c = node->left;
	if (!c)
		return node;
	while (c->right)
		c = c->right;
	return c;
}

/**
 * @brief Get the successor
 * @param node: node
 * @return successor
 */
static inline
struct rbtree_node *rbtree_successor(struct rbtree_node *node)
{
	register struct rbtree_node *c = node->right;
	if (!c)
		return node;
	while (c->left)
		c = c->left;
	return c;
}

/**
 *@brief Use a new node to replace the old node and inherit the old color
 *@param newd: new node
 *@param oldn: old node
 */
static inline
void rbtree_transplant(struct rbtree_node *newn, struct rbtree_node *oldn)
{
	newn->lpc = oldn->lpc;
	*rbtree_link(oldn->lpc) = newn;
}

/**
 *@brief Use a leaf to replace the old node
 *@param oldn: old node
 */
static inline
void rbtree_transplant_nil(struct rbtree_node *oldn)
{
	*rbtree_link(oldn->lpc) = NULL;
}

void rbtree_insert_color(struct rbtree *tree, struct rbtree_node *node);

void rbtree_rm(struct rbtree *tree, struct rbtree_node *node);

#endif /* rbtree.h */
