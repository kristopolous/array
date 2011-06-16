/* Array
 *	array management for shell scripts
 *
 * (c) Copyright 2005, 2008, 2011 Christopher J. McKenzie under the terms of the
 *  GNU Public License, incorporated herein by reference
 */
#ifndef _HASH_H_
#define _HASH_H_

#define VERSION	/*BEGIN_VERSIONATE*/"3.0.2357"/*END_VERSIONATE*/

/*
 * Each one has a type, either leaf (end point) or HashTable (further hash)
 */

enum {
	HASH,
	TUPLE
};

typedef int NodeType;

typedef struct HashTable_struct {
	size_t	size_current;
	size_t	size_maximum,
		size_mask;
	
	struct node_struct**table;
} HashTable;

typedef struct node_struct {
	NodeType	type;

	char*key;

	union {
		char*value;
		HashTable*hash;
	};

} Node;

#endif // _HASH_H_

