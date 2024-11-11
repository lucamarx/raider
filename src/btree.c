#include <btree.h>
#include <stdlib.h>


BTNode* btree_new(BTKey key) {
  BTNode* n = (BTNode*) malloc(sizeof(BTNode));

  n->key = key;
  n->value = NULL;

  n->left = NULL;
  n->right = NULL;

  return n;
}


void btree_set(BTNode* root, BTKey key, void* value) {
  BTNode* tmp;

  if (key == root->key)
    root->value = value;
  else if (key < root->key) {
    if (root->left == NULL) {
      tmp = btree_new(key);
      tmp->value = value;

      root->left = tmp;
    }
    else btree_set(root->left, key, value);
  }
  else if (key > root->key) {
    if (root->right == NULL) {
      tmp = btree_new(key);
      tmp->value = value;

      root->right = tmp;
    }
    else btree_set(root->right, key, value);
  }
}


void* btree_get(const BTNode* root, BTKey key) {
  if (root == NULL)    return NULL;

  if (key < root->key) return btree_get(root->left, key);

  if (key > root->key) return btree_get(root->right, key);

  return root->value;
}


bool btree_has_key(const BTNode* root, BTKey key) {
  return btree_get(root, key) != NULL;
}


void btree_traverse(BTNode* root, void* (*f)(BTKey key, void* val, void* ctx), void* ctx) {
  if (root == NULL) return;

  if (root->value != NULL)
    root->value = f(root->key, root->value, ctx);

  btree_traverse(root->left, f, ctx);
  btree_traverse(root->right, f, ctx);
}


BTKey btree_cantor(BTKey k1, BTKey k2) {
  // https://stackoverflow.com/questions/919612/mapping-two-integers-to-one-in-a-unique-and-deterministic-way
  // https://en.wikipedia.org/wiki/Cantor_pairing_function
  return (k1 + k2) * (k1 + k2 + 1) / 2 + k1;
}


void btree_free(BTNode* root) {
  BTNode* l = root->left;
  BTNode* r = root->right;

  if (root->value != NULL) free(root->value);

  free(root);

  if (l != NULL) btree_free(l);
  if (r != NULL) btree_free(r);
}
