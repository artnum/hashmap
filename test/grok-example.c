#include "../src/include/hashmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xxhash.h> // Assuming XXHash is installed

// Custom hash function using XXHash
uint64_t my_hash(const char *key, size_t len) { return XXH64(key, len, 0); }

// Optional free function for values (here, assuming values are malloc'd
// strings)
void free_string(void *data) { free(data); }

// Iteration callback example
void print_item(HashMapBucketKey key, void *data) {
  printf("Key (pkey: %u, skey: %u) -> Value: %s\n", key.pkey, key.skey,
         (char *)data);
}

int main() {
  // Create hashmap with capacity 100, custom hash, and free function
  HashMap *map = hashmap_create(100, my_hash, free_string);
  if (!map) {
    fprintf(stderr, "Failed to create hashmap\n");
    return 1;
  }

  // Insert some items (values are malloc'd strings)
  char *val1 = strdup("Hello");
  char *val2 = strdup("World");
  hashmap_set(map, "key1", val1);
  hashmap_set(map, "key2", val2);

  // Get a value
  char *retrieved = (char *)hashmap_get(map, "key1");
  if (retrieved) {
    printf("Retrieved: %s\n", retrieved); // Output: Hello
  }

  // Iterate over items
  hashmap_iterate(map, print_item);

  // Delete an item
  void *deleted = NULL;
  if (hashmap_delete(map, "key1", &deleted)) {
    printf("Deleted value: %s\n", (char *)deleted);
    free(deleted); // Manually free if not using free_item in destroy
  }

  // Clean up
  hashmap_destroy(map); // Automatically calls free_string on remaining items

  return 0;
}
