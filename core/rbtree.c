/**
 * @file
 * @brief C source of the ofs (octagram filesystem, outlandish filesystem, or
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

/******** ******** ******** ******** ******** ******** ******** ********
 ******** ******** ********      include      ******** ******** ********
 ******** ******** ******** ******** ******** ******** ******** ********/
#include "rbtree.h"

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
/**
 * @brief fix color after insert a new red node
 * @param tree: red-black tree
 * @param node: node to link
 * @note
 * * Node to insert is red at first.
 */
void rbtree_insert_color(struct rbtree *tree, struct rbtree_node *node)
{
	struct rbtree_node *parent, *gparent;
	union {
		struct rbtree_node *child;
		struct rbtree_node *sibling;
		struct rbtree_node *uncle;
	} tmp;
	ptr_t rotation, lpc;

recursively_fix:
	if (node == tree->root) {
		/*
		 * case 1: rbtree is empty before rbtree_link_red(node)
		 *
		 *               n(R)
		 *               /  \
		 *        NULL(B)    NULL(B)
		 *
		 *  flip_color (n)
		 * --------------->
		 *
		 *               N(B)
		 *               /  \
		 *        NULL(B)    NULL(B)
		 *
		 */
		rbtree_flip_color(node);
		return;
	}

	parent = rbtree_parent(node);
	if (rbtree_color_is_black(parent->color)) {
		/*
		 * case 2: Parent is black. Do nothing.
		 */
		return;
	}

	/* case 3: Parent is red */
	gparent = rbtree_parent(parent);
	if (rbtree_linking_r(parent->pos)) {
		tmp.uncle = gparent->left;
		/* `rotation' is used in case 3.2.1 to right-rotate parent */
		rotation = rbtree_rr(node);
	} else {
		tmp.uncle = gparent->right;
		/* `rotation' is used in case 3.2.1 to left-rotate parent */
		rotation = rbtree_lr(node);
	}

	if (tmp.uncle && rbtree_color_is_red(tmp.uncle->color)) {
		/*
		 * case 3.1: Uncle is red.
		 *                   |
		 *                  G(B)
		 *                __/  \__
		 *               /        \
		 *           p(R)          u(R)
		 *           /  \          /  \
		 *       n(R)
		 *       /  \
		 *
		 *  flip_color(p); flip_color(u); flip_color(G);
		 * ---------------------------------------------->
		 *
		 *                   |
		 *                  g(R)
		 *                __/  \__
		 *               /        \
		 *           P(B)          U(B)
		 *           /  \          /  \
		 *       n(R)
		 *       /  \
		 *
		 *  recursively fix_color(G);
		 * --------------------------->
		 * Shove the work onto grandparent.
		 *
		 * ******** ******** ******** ******** ******** ********
		 *  OR
		 * ******** ******** ******** ******** ******** ********
		 *
		 *                   |
		 *                  G(B)
		 *                __/  \__
		 *               /        \
		 *           u(R)          p(R)
		 *           /  \          /  \
		 *                             n(R)
		 *                             /  \
		 *
		 *  flip_color(p); flip_color(u); flip_color(G);
		 * ---------------------------------------------->
		 *
		 *                   |
		 *                  g(R)
		 *                __/  \__
		 *               /        \
		 *           U(B)          P(B)
		 *           /  \          /  \
		 *                             n(R)
		 *                             /  \
		 *
		 *  recursively fix_color(G);
		 * --------------------------->
		 * Shove the work onto grandparent.
		 *
		 */
		rbtree_flip_color(parent);
		rbtree_flip_color(tmp.uncle);
		rbtree_flip_color(gparent);
		node = gparent;
		goto recursively_fix;
	}

	/* Case 3.2: Uncle is leaf or black-child */
	if (rbtree_position(parent->pos) != rbtree_position(node->pos)) {
		/*
		 * Case 3.2.1: node->pos != parent->pos.
		 *
		 *                      |
		 *                     G(B)
		 *                   __/  \__
		 *                  /        \
		 *               p(R)         U(B)
		 *             __/  \__       /	 \
		 *            /	       \
		 *        S(B)          n(R)
		 *        /  \          /  \
		 *                  L(B)    R(B)
		 *                  /  \    /  \
		 *
		 *  left_rotate(p);
		 * ----------------->
		 *
		 *                           |
		 *                          G(B)
		 *                        __/  \__
		 *                       /        \
		 *                   n(R)          U(B)
		 *                 __/  \__        /  \
		 *                /	   \
		 *            p(R)          R(B)
		 *            /  \          /  \
		 *        S(B)    L(B)
		 *        /  \    /  \
		 *
		 * ******** ******** ******** ******** ******** ********
		 *  OR
		 * ******** ******** ******** ******** ******** ********
		 *
		 *               |
		 *              G(B)
		 *            __/  \__
		 *           /        \
		 *        U(B)         p(R)
		 *        /  \       __/  \__
		 *                  /        \
		 *              n(R)          S(B)
		 *              /  \          /  \
		 *          L(B)    R(B)
		 *          /  \    /  \
		 *
		 *  right_rotate(p);
		 * ----------------->
		 *
		 *               |
		 *              G(B)
		 *            __/  \__
		 *           /        \
		 *       U(B)          n(R)
		 *       /  \        __/  \__
		 *                  /        \
		 *              L(B)          p(R)
		 *                            /  \
		 *                        R(B)    S(B)
		 *                        /  \    /  \
		 *
		 * Transform into case 3.3: parent is new node .
		 */
		tmp.child = *rbtree_link(rotation);
		lpc = parent->lpc;

		*rbtree_link(node->link) = tmp.child;
		if (tmp.child)
			tmp.child->lpc = node->lpc | RBTREE_BLACK;

		*rbtree_link(rotation) = parent;
		parent->lpc = rotation; /* color: red */

		/* *rbtree_link(lpc) = node; */ /* Can be omitted. */
		node->lpc = lpc; /* color: red */

		/* Transform into case 3.2.2 */
		tmp.child = parent;
		parent = node;
		node = tmp.child;
		/* lpc = parent->lpc; */ /* Can be omitted. */
	}

	/*
	 * case 3.2.2: node->pos == parent->pos.
	 *
	 *                           |
	 *                          G(B)
	 *                        __/  \__
	 *                       /        \
	 *                   p(R)          U(B)
	 *                 __/  \__        /  \
	 *                /	   \
	 *            n(R)          S(B)
	 *            /  \          /  \
	 *        L(B)    R(B)
	 *        /  \    /  \
	 *
	 *  right_rotate(G);
	 * ------------------>
	 *
	 *                     |
	 *                    p(R)
	 *                 ___/  \___
	 *                /	     \
	 *            n(R)            G(B)
	 *            /  \            /  \
	 *        L(B)    R(B)    S(B)    U(B)
	 *        /  \    /  \    /  \    /  \
	 *
	 *  flip_color(p); flip_color(G);
	 * ------------------------------->
	 *
	 *                     |
	 *                    P(B)
	 *                 ___/  \___
	 *                /	     \
	 *            n(R)            g(R)
	 *            /  \            /  \
	 *        L(B)    R(B)    S(B)    U(B)
	 *        /  \    /  \    /  \    /  \
	 *
	 * ******** ******** ******** ******** ******** ********
	 *  OR
	 * ******** ******** ******** ******** ******** ********
	 *
	 *               |
	 *              G(B)
	 *            __/  \__
	 *           /        \
	 *       U(B)          p(R)
	 *       /  \        __/  \__
	 *                  /        \
	 *              S(B)          n(R)
	 *                            /  \
	 *                        L(B)    R(B)
	 *                        /  \    /  \
	 *
	 *  left_rotate(G);
	 * ------------------>
	 *
	 *                     |
	 *                    p(R)
	 *                 ___/  \___
	 *                /	     \
	 *            G(B)            n(R)
	 *            /  \            /  \
	 *        U(B)    S(B)    L(B)    R(B)
	 *        /  \    /  \    /  \    /  \
	 *
	 *  flip_color(p); flip_color(G);
	 * ------------------------------->
	 *
	 *                     |
	 *                    P(B)
	 *                 ___/  \___
	 *                /	     \
	 *            G(R)            n(R)
	 *            /  \            /  \
	 *        U(B)    S(B)    L(B)    R(B)
	 *        /  \    /  \    /  \    /  \
	 *
	 */
	if (rbtree_linking_r(node->pos)) {
		tmp.sibling = parent->left;
		rotation = rbtree_lr(parent); /* left_rotate gparent */
	} else {
		tmp.sibling = parent->right;
		rotation = rbtree_rr(parent); /* right_rotate gparent */
	}
	lpc = parent->lpc;

	*rbtree_link(gparent->link) = parent;
	parent->lpc = gparent->lpc; /* flip_color(p): black */

	*rbtree_link(lpc) = tmp.sibling;
	if (tmp.sibling)
		tmp.sibling->lpc = ((ptr_t)lpc) | RBTREE_BLACK;

	*rbtree_link(rotation) = gparent;
	gparent->lpc = rotation; /* flip_color(G): red */
}

