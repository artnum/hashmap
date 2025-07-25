/**
 * comprehensive_test.c
 *
 * A more thorough test suite for the hashmap implementation.
 *
 * This suite verifies:
 * - Basic create, set, get, delete, and destroy operations.
 * - Correct handling of key overwrites.
 * - Behavior with non-existent keys.
 * - Correctness of the iteration functionality.
 * - Memory management using the custom free function.
 * - Collision handling and dynamic bucket growth.
 * - Dynamic bucket shrinking after many deletions.
 *
 * To compile and run (assuming you are in the root of the project):
 * gcc -o test/comprehensive_test test/comprehensive_test.c src/hashmap.c
 * -I./src/include -std=c99 -Wall -Wextra
 * ./test/comprehensive_test
 */
#include "../src/include/hashmap.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A simple counter to track how many times our custom free function is called.
static int free_counter = 0;

// --- Test Utilities ---

// A simple assertion macro to keep tests clean.
#define TEST_ASSERT(condition, message)                                        \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "Assertion failed: (%s), in %s, line %d: %s\n",          \
              #condition, __FILE__, __LINE__, message);                        \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

// A custom hash function designed to create predictable collisions.
// It hashes based on the first character of the key, forcing all keys
// starting with the same letter into the same primary bucket.
uint64_t collision_hash(const char *key, size_t len) {
  (void)len; // Unused parameter
  if (!key || *key == '\0') {
    return 0;
  }
  // The primary key is based on the first character.
  // The secondary key is a simple checksum of the rest of the string.
  uint64_t pkey = (uint64_t)key[0];
  uint32_t skey = 0;
  for (size_t i = 1; key[i] != '\0'; ++i) {
    skey += key[i];
  }
  return (pkey | ((uint64_t)skey << 32));
}

// A custom free function that increments a counter.
void counting_free(void *data) {
  free(data);
  free_counter++;
}

// --- Test Cases ---

void test_basic_operations() {
  printf("Running: %s\n", __FUNCTION__);
  free_counter = 0;

  // Use the collision hash to have predictable keys
  HashMap *map = hashmap_create(128, collision_hash, counting_free);
  TEST_ASSERT(map != NULL, "hashmap_create should not return NULL.");

  char *value1 = strdup("value1");
  TEST_ASSERT(hashmap_set(map, "key1", value1), "hashmap_set should succeed.");

  char *retrieved = hashmap_get(map, "key1");
  TEST_ASSERT(retrieved != NULL, "hashmap_get should find the key.");
  TEST_ASSERT(strcmp(retrieved, "value1") == 0,
              "Retrieved value should be correct.");

  // Test overwrite
  char *value2 = strdup("value2");
  TEST_ASSERT(hashmap_set(map, "key1", value2),
              "hashmap_set (overwrite) should succeed.");
  TEST_ASSERT(free_counter == 1,
              "Old value should have been freed on overwrite.");

  retrieved = hashmap_get(map, "key1");
  TEST_ASSERT(strcmp(retrieved, "value2") == 0,
              "Retrieved value should be the new value.");

  // Test get non-existent key
  retrieved = hashmap_get(map, "nonexistent");
  TEST_ASSERT(retrieved == NULL,
              "hashmap_get for non-existent key should return NULL.");

  // Test delete
  void *deleted_data = NULL;
  TEST_ASSERT(hashmap_delete(map, "key1", &deleted_data),
              "hashmap_delete should succeed.");
  TEST_ASSERT(deleted_data != NULL, "Deleted data should be returned.");
  TEST_ASSERT(strcmp((char *)deleted_data, "value2") == 0,
              "Returned deleted data is incorrect.");
  free(deleted_data); // Manually free the returned data

  retrieved = hashmap_get(map, "key1");
  TEST_ASSERT(retrieved == NULL, "Key should not exist after deletion.");

  hashmap_destroy(map);
  // free_counter should be 1 because the first value was freed on overwrite,
  // and the second was returned from delete, not freed by destroy.
  TEST_ASSERT(free_counter == 1, "Final free count should be correct.");
  printf("PASSED: %s\n\n", __FUNCTION__);
}

void test_collisions_and_bucket_growth() {
  printf("Running: %s\n", __FUNCTION__);
  free_counter = 0;

  // Create a map with a small capacity to make collisions more likely.
  // Use the collision_hash to force all keys into the same bucket.
  HashMap *map = hashmap_create(1, collision_hash, counting_free);
  TEST_ASSERT(map != NULL, "hashmap_create should not return NULL.");

  // HASH_MAP_BUCKET_SIZE defaults to 8. We will insert 10 items.
  // All keys start with 'a', so they will all map to the same primary bucket.
  const int num_items = 10;
  char key[16];
  char value[16];

  for (int i = 0; i < num_items; ++i) {
    sprintf(key, "a_key_%d", i);
    sprintf(value, "value_%d", i);
    TEST_ASSERT(hashmap_set(map, key, strdup(value)),
                "hashmap_set should succeed during growth.");
  }

  // Verify all items are retrievable after collisions and growth
  for (int i = 0; i < num_items; ++i) {
    sprintf(key, "a_key_%d", i);
    sprintf(value, "value_%d", i);
    char *retrieved = hashmap_get(map, key);
    TEST_ASSERT(retrieved != NULL,
                "Should be able to retrieve key after bucket growth.");
    TEST_ASSERT(strcmp(retrieved, value) == 0,
                "Retrieved value should be correct after growth.");
  }

  // Check the capacity of the grown bucket (should be 16, since it starts at 8
  // and doubles) Note: This is a white-box test, peeking into the
  // implementation details.
  HashMapBucket *bucket = &map->table[((uint64_t)'a') % map->capacity];
  TEST_ASSERT(bucket->capacity == 16,
              "Bucket capacity should have doubled from 8 to 16.");
  TEST_ASSERT(bucket->count == 10, "Bucket count should be correct.");

  hashmap_destroy(map);
  TEST_ASSERT(free_counter == num_items,
              "All items should be freed by destroy.");
  printf("PASSED: %s\n\n", __FUNCTION__);
}

