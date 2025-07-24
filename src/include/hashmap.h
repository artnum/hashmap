#ifndef HASH_H__
#define HASH_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint32_t pkey; // primary key used for primary hash table
  uint32_t skey; // secondary key used for secondary hash table
} HashMapNodeKey;

typedef struct {
  HashMapNodeKey key;
  void *data;
} HashMapNodeItem;

typedef struct {
  HashMapNodeItem *items;
  size_t count;
  size_t capacity;
} HashMapNode;

typedef struct {
  HashMapNode *table;
  size_t capacity;

  /* function */
  void (*free_item)(void *data);
  uint64_t (*hash_function)(const char *key, size_t key_len);

  /* when growing a node, we need to copy data, use that */
  HashMapNodeItem *_tmp;
  size_t _tmp_capacity;
} HashMap;

typedef uint64_t (*HashMapHashFunction)(const char *key, size_t len);
typedef void (*HashMapFreeItemFunction)(void *data);
typedef void (*HashMapIterateItemFunction)(HashMapNodeKey key, void *data);

HashMap *hashmap_create(size_t capacity, HashMapHashFunction hash_function,
                        HashMapFreeItemFunction free_item);
void *hashmap_get(HashMap *map, const char *key);
bool hashmap_set(HashMap *map, const char *key, void *data);
bool hashmap_delete(HashMap *map, const char *key, void **data);
void hashmap_iterate(HashMap *map, HashMapIterateItemFunction callback);
void hashmap_destroy(HashMap *table);

#endif /* HASH_H__ */
