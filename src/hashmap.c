#include "include/hashmap.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifndef HASH_MAP_BUCKET_SIZE
#define HASH_MAP_BUCKET_SIZE 8
#endif
#define KEY_EQU(a, b) (((a).pkey == (b).pkey) && ((a).skey == (b).skey))

/* Private function */
static HashMapBucketItem *_get_item(HashMap *map, HashMapBucketKey key,
                                    bool empty, HashMapBucket **n) {
  HashMapBucket *node = &map->table[key.pkey % map->capacity];
  if (n) {
    *n = node;
  }
  if (node->capacity == 0) {
    return NULL;
  }
  size_t idx = key.skey % node->capacity;
  size_t i = 0;
  for (i = 0; i < node->capacity; i++) {
    /* find the node */
    if (node->items[(idx + i) % node->capacity].data != NULL &&
        KEY_EQU(node->items[(idx + i) % node->capacity].key, key)) {
      return &node->items[(idx + i) % node->capacity];
    }
    /* accept empty node because we are in set, first empty is given */
    if (empty && node->items[(idx + i) % node->capacity].data == NULL) {
      return &node->items[(idx + i) % node->capacity];
    }
  }
  return NULL;
}

static bool _grow_node_if_needed(HashMap *map, HashMapBucketKey key) {
  HashMapBucket *node = &map->table[key.pkey % map->capacity];
  if (node->count + 1 < node->capacity) {
    return true;
  }

  size_t new_capacity =
      (node->capacity == 0 ? HASH_MAP_BUCKET_SIZE : (node->capacity * 2));
  size_t new_size = new_capacity * sizeof(*node->items);
  if (node->capacity > 0) {
    if (map->_tmp_capacity < node->capacity) {
      void *tmp2 = realloc(map->_tmp, node->capacity * sizeof(*map->_tmp));
      if (!tmp2) {
        /* we don't realloc here, extra size is ok */
        return false;
      }
      map->_tmp = tmp2;
      map->_tmp_capacity = node->capacity;
    }

    memcpy(map->_tmp, node->items, sizeof(*node->items) * node->capacity);
    void *tmp = realloc(node->items, new_size);
    if (!tmp) {
      return false;
    }

    size_t old_capacity = node->capacity;
    node->items = tmp;
    node->capacity = new_capacity;
    memset(node->items, 0, sizeof(*node->items) * node->capacity);
    size_t i = 0;
    size_t j = 0;
    for (i = 0; i < old_capacity; i++) {
      size_t idx = map->_tmp[i].key.skey % node->capacity;
      for (j = 0; j < node->capacity; j++) {
        if (node->items[(idx + j) % node->capacity].data == NULL) {
          memcpy(&node->items[(idx + j) % node->capacity], &map->_tmp[i],
                 sizeof(map->_tmp[i]));
          break;
        }
      }
    }
  } else {
    node->items = calloc(new_capacity, sizeof(*node->items));
    if (!node->items) {
      return false;
    }
    node->capacity = new_capacity;
  }

  return true;
}

static HashMapBucketKey _compute_key(HashMap *map, const char *key) {
  size_t key_len = strlen(key);
  assert(key_len > 0);
  uint64_t ukey = map->hash_function(key, key_len);
  HashMapBucketKey k = {.pkey = (uint32_t)(ukey & 0xFFFFFFFF),
                        .skey = (uint32_t)(ukey >> 32)};
  return k;
}

/* Public API */
HashMap *hashmap_create(size_t capacity, HashMapHashFunction hash_function,
                        HashMapFreeItemFunction free_item) {
  assert(capacity > 0);
  assert(hash_function != NULL);
  HashMap *map = calloc(1, sizeof(*map) + (sizeof(*map->table) * capacity));
  if (map) {
    map->table = (HashMapBucket *)(((void *)map) + sizeof(*map));
    map->capacity = capacity;
    map->free_item = free_item;
    map->hash_function = hash_function;
  }
  return map;
}

bool hashmap_set(HashMap *map, const char *key, void *data) {
  assert(map != NULL);
  assert(key != NULL);
  HashMapBucket *node = NULL;
  HashMapBucketKey ukey = _compute_key(map, key);
  if (!_grow_node_if_needed(map, ukey)) {
    return false;
  }
  HashMapBucketItem *item = _get_item(map, ukey, true, &node);
  if (!item) {
    return false;
  }
  if (item->data == NULL) {
    node->count++;
  } else {
    if (map->free_item) {
      map->free_item(item->data);
      item->data = NULL;
    }
  }
  item->data = data;
  item->key = ukey;

  return true;
}

void *hashmap_get(HashMap *map, const char *key) {
  assert(map != NULL);
  assert(key != NULL);
  HashMapBucketKey ukey = _compute_key(map, key);
  HashMapBucketItem *item = _get_item(map, ukey, false, NULL);
  if (!item) {
    return NULL;
  }
  return item->data;
}

bool hashmap_delete(HashMap *map, const char *key, void **data) {
  assert(map != NULL);
  assert(key != NULL);
  HashMapBucketKey ukey = _compute_key(map, key);
  HashMapBucket node = map->table[ukey.pkey % map->capacity];
  size_t idx = ukey.skey % node.capacity;
  size_t i = 0;
  for (i = 0; i < node.capacity; i++) {
    if (node.items[(idx + i) % node.capacity].data == NULL) {
      return false;
    }
    if (KEY_EQU(node.items[(idx + i) % node.capacity].key, ukey)) {
      if (data != NULL) {
        *data = node.items[(idx + i) % node.capacity].data;
      }
      node.items[(idx + i) % node.capacity].data = NULL;
      node.items[(idx + i) % node.capacity].key.pkey = 0;
      node.items[(idx + i) % node.capacity].key.skey = 0;
      node.count--;
      return true;
    }
    i++;
  }
  return false;
}

void hashmap_iterate(HashMap *map, HashMapIterateItemFunction callback) {
  size_t i = 0;
  for (i = 0; i < map->capacity; i++) {
    if (map->table[i].capacity == 0) {
      continue;
    }
    size_t j = 0;
    for (j = 0; j < map->table[i].capacity; j++) {
      if (map->table[i].items[j].data != NULL) {
        callback((map->table[i].items[j].key), map->table[i].items[j].data);
      }
    }
  }
}

void hashmap_destroy(HashMap *map) {
  assert(map != NULL);
  size_t i = 0;
  for (i = 0; i < map->capacity; i++) {
    if (map->table[i].items != NULL) {
      if (map->free_item) {
        size_t j = 0;
        for (j = 0; j < map->table[i].capacity; j++) {
          if (map->table[i].items[j].data != NULL) {
            map->free_item(map->table[i].items[j].data);
          }
        }
      }
      free(map->table[i].items);
    }
  }
  if (map->_tmp) {
    free(map->_tmp);
  }
  free(map);
}