void test_bucket_shrinking() {
  printf("Running: %s\n", __FUNCTION__);
  free_counter = 0;

  // Use a hash that forces collisions into one bucket
  HashMap *map = hashmap_create(1, collision_hash, counting_free);
  TEST_ASSERT(map != NULL, "hashmap_create should not return NULL.");

  // HASH_MAP_BUCKET_SIZE defaults to 8.
  // Insert 9 items to force the bucket to grow from 8 to 16.
  const int initial_items = 9;
  char key[20];
  for (int i = 0; i < initial_items; i++) {
    sprintf(key, "a_key_%d", i);
    hashmap_set(map, key, strdup("some_data"));
  }

  // White-box test: check that the bucket has grown.
  HashMapBucket *bucket = &map->table[((uint64_t)'a') % map->capacity];
  TEST_ASSERT(bucket->capacity == 16,
              "Bucket should have grown to capacity 16.");
  TEST_ASSERT(bucket->count == 9, "Bucket count should be 9.");

  // Now, delete items to trigger a shrink.
  // A common shrink condition is when load factor < 25%.
  // Deleting 7 items will leave 2. Load factor 2/16 = 12.5%, which should
  // trigger a shrink. The bucket of capacity 16 should shrink back to 8.
  const int items_to_delete = 7;
  for (int i = 0; i < items_to_delete; i++) {
    sprintf(key, "a_key_%d", i);
    void *data = NULL;
    hashmap_delete(map, key, &data);
    // The data is returned by hashmap_delete, so we must free it manually.
    TEST_ASSERT(data != NULL, "Key exists, data should be returned.");
    if (data) {
      free(data);
    }
  }

  // White-box test: check that the bucket has shrunk.
  TEST_ASSERT(bucket->capacity == 8,
              "Bucket should have shrunk back to capacity 8.");
  TEST_ASSERT(bucket->count == 2, "Bucket count should be 2 after deletions.");

  // Verify the remaining items are still accessible and correct.
  char *retrieved;
  // Keys "a_key_7" and "a_key_8" should still be in the map.
  retrieved = hashmap_get(map, "a_key_7");
  TEST_ASSERT(retrieved != NULL, "Should find key 'a_key_7' after shrink.");
  TEST_ASSERT(strcmp(retrieved, "some_data") == 0,
              "Value for 'a_key_7' is incorrect.");

  retrieved = hashmap_get(map, "a_key_8");
  TEST_ASSERT(retrieved != NULL, "Should find key 'a_key_8' after shrink.");
  TEST_ASSERT(strcmp(retrieved, "some_data") == 0,
              "Value for 'a_key_8' is incorrect.");

  // Verify a deleted key is gone.
  retrieved = hashmap_get(map, "a_key_0");
  TEST_ASSERT(retrieved == NULL, "Deleted key 'a_key_0' should not be found.");

  // Clean up. The remaining 2 items should be freed by destroy.
  hashmap_destroy(map);
  TEST_ASSERT(free_counter == 2, "Destroy should free the 2 remaining items.");

  printf("PASSED: %s\n\n", __FUNCTION__);
}

static int iter_count = 0;
void iteration_callback(HashMapBucketKey key, void *data) {
  (void)key;
  (void)data;
  iter_count++;
}

void test_iteration() {
  printf("Running: %s\n", __FUNCTION__);
  free_counter = 0;
  iter_count = 0;

  HashMap *map = hashmap_create(10, collision_hash, free);
  TEST_ASSERT(map != NULL, "hashmap_create should not return NULL.");

  hashmap_set(map, "iter_key1", strdup("data1"));
  hashmap_set(map, "iter_key2", strdup("data2"));
  hashmap_set(map, "another_key", strdup("data3"));

  hashmap_iterate(map, iteration_callback);
  TEST_ASSERT(iter_count == 3, "Iterator should visit all 3 items.");

  // Test iteration on an empty map
  iter_count = 0;
  HashMap *empty_map = hashmap_create(10, collision_hash, NULL);
  hashmap_iterate(empty_map, iteration_callback);
  TEST_ASSERT(iter_count == 0, "Iterator should not run on an empty map.");

  hashmap_destroy(map);
  hashmap_destroy(empty_map);
  printf("PASSED: %s\n\n", __FUNCTION__);
}

int main() {
  printf("--- Starting Hashmap Test Suite ---\n\n");

  test_basic_operations();
  test_collisions_and_bucket_growth();
  test_bucket_shrinking(); // New test case
  test_iteration();

  printf("--- All tests passed successfully! ---\n");

  return EXIT_SUCCESS;
}
