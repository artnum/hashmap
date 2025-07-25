#include "../src/include/hashmap.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <xxhash.h>

char *strdup(const char *string) {
  assert(string != NULL);
  size_t len = strlen(string);
  char *dup = NULL;
  if (len > 0) {
    dup = calloc(len + 1, sizeof(*dup));
    if (dup) {
      memcpy(dup, string, len);
    }
  }
  return dup;
}

void dump(HashMapBucketKey key, void *data) {
  printf("%8x %8x : %s", key.pkey, key.skey, (char *)data);
}

int main(int argc, char **argv) {
  //  HashMap *map = hashmap_create(501173, (HashMapHashFunction)XXH3_64bits,
  //  free);
  /* my dict has about 350'000 entries, so let's a timing test with different
   * hashmap size value */

  int hashmap_size[] = {1, 10, 13, 587, 7823, 10000, 175000, 174989, 349999, 0};
  int f = 0;
  for (f = 0; hashmap_size[f] != 0; f++) {
    HashMap *map =
        hashmap_create(hashmap_size[f], (HashMapHashFunction)XXH3_64bits, free);
    int64_t timing = 0;
    int64_t timing2 = 0;
    int items = 0;
    int items2 = 0;
    if (!map) {
      exit(EXIT_FAILURE);
    }

    FILE *fp = fopen("/usr/share/dict/words", "r");
    if (!fp) {
      hashmap_destroy(map);
      exit(EXIT_FAILURE);
    }

    char buffer[500];
    /* this loop is buggy (last entry is inserted twice) but hashmap handles it
     */
    while (!feof(fp)) {
      fgets(buffer, 500, fp);
      char *ptr = strdup(buffer);
      struct timespec tp1;
      struct timespec tp2;
      clock_gettime(CLOCK_MONOTONIC, &tp1);
      bool ret = hashmap_set(map, buffer, ptr);
      clock_gettime(CLOCK_MONOTONIC, &tp2);
      if (!ret) {
        fprintf(stderr, "IT HAS A BUG\n");
        free(ptr);
      } else {
        items++;
        timing += ((int64_t)(tp2.tv_sec - tp1.tv_sec) * (int64_t)1000000000UL) +
                  (tp2.tv_nsec - tp1.tv_nsec);
      }
    }

    rewind(fp);
    while (!feof(fp)) {
      fgets(buffer, 500, fp);
      struct timespec tp1;
      struct timespec tp2;
      clock_gettime(CLOCK_MONOTONIC, &tp1);
      char *ptr = hashmap_get(map, buffer);
      clock_gettime(CLOCK_MONOTONIC, &tp2);
      if (strcmp(ptr, buffer) != 0) {
        fprintf(stderr, "IT HAS A BUG\n");
      } else {
        items2++;
        timing2 +=
            ((int64_t)(tp2.tv_sec - tp1.tv_sec) * (int64_t)1000000000UL) +
            (tp2.tv_nsec - tp1.tv_nsec);
      }
    }
    fclose(fp);

    hashmap_destroy(map);
    printf(
        "For size : %06d\t\thashmap_set :  %04ldns for %06d items : (%05.06f "
        "item/ns)\n",
        hashmap_size[f], timing, items, (float)items / timing);
    printf("For size : %06d\t\thashmap_get :  %04ldns for %06d items : "
           "(%05.04f item/ns)\n",
           hashmap_size[f], timing2, items2, (float)items2 / timing2);
  }
  exit(EXIT_SUCCESS);
}
