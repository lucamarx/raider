#ifndef BTREE_H
#define BTREE_H

#include <stdbool.h>

typedef unsigned long long BTKey;

typedef struct BTNode {
  BTKey key;
  void* value;

  struct BTNode* left;
  struct BTNode* right;
} BTNode;


// create a new btree
BTNode* btree_new(BTKey key);

// insert a value
void  btree_set(BTNode* root, BTKey key, void* val);
// retrieve value by key
void* btree_get(const BTNode* root, BTKey key);

// check if a key exists
bool btree_has_key(const BTNode* root, BTKey key);

// traverse tree
void btree_traverse(BTNode* root, void* (*f)(BTKey key, void* val, void* ctx), void* ctx);

// combine two keys into one
BTKey btree_cantor(BTKey k1, BTKey k2);

// free btree
void btree_free(BTNode* root);
#endif
