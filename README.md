# C HashMap

A lightweight hash map implementation in C, mapping string keys to arbitrary data pointers (void*).

## License

This project is licensed under the MIT License.

## Overview

This hash map provides basic key-value storage and retrieval operations for string keys and generic values. It supports insertion, lookup, deletion, iteration, and destruction. The implementation is designed for efficiency in scenarios where the primary table size can be estimated upfront, avoiding costly global resizes. It uses a user-provided 64-bit hash function (e.g., from XXHash) and optionally a function to free stored items during deletion or destruction.

Key features:

- String keys (const char*).
- Generic values (void*).
- Custom hash function for flexibility.
- Optional item-freeing callback.
- Iteration callback for traversing entries.

## Design Choices

Several design decisions were made to balance performance, memory usage, and simplicity:

- **Two-level Hashing**: A fixed-capacity primary hash table (set at creation) where each bucket is a dynamic secondary structure (array). The 64-bit hash is split into two 32-bit parts: the lower bits (pkey) index into the primary table, and the upper bits (skey) are used for probing in the secondary bucket. This avoids resizing the entire map, only growing individual buckets as needed.
  
- **Collision Resolution**: Linear probing within each secondary bucket. Probing starts at `skey % bucket->capacity` and wraps around. This is simple and cache-friendly but may degrade if buckets become heavily loaded (though buckets double in size when full).

- **Dynamic Bucket Growth**: Secondary buckets start empty and allocate/grow (doubling capacity, starting from `HASH_MAP_BUCKET_SIZE` which defaults to 8) only when insertions would exceed capacity. A temporary buffer (`_tmp`) is reused across resizes to avoid repeated allocations during rehashing.

- **Fixed Primary Capacity**: No global resizing to prevent pauses in real-time or performance-sensitive applications. Choose an initial capacity based on expected load (e.g., a prime number for better distribution).

- **Tombstones in Deletion**: Deletions mark slots as empty (data = NULL, keys = 0) without shifting elements, preserving probe sequences. This keeps operations O(1) amortized but may lead to wasted space if deletions are frequent.

- **Memory Management**: Uses calloc/realloc/free for allocations. A reusable temp buffer minimizes overhead during bucket resizes. Users can provide a free_item callback for custom cleanup of values.

- **Hash Function Flexibility**: Requires a user-provided 64-bit hash function (signature: uint64_t hash(const char* key, size_t len)). This allows integration with libraries like XXHash or custom implementations. XXHash is recommended for its speed and quality.

- **Minimal Dependencies**: Relies on standard C libraries: <stdlib.h>, <stdint.h>, <stdbool.h>, <string.h>, and <assert.h>. No external dependencies are included by default; users must provide and link their hash function (e.g., link against libxxhash if using XXHash).

- **Thread Safety**: Not thread-safe; users must handle concurrency externally.

- **Error Handling**: Functions like set/grow return bool for success (false on allocation failure). Assertions are used for invalid inputs in debug builds.

These choices prioritize low overhead and predictability over automatic scaling, making it suitable for embedded systems or performance-critical code where load factors are manageable.

## Usage

To use this hash map in your project:

1. **Copy Files**: Add `hashmap.h` and `hashmap.c` to your source directory.

2. **Include Header**: `#include "hashmap.h"` in your code.

3. **Provide Hash Function**: Implement or use an existing 64-bit hash function. Example with XXHash:

   ```c
   #include <xxhash.h>  // User must include this if using XXHash

   uint64_t my_hash(const char *key, size_t len) {
       return XXH64(key, len, 0);  // Seed 0 for determinism
   }
   ```

4. **Link Dependencies**: If using an external hash library like XXHash, compile with the appropriate flags (e.g., `-lxxhash` assuming it's installed).

5. **Compile**: Include `hashmap.c` in your build. Example with gcc:

   ```
   gcc -o myprogram myprogram.c hashmap.c -lxxhash
   ```

   You can define, at compile time, default bucket size by adding `-HASH_MAP_BUCKET_SIZE=__X__` where `__X__` is the size you want.

6. **API Reference**:
   - `HashMap *hashmap_create(size_t capacity, HashMapHashFunction hash_function, HashMapFreeItemFunction free_item)`: Create a map with given primary capacity (must be >0), hash function (required), and optional free_item callback (NULL if not needed).
   - `bool hashmap_set(HashMap *map, const char *key, void *data)`: Insert or update key with data. Returns false on allocation failure.
   - `void *hashmap_get(HashMap *map, const char *key)`: Retrieve data for key, or NULL if not found.
   - `bool hashmap_delete(HashMap *map, const char *key, void **data)`: Remove key, optionally store deleted data in **data. Returns true if found and deleted.
   - `void hashmap_iterate(HashMap *map, HashMapIterateItemFunction callback)`: Call callback(HashMapBucketKey key, void *data) for each entry.
   - `void hashmap_destroy(HashMap *map)`: Free the map and all buckets; calls free_item on values if provided.

## Example

Here's a simple usage example:

```c
#include "hashmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <xxhash.h>  // Assuming XXHash is installed

// Custom hash function using XXHash
uint64_t my_hash(const char *key, size_t len) {
    return XXH64(key, len, 0);
}

// Optional free function for values (here, assuming values are malloc'd strings)
void free_string(void *data) {
    free(data);
}

// Iteration callback example
void print_item(HashMapBucketKey key, void *data) {
    printf("Key (pkey: %u, skey: %u) -> Value: %s\n", key.pkey, key.skey, (char *)data);
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
        printf("Retrieved: %s\n", retrieved);  // Output: Hello
    }

    // Iterate over items
    hashmap_iterate(map, print_item);

    // Delete an item
    void *deleted = NULL;
    if (hashmap_delete(map, "key1", &deleted)) {
        printf("Deleted value: %s\n", (char *)deleted);
        free(deleted);  // Manually free if not using free_item in destroy
    }

    // Clean up
    hashmap_destroy(map);  // Automatically calls free_string on remaining items

    return 0;
}
```

This example demonstrates basic operations. Adjust capacity and hash function based on your needs. For production, add error checking on set operations.

## AI Usage

The hashmap implementation has no code from AI. There is two AI generated code, `test/grok-example.c` written by Grok (xAI), and `test/gemini-test.c` written by Gemini (Google).
This README file was, mainly, generated by Grok.

*Generated with ❤️ by Grok, built by xAI.*
