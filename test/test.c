#include "../src/include/hashmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xxhash.h>

void dump(HashMapBucketKey key, void *data) {
  printf("%16lx : %s", *(uint64_t *)&key, (char *)data);
}

int main(int argc, char **argv) {
  char *files[] = {"/usr/share/dict/words", __FILE__, "/usr/include/stdio.h",
                   "src/hashmap.c", 0};
  HashMap *map = hashmap_create(501173, (HashMapHashFunction)XXH3_64bits, free);
  for (int f = 0; files[f] != 0; f++) {
    if (!map) {
      exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(files[f], "r");
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
      if (!hashmap_set(map, buffer, ptr)) {
        free(ptr);
      }
    }

    rewind(fp);
    while (!feof(fp)) {
      fgets(buffer, 500, fp);
      char *ptr = hashmap_get(map, buffer);
      if (strcmp(ptr, buffer) != 0) {
        fprintf(stderr, "IT HAS A BUG\n");
      }
    }

    fclose(fp);

    hashmap_iterate(map, dump);
  }
  hashmap_destroy(map);
  exit(EXIT_SUCCESS);
}
