#ifndef HASH_MAP_H__
#define HASH_MAP_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* A key */
typedef struct {
  uint32_t pkey; // primary key used for primary hash table
  uint32_t skey; // secondary key used for secondary hash table
} HashMapBucketKey;

/* An item */
typedef struct {
  HashMapBucketKey key;
  void *data;
} HashMapBucketItem;

/* A bucket */
typedef struct {
  HashMapBucketItem *items;
  size_t count;
  size_t capacity;
} HashMapBucket;

/* HashMap struct */
typedef struct {
  HashMapBucket *table;
  size_t capacity;

  /* function */
  void (*free_item)(void *data);
  uint64_t (*hash_function)(const char *key, size_t key_len);

  /* when growing a node, we need to copy data, use that */
  HashMapBucketItem *_tmp;
  size_t _tmp_capacity;
} HashMap;

/**
 * Signature for the hash function
 */
typedef uint64_t (*HashMapHashFunction)(const char *key, size_t len);

/**
 * Signature for the free function
 */
typedef void (*HashMapFreeItemFunction)(void *data);

/**
 * Singature for the iterate callback function
 */
typedef void (*HashMapIterateItemFunction)(HashMapBucketKey key, void *data);

/**
 * Create a new hash map.
 *
 * @param capacity Main index size. It is allocated once and never modified.
 * @param hash_function Hash function to use, the function must return a
 * uint64_t
 * @param free_item If data stored in table must be freed, pass a function. If
 * not, just pass NULL.
 *
 * @return a new hash map or NULL in case of an error.
 */
HashMap *hashmap_create(size_t capacity, HashMapHashFunction hash_function,
                        HashMapFreeItemFunction free_item);
/**
 * Get an item by key. The item must not be freed, it stay in the map untouched.
 *
 * @param map The hash map object
 * @param key The key
 *
 * @return The item data or NULL if not found.
 */
void *hashmap_get(HashMap *map, const char *key);

/**
 * Add an item in the hash map. If the item could not be added, it is not freed.
 *
 * @param map The hash map object.
 * @param key The key where to add.
 * @param data The data you want to associate with the key.
 *
 * @return True in case of success, false owtherwise.
 */
bool hashmap_set(HashMap *map, const char *key, void *data);

/**
 * Remove an item from the hash map. The item is not freed, if you need to free
 * it, pass a pointer to get the value back.
 *
 * @param map The hash map object.
 * @param key The key
 * @param[out] A point to a void * where the data associated with the key is
 * set.
 *
 * @return True if the value is found and deleted, false owtherwise.
 */
bool hashmap_delete(HashMap *map, const char *key, void **data);

/**
 * Call a function on each item.
 * The callback signature is `void f(HashMapBucketKey key, void * item)`
 *
 * @param map The hash map object
 * @param callback The function to call on each item
 */
void hashmap_iterate(HashMap *map, HashMapIterateItemFunction callback);

/**
 * Destroy the hashmap, free everything including the data stored in it.
 *
 * @param map The hash map object
 */
void hashmap_destroy(HashMap *map);

#endif /* HASH_MAP_H__ */