/**
 * @brief Remove node and fix color
 * @param tree: red-black tree
 * @param node: node to be removed
 * @note
 * * Node to be removed must be linked in rbtree.
 */
void rbtree_rm(struct rbtree *tree, struct rbtree_node *node)
{
	struct rbtree_node *parent, *sibling;
	union {
		struct rbtree_node *same;
		struct rbtree_node *left;
	} sl;
	union {
		struct rbtree_node *reverse;
		struct rbtree_node *right;
	} rr;
	union {
		struct rbtree_node *child;
		ptr_t color;
	} cc;
	ptr_t lpc;
	ptr_t rotation;

	/* Stage 1: remove node */
	rr.right =  node->right;
	sl.left =  node->left;
	if (!sl.left) {
		parent = rbtree_parent(node);
		*rbtree_link(node->link) = rr.right;
		if (!rr.right) {
			/*
			 * Case 1: Node has no child.
			 *
			 *        |
			 *      p(Cp)
			 *      /   \
			 * s(Cs)     n(Cn)
			 *
			 *  transplant_null(n)
			 * -------------------->
			 *
			 *       |                            |
			 *     p(Cp)                        p(Cp)
			 *     /   \         or             /   \          or
			 * s(R)     NULL(B)             S(B)     NULL(BB)
			 *                              /  \
			 *                       NULL(B)    NULL(B)
			 *
			 *           |
			 *         p(Cp)
			 *         /   \
			 *      s(R)    NULL(BB)
			 *      /  \
			 *  L(B)    R(B)
			 *
			 * ******** ******** ******** ******** ******** ********
			 *  OR
			 * ******** ******** ******** ******** ******** ********
			 *
			 *        |
			 *      p(Cp)
			 *      /   \
			 * n(Cn)     s(Cn)
			 *
			 *  transplant_null(n)
			 * -------------------->
			 *
			 *          |                      |
			 *        p(Cp)                  p(Cp)
			 *        /   \      or          /   \              or
			 * NULL(B)     s(R)      NULL(BB)     S(B)
			 *                                    /  \
			 *                             NULL(B)    NULL(B)
			 *
			 *           |
			 *         p(Cp)
			 *         /   \
			 * NULL(BB)     s(R)
			 *              /  \
			 *          L(B)    R(B)
			 *
			 * * If node is red, the sibling is NULL or red due 5).
       			 * * If node is black, the sibling is not NULL
			 *   due to 5). And if sibling is red, it has two
			 *   black children. If sibling is black, it has
			 *   no child.
			 */
			if (NULL == tree->root)
				return;

			lpc = node->lpc;
			if (rbtree_color_is_black(lpc)) {
				/*
				 * `rotation' is Used to right-rotate parent
				 * in stage 2.
				 */
				if (rbtree_linking_r(lpc)) {
					sibling = parent->left;
					rotation = rbtree_rr(sibling);
				} else {
					sibling = parent->right;
					rotation = rbtree_lr(sibling);
				}
			} else
				sibling = NULL;
		} else {
			/*
			 * Case 2.1: Node has no left child.
			 *
			 *    |
			 *   n(B)
			 *      \
			 *       r(R)
			 *
			 *  transplant(n->right, n)
			 * ------------------------->
			 *
			 *    |
			 *  r(RB)
			 *
			 *  set_black(r)
			 * ---------------->
			 *
			 *    |
			 *   r(B)
			 *
			 * If there is one child, it must be red due to 5),
			 * and the node is black due to 4).
			 */
			rr.right->lpc = node->lpc; /* Inherit color */
			sibling = NULL;
		}
	} else if (!rr.right) {
		/*
		 * Case 2.2: Node has no right child.
		 *
		 *       |
		 *      n(B)
		 *      /
		 *  r(R)
		 *
		 *  transplant(n->left, n)
		 * ------------------------->
		 *
		 *    |
		 *  r(RB)
		 *
		 *  set_black(r)
		 * ---------------->
		 *
		 *    |
		 *   r(B)
		 *
		 * If there is one child it must be red due to 5),
		 * and the node is black due to 4).
		 */
		rbtree_lpc(sl.left, node->lpc); /* Inherit color */
		sibling = NULL;
	} else {
		struct rbtree_node *successor = rbtree_successor(node);

		if (successor == rr.right) {
			/*
			 * Case 3: Node(n)'s successor is its right child(r).
			 *
			 *        |
			 *       n(Cn)
			 *      /   \
			 * l(Cl)     s(Cs)
			 *               \
			 *                c(R)_or_NULL(B)
			 *
			 *   transplant(s, n)
			 *  -------------------------------------------->
			 *   (s inherits color of n and leaves Cr to c)
			 *
			 *      |
			 *     s(Cn)                         n()
			 *         \                         /
			 *          c(RB)_or_NULL(BB)   l(Cl)
			 *
			 *   lpc(l, ((&r->left) | (BSTREE_LEFT) | color(l)))
			 *  ------------------------------------------------->
			 *
			 *         |
			 *       s(Cn)
			 *       /   \
			 *  l(Cl)     c(B)_or_NULL(BB)
			 *
			 * * Right child of successor must be red if existing
			 *   due to 5), and the successor is black due to 4).
			 * * If successor is red, it has no child due to 5).
			 * * If successor has no right child and successor is
			 *   black, left must be not NULL due to 5).
			 *
			 */

			if (successor->right) {
				rbtree_set_black(successor->right);
				sibling = NULL;
			} else {
				/*
				 * `rotation' is Used to right-rotate parent
				 * in stage 2.
				 */
				if (rbtree_color_is_black(successor->color)) {
					sibling = sl.left;
					parent = successor;
					rotation = rbtree_rr(sibling);
				} else
					sibling = NULL;
			}
			rbtree_transplant(successor, node);
			cc.color = rbtree_color(sl.left->color);
			rbtree_lpc(sl.left, rbtree_lc(successor, cc.color));
		} else {
			/*
			 * Case 4: Node(n)'s successor(s) is leftmost under
			 *         node(n)'s right child subtree.
			 *
			 *        |
			 *      n(Cn)
			 *      /   \
			 *     l     r
			 *          / \
			 *         p
			 *        / \
			 *   s(Cs)
			 *       \
			 *        c(R)_or_NIL(B)
			 *
			 *   transplant(s->right, s)
			 *  ------------------------->
			 *
			 *                  |
			 *                n(Cn)            s()
			 *                /   \
			 *               l     r
			 *                    / \
			 *                   p
			 *                  / \
			 *  c(RB)_or_NIL(BB)
			 *
			 *   lpc(r, ((&s->right) | (BSTREE_RIGHT) | color(r)))
			 *  --------------------------------------------------->
			 *
			 *      |
			 *    n(Cn)              s()
			 *    /                    \
			 *   l                      r
			 *                         / \
			 *                        p
			 *                       / \
			 *        C(B)_or_NIL(BB)
			 *
			 *   lpc(l, ((&s->left) | (BSTREE_LEFT) | color(l)))
			 *  ------------------------------------------------->
			 *
			 *       |
			 *     n(Cn)            s()
			 *                      / \
			 *                     l   r
			 *                        / \
			 *                       p
			 *                      / \
			 *       C(B)_or_NIL(BB)
			 *
			 *   transplant(s, n)
			 *  ------------------>
			 *
			 *                    |
			 *                  s(Cn)
			 *                  /   \
			 *                 l     r
			 *                      / \
			 *                     p
			 *                    / \
			 *     C(B)_or_NIL(BB)
			 *
			 * * Right child of successor must be red if existing
			 *   due to 5), and the successor is black due to 4).
			 * * If successor has no right child and successor is
			 *   black, left must be not NULL due to 5).
			 */
			parent = rbtree_parent(successor);
			lpc = successor->lpc; /* cache */
			cc.child = successor->right;

			*rbtree_link(lpc) = cc.child;
			if (cc.child) {
				cc.child->lpc = lpc; /* Inherit color */
				sibling = NULL;
			} else {
				/*
				 * `rotation' is Used to left-rotate parent
				 * in stage 2.
				 */
				if (rbtree_color_is_black(successor->color)) {
					sibling = parent->right;
					rotation = rbtree_lr(sibling);
				} else
					sibling = NULL;
			}

			cc.color = rbtree_color(rr.right->color);
			rbtree_lpc(rr.right, rbtree_rc(successor, cc.color));

			cc.color = rbtree_color(sl.left->color);
			rbtree_lpc(sl.left, rbtree_lc(successor, cc.color));

			rbtree_transplant(successor, node);
		}
	}

	/* Stage 2: fix color */
	if (!sibling)
		return;

recursively_fix:
	if (rbtree_color_is_red(sibling->color)) {
		/*
		 * Case 1: Sibling is red.
		 *                          *
		 *         |                *           |
		 *        P(B)              *          P(B)
		 *        /  \              *          /  \
		 *   X(BB)    s(R)          *      s(R)    X(BB)
		 *            /  \          *      /  \
		 *        L(B)    R(B)      *  L(B)    R(B)
		 *                          *
		 *  left_rotate(P)          *  right_rotate(P)
		 * ---------------->        * ----------------->
		 *                          *
		 *            |             *           |
		 *           s(R)           *          s(R)
		 *           /  \           *          /  \
		 *       P(B)    R(B)       *      L(B)    P(B)
		 *       /  \               *              /  \
		 *  X(BB)    L(B)           *          R(B)    X(BB)
		 *                          *
		 *  flip_color(s);          *  flip_color(s);
		 * ---------------->        * ---------------->
		 *  flip_color(P);          *  flip_color(P);
		 *                          *
		 *            |             *           |
		 *           S(B)           *          S(B)
		 *           /  \           *          /  \
		 *       p(R)    R(B)       *      L(B)    p(R)
		 *       /  \               *              /  \
		 *  X(BB)    L(B)           *          R(B)    X(BB)
		 *                          *
		 * L is new sibling.        * R is new sibling.
		 *                          *
		 * * Parent must be black due to 4).
		 * * Sibling has two black children due to 5).
		 */
		lpc = sibling->lpc;
		cc.child = *rbtree_link(rotation);

		*rbtree_link(parent->link) = sibling;
		sibling->lpc = parent->lpc; /* flip_color: black */

		*rbtree_link(lpc) = cc.child;
		if (cc.child)
			cc.child->lpc = lpc | RBTREE_BLACK;

		/* rbtree_lpc(parent, rotation); */
		*rbtree_link(rotation) = parent;
		parent->lpc = rotation; /* flip_color: red */

		/* parent = parent; */
		sibling = cc.child;
	}

black_sibling:
	/*
	 * Case 2: Sibling is black
	 */
	if (rbtree_linking_r(sibling->link)) {
		/* `rotation' is used to left rotate parent in case 2.3 . */
		rotation = rbtree_lr(sibling);
		rr.reverse = sibling->left;
		sl.same = sibling->right;
		/* `lpc' is used to right rotate reverse in case 2.2 . */
		if (rr.reverse)
			lpc = rbtree_rr(rr.reverse);
	} else {
		/* `rotation' is used to right rotate parent in case 2.3 . */
		rotation = rbtree_rr(sibling);
		rr.reverse = sibling->right;
		sl.same = sibling->left;
		/* `lpc' is used to left rotate reverse in case 2.2 . */
		if (rr.reverse)
			lpc = rbtree_lr(rr.reverse);
	}

	if (!sl.same || rbtree_color_is_black(sl.same->color)) {
		if (!rr.reverse || rbtree_color_is_black(rr.reverse->color)) {
			/*
			 * Case 2.1: Sibling has no red child.
			 *
			 *             |
			 *           p(Cp)
			 *           /   \
			 *       S(B)     X(BB)
			 *       /  \
			 *  Cs(B)    Cr(B)
			 *
			 *  flip_color(p); flip_color(S); flip_color(X);
			 * ---------------------------------------------->
			 *
			 *             |
			 *           p(RB_or_BB)
			 *           /    \
			 *       s(R)      X(B)
			 *       /  \
			 *  Cs(B)    Cr(B)
			 *
			 *  If p is red, set_black(p)
			 * --------------------------->
			 *  If p is black, p is new X
			 *
			 * ******** ******** ******** ******** ******** ********
			 *  OR
			 * ******** ******** ******** ******** ******** ********
			 *
			 *            |
			 *          p(Cp)
			 *          /   \
			 *     X(BB)     S(B)
			 *               /  \
			 *          Cr(B)    Cs(B)
			 *
			 *  flip_color(p); flip_color(S); flip_color(X);
			 * ---------------------------------------------->
			 *
			 *            |
			 *          p(RB_or_BB)
			 *          /    \
			 *      X(B)      s(R)
			 *                /  \
			 *           Cr(B)    Cs(B)
			 *
			 *  If p is red, set_black(p)
			 * --------------------------->
			 *  If p is black, p is new X
			 *
			 */
			rbtree_flip_color(sibling);
			if (rbtree_color_is_black(parent->color)) {
				if(parent != tree->root) {
					cc.child = parent;
					parent = rbtree_parent(cc.child);
					if (rbtree_linking_r(cc.child->link)) {
						sibling = parent->left;
						rotation = rbtree_rr(sibling);
					} else {
						sibling = parent->right;
						rotation = rbtree_lr(sibling);
					}
					goto recursively_fix;
				}
			} else
				rbtree_flip_color(parent);
			return;
		}

		/*
		 * Case 2.2: Child in reverse side is red and child in same side
		 *           is black.
		 *
		 *             |
		 *           p(Cp)
		 *           /   \
		 *       S(B)     X(BB)
		 *       /  \
		 *  Cs(B)    cr(R)
		 *           /
		 *      Gc(B)
		 *
		 *  left_rotate(S);
		 * ----------------->
		 *
		 *                  |
		 *                p(Cp)
		 *                /   \
		 *           cr(R)     X(BB)
		 *           /
		 *       S(B)
		 *       /  \
		 *  Cs(B)    Gc(B)
		 *
		 *  flip_color(cr); flip_color(S)
		 * ------------------------------->
		 *
		 *                  |
		 *                p(Cp)
		 *                /   \
		 *           Cr(B)     X(BB)
		 *           /
		 *       s(R)
		 *       /  \
		 *  Cs(B)    Gc(B)
		 *
		 * ******** ******** ******** ******** ******** ********
		 *  OR
		 * ******** ******** ******** ******** ******** ********
		 *
		 *            |
		 *          p(Cp)
		 *          /   \
		 *     X(BB)     S(B)
		 *               /  \
		 *          cr(R)    Cs(B)
		 *              \
		 *               Gs(B)
		 *
		 *  right_rotate(S);
		 * ------------------>
		 *
		 *            |
		 *          p(Cp)
		 *          /  \
		 *     X(BB)    cr(R)
		 *                  \
		 *                   S(B)
		 *                   /  \
		 *              Gc(B)    Cs(B)
		 *
		 *  flip_color(cr); flip_color(S)
		 * ------------------------------->
		 *
		 *            |
		 *          p(Cp)
		 *          /   \
		 *     X(BB)     Cr(B)
		 *                   \
		 *                    s(R)
		 *                    /  \
		 *               Gc(B)    Cs(B)
		 *
		 * Transform into case 2.3 .
		 */
		rotation = sibling->lpc;
		cc.child = *rbtree_link(lpc);

		/* rbtree_lpc(sibling, lpc); */
		*rbtree_link(lpc) = sibling;
		sibling->lpc = lpc; /* flip_color(S): red */

		/* if (cc.child) { */
		/*	rbtree_lpc(cc.child, rr.reverse->lpc | RBTREE_BLACK); */
		/* else */
		/*	rbtree_lpc_null(rr.reverse->lpc); */
		*rbtree_link(rr.reverse->link) = cc.child;
		if (cc.child)
			cc.child->lpc = rr.reverse->lpc | RBTREE_BLACK;


		/* rbtree_lpc(rr.reverse, rotation | RBTREE_BLACK) */
		*rbtree_link(rotation) = rr.reverse;
		rr.reverse->lpc = rotation | RBTREE_BLACK;  /* flip_color(cr) */

		/* Transform into case 2.3 . */
		sibling = rr.reverse;
		/* parent = parent; */
		goto black_sibling;
	}

	/*
	 * Case 2.3: Child in same side is red.
	 *
	 *             |
	 *           p(Cp)
	 *           /   \
	 *       S(B)     X(BB)
	 *       /  \
	 *  cs(R)    cr(Ccr)
	 *
	 *  right_rotate(p);
	 * ------------------>
	 *  leave color
	 *
	 *             |
	 *           S(Cp)
	 *           /   \
	 *     Cs(RB)     P(B)
	 *                /  \
	 *         cr(Ccr)    X(B)
	 *
	 * ******** ******** ******** ******** ******** ********
	 *  OR
	 * ******** ******** ******** ******** ******** ********
	 *
	 *             |
	 *           p(Cp)
	 *           /   \
	 *      X(BB)     S(B)
	 *                /  \
	 *         cr(Ccr)    cs(R)
	 *
	 *  left_rotate(p);
	 * ------------------>
	 *  leave color
	 *
	 *               |
	 *             S(Cp)
	 *             /   \
	 *         P(B)     cs(RB)
	 *         /  \
	 *     X(B)    cr(Ccr)
	 *
	 *  S leaves color 'B' to cs and inherits color 'Cp' from p.
	 *  p leaves color 'Cp' and inherits one color 'B' from X.
	 *  X leaves one color 'B'.
	 *
	 */
	lpc = parent->lpc;

	/* rbtree_lpc(parent, rotation | RBTREE_BLACK); */
	*rbtree_link(rotation) = parent;
	parent->lpc = rotation | RBTREE_BLACK;

	/* cc.color = rbtree_color(rr.reverse->color); */
	/* rbtree_lpc(rr.reverse, rbtree_lnpos(sibling->lpc) | cc.color); */
	*rbtree_link(sibling->link) = rr.reverse;
	if (rr.reverse) {
		cc.color = rbtree_color(rr.reverse->color);
		rr.reverse->lpc = rbtree_lnpos(sibling->lpc) | cc.color;
	}

	/* rbtree_lpc(sibling, lpc); */
	sibling->lpc = lpc; /* Inherit color. */
	*rbtree_link(lpc) = sibling;

	rbtree_set_black(sl.same); /* Inherit color of sibling */
}
