// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2025 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_aatree.h
/// \brief AA trees code

#include "m_aatree.h"
#include "z_zone.h"

// A partial implementation of AA trees,
// according to the algorithms given on Wikipedia.
// http://en.wikipedia.org/wiki/AA_tree

typedef struct aatree_node_s
{
	void*	key;
	void*	value;

	struct aatree_node_s *left, *right;
} aatree_node_t;

struct aatree_s
{
	aatree_node_t	*root;
	UINT32		flags;
};

aatree_t *M_AATreeAlloc(UINT32 flags)
{
	aatree_t *aatree = Z_Malloc(sizeof (aatree_t), PU_STATIC, NULL);
	aatree->root = NULL;
	aatree->flags = flags;
	return aatree;
}

static void M_AATreeFree_Node(aatree_node_t *node)
{
	if (node->left)
		M_AATreeFree_Node(node->left);
	if (node->right)
		M_AATreeFree_Node(node->right);
	Z_Free(node);
}

void M_AATreeFree(aatree_t *aatree)
{
	if (aatree->root)
		M_AATreeFree_Node(aatree->root);

	Z_Free(aatree);
}

static aatree_node_t *M_AATreeRotateRight(aatree_node_t *node)
{
	aatree_node_t *newnode = node->left;
	newnode->right = node;
	node->left = NULL;
	return newnode;
}

static aatree_node_t *M_AATreeRotateLeft(aatree_node_t *node)
{
	aatree_node_t *newnode = node->right;
	newnode->left = node;
	node->right = NULL;
	return newnode;
}

static aatree_node_t *M_AATreeRebalance(aatree_node_t *node)
{
	if (node->right && !node->left)
	{
		if (node->right->left && !node->right->right)
		{
			node->right = M_AATreeRotateRight(node->right);
			return M_AATreeRotateLeft(node);
		}

		if (node->right->right && !node->right->left)
		{
			return M_AATreeRotateLeft(node);
		}
	}
	else if (node->left && !node->right)
	{
		if (node->left->right && !node->left->left)
		{
			node->left = M_AATreeRotateLeft(node->left);
			return M_AATreeRotateRight(node);
		}

		if (node->left->left && !node->left->right)
		{
			return M_AATreeRotateRight(node);
		}
	}

	return node;
}

static aatree_node_t *M_AATreeSet_Node(aatree_node_t *node, UINT32 flags, void* key, void* value, aatree_comp_t callback, aatree_dealloc_t deallocator)
{
	if (!node)
	{
		// Nothing here, so just add where we are
		node = Z_Malloc(sizeof (aatree_node_t), PU_STATIC, NULL);
		node->key = key;
		if (value && (flags & AATREE_ZUSER))
			Z_SetUser(value, &node->value);
		else
			node->value = value;
		node->left = node->right = NULL;
	}
	else
	{
		if (callback(key, node->key) < 0)
			node->left = M_AATreeSet_Node(node->left, flags, key, value, callback, deallocator);
		else if (callback(key, node->key) > 0)
			node->right = M_AATreeSet_Node(node->right, flags, key, value, callback, deallocator);
		else
		{
			if (value && (flags & AATREE_ZUSER))
				Z_SetUser(value, &node->value);
			else
				node->value = value;

			if (deallocator)
				deallocator(key);
		}

		node = M_AATreeRebalance(node);
	}
	
	return node;
}

void M_AATreeSet(aatree_t *aatree, void* key, void* value, aatree_comp_t callback, aatree_dealloc_t deallocator)
{
	I_Assert(callback != NULL);
	aatree->root = M_AATreeSet_Node(aatree->root, aatree->flags, key, value, callback, deallocator);
}

// Caveat: we don't distinguish between nodes that don't exists
// and nodes with value == NULL.
static void *M_AATreeGet_Node(aatree_node_t *node, void* key, aatree_comp_t callback, aatree_dealloc_t deallocator)
{
	if (node)
	{
		if (callback(key, node->key) == 0)
		{
			if (deallocator)
				deallocator(key);
			return node->value;
		}
		else if(callback(node->key, key) < 0)
			return M_AATreeGet_Node(node->right, key, callback, deallocator);
		else
			return M_AATreeGet_Node(node->left, key, callback, deallocator);
	}

	if (deallocator)
		deallocator(key);
	return NULL;
}

void *M_AATreeGet(aatree_t *aatree, void* key, aatree_comp_t callback, aatree_dealloc_t deallocator)
{
	I_Assert(callback != NULL);
	return M_AATreeGet_Node(aatree->root, key, callback, deallocator);
}

static void M_AATreeIterate_Node(aatree_node_t *node, aatree_iter_t callback)
{
	if (node->left)
		M_AATreeIterate_Node(node->left, callback);
	callback(node->key, node->value);
	if (node->right)
		M_AATreeIterate_Node(node->right, callback);
}

void M_AATreeIterate(aatree_t *aatree, aatree_iter_t callback)
{
	if (aatree->root)
		M_AATreeIterate_Node(aatree->root, callback);
}
