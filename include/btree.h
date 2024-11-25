/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Luca Marx
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
