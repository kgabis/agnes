#ifndef tests_common_h
#define tests_common_h

#include <stdint.h>
#include <stdbool.h>

#ifdef AGNES_XCODE
#include "agnes.h"
#else
#include "../agnes.h"
#endif

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))

unsigned input_to_number(const agnes_input_t* input);
void number_to_input(unsigned input, agnes_input_t* out_input);
void* read_file(const char *filename, size_t *out_len);

#define DJB2_INITIAL_HASH 5381
uint32_t djb2_hash(void *data, size_t data_size);
uint32_t djb2_hash_incremental(uint32_t current, uint32_t val);

#endif /* tests_common_h */
