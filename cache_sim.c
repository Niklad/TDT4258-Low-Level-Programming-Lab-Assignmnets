#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef enum { dm, fa } cache_map_t;
typedef enum { uc, sc } cache_org_t;
typedef enum { instruction, data } access_t;

typedef struct {
  uint32_t address;
  access_t accesstype;
} mem_access_t;

typedef struct {
  uint64_t accesses;
  uint64_t hits;
} cache_stat_t;

// DECLARE CACHES AND COUNTERS FOR THE STATS HERE

uint32_t cache_size;
const uint32_t block_size = 64;
const uint32_t address_size = 32;
cache_map_t cache_mapping;
cache_org_t cache_org;

// USE THIS FOR YOUR CACHE STATISTICS
cache_stat_t cache_statistics;


void read_params_and_init(int argc, char** argv);

FILE* read_access_from_file(char *file_name);

void dm_uc_cache(FILE* ptr_file);

void dm_sc_cache(FILE* ptr_file);

void fa_uc_cache(FILE* ptr_file);

void fa_sc_cache(FILE* ptr_file);

mem_access_t read_transaction(FILE* ptr_file);

void print_statistics(cache_stat_t cache_statistics);


void main(int argc, char** argv) {
  // Reset statistics:
  memset(&cache_statistics, 0, sizeof(cache_stat_t));

  read_params_and_init(argc, argv);

  FILE* ptr_file = read_access_from_file("mem_trace.txt");

  // Branch to the respective cache function given the cache parameters
  if (cache_mapping == dm) {
    if (cache_org == uc) {
      dm_uc_cache(ptr_file);
    } else if (cache_org == sc) {
      dm_sc_cache(ptr_file);
    }
  } else if (cache_mapping == fa) {
    if (cache_org == uc) {
      fa_uc_cache(ptr_file);
    } else if (cache_org == sc) {
      fa_sc_cache(ptr_file);
    }
  }

  print_statistics(cache_statistics);

  /* Close the trace file */
  fclose(ptr_file);
}


/* Reads a memory access from the trace file and returns
 * 1) access type (instruction or data access
 * 2) memory address
 */
mem_access_t read_transaction(FILE* ptr_file) {
  char type;
  mem_access_t access;

  if (fscanf(ptr_file, "%c %x\n", &type, &access.address) == 2) {
    if (type != 'I' && type != 'D') {
      printf("Unkown access type\n");
      exit(0);
    }
    access.accesstype = (type == 'I') ? instruction : data;
    return access;
  }

  /* If there are no more entries in the file,
   * return an address 0 that will terminate the infinite loop in main
   */
  access.address = 0;
  return access;
}


void read_params_and_init(int argc, char** argv){
  /* Read command-line parameters and initialize:
   * cache_size, cache_mapping and cache_org variables
   */
  /* IMPORTANT: *IF* YOU ADD COMMAND LINE PARAMETERS (you really don't need to),
   * MAKE SURE TO ADD THEM IN THE END AND CHOOSE SENSIBLE DEFAULTS SUCH THAT WE
   * CAN RUN THE RESULTING BINARY WITHOUT HAVING TO SUPPLY MORE PARAMETERS THAN
   * SPECIFIED IN THE UNMODIFIED FILE (cache_size, cache_mapping and cache_org)
   */
  if (argc != 4) { /* argc should be 2 for correct execution */
    printf(
        "Usage: ./cache_sim [cache size: 128-4096] [cache mapping: dm|fa] "
        "[cache organization: uc|sc]\n");
    exit(0);
  } else {
    /* argv[0] is program name, parameters start with argv[1] */

    /* Set cache size */
    cache_size = atoi(argv[1]);

    /* Set Cache Mapping */
    if (strcmp(argv[2], "dm") == 0) {
      cache_mapping = dm;
    } else if (strcmp(argv[2], "fa") == 0) {
      cache_mapping = fa;
    } else {
      printf("Unknown cache mapping\n");
      exit(0);
    }

    /* Set Cache Organization */
    if (strcmp(argv[3], "uc") == 0) {
      cache_org = uc;
    } else if (strcmp(argv[3], "sc") == 0) {
      cache_org = sc;
    } else {
      printf("Unknown cache organization\n");
      exit(0);
    }
  }
}

void dm_uc_cache(FILE* ptr_file) {
  mem_access_t access;

  // Generate cache of specified size
  uint32_t num_of_blocks = cache_size/block_size;
  uint32_t index_num_of_bits = floor(log2(num_of_blocks));
  uint32_t block_offset_num_of_bits = floor(log2(block_size));         // 6 bits for indexing into 64B
  uint32_t tag_num_of_bits = address_size - block_offset_num_of_bits - index_num_of_bits;


  /* Loop until whole trace file has been read */
  while (1) {

    access = read_transaction(ptr_file);
    // If no transactions left, break out of loop
    if (access.address == 0) break;
    printf("%d %x\n", access.accesstype, access.address);
    /* Do a cache access */
    // ADD YOUR CODE HERE
    if ((access.address&(0x3FFFFF<<address_size)) == (0x8cda3fa8&(0x3FFFFF<<address_size))) {
      // Hit!
      cache_statistics.hits++;
    } else {
      // no hit
    }

    cache_statistics.accesses++;
  }
}

void dm_sc_cache(FILE* ptr_file) {
  mem_access_t access;

  /* Loop until whole trace file has been read */
  while (1) {
    access = read_transaction(ptr_file);
    // If no transactions left, break out of loop
    if (access.address == 0) break;
    printf("%d %x\n", access.accesstype, access.address);
    /* Do a cache access */
    // ADD YOUR CODE HERE


    cache_statistics.accesses++;
  }
}

void fa_uc_cache(FILE* ptr_file) {
  mem_access_t access;

  /* Loop until whole trace file has been read */
  while (1) {
    access = read_transaction(ptr_file);
    // If no transactions left, break out of loop
    if (access.address == 0) break;
    printf("%d %x\n", access.accesstype, access.address);
    /* Do a cache access */
    // ADD YOUR CODE HERE
    cache_statistics.accesses++;
  }
}

void fa_sc_cache(FILE* ptr_file) {
  mem_access_t access;

  /* Loop until whole trace file has been read */
  while (1) {
    access = read_transaction(ptr_file);
    // If no transactions left, break out of loop
    if (access.address == 0) break;
    printf("%d %x\n", access.accesstype, access.address);
    /* Do a cache access */
    // ADD YOUR CODE HERE
    cache_statistics.accesses++;
  }
}


FILE* read_access_from_file(char *file_name){
    /* Open the file mem_trace.txt to read memory accesses */
  FILE* ptr_file;
  ptr_file = fopen(file_name, "r");
  if (!ptr_file) {
    printf("Unable to open the trace file\n");
    exit(1);
  }
  return ptr_file;
}


void print_statistics(cache_stat_t cache_statistics){
  /* Print the statistics */
  // DO NOT CHANGE THE FOLLOWING LINES!
  printf("\nCache Statistics\n");
  printf("-----------------\n\n");
  printf("Accesses: %ld\n", cache_statistics.accesses);
  printf("Hits:     %ld\n", cache_statistics.hits);
  printf("Hit Rate: %.4f\n",
         (double)cache_statistics.hits / cache_statistics.accesses);
}
