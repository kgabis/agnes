#include "tests_common.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef AGNES_XCODE
#include "agnes.h"
#else
#include "../agnes.h"
#endif

#define KGFLAGS_IMPLEMENTATION
#include "deps/kgflags.h"

unsigned input_to_number(const agnes_input_t* input) {
    unsigned res = 0;
    res |= input->a      << 0;
    res |= input->b      << 1;
    res |= input->select << 2;
    res |= input->start  << 3;
    res |= input->up     << 4;
    res |= input->down   << 5;
    res |= input->left   << 6;
    res |= input->right  << 7;
    return res;
}

void number_to_input(unsigned input, agnes_input_t* out_input) {
    out_input->a      = (input >> 0) & 0x1;
    out_input->b      = (input >> 1) & 0x1;
    out_input->select = (input >> 2) & 0x1;
    out_input->start  = (input >> 3) & 0x1;
    out_input->up     = (input >> 4) & 0x1;
    out_input->down   = (input >> 5) & 0x1;
    out_input->left   = (input >> 6) & 0x1;
    out_input->right  = (input >> 7) & 0x1;
}

void* read_file(const char *filename, size_t *out_len) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        return NULL;
    }
    fseek(fp, 0L, SEEK_END);
    long pos = ftell(fp);
    if (pos < 0) {
        fclose(fp);
        return NULL;
    }
    size_t file_size = pos;
    rewind(fp);
    unsigned char *file_contents = (unsigned char *)malloc(file_size);
    if (!file_contents) {
        fclose(fp);
        return NULL;
    }
    if (fread(file_contents, file_size, 1, fp) < 1) {
        if (ferror(fp)) {
            fclose(fp);
            free(file_contents);
            return NULL;
        }
    }
    fclose(fp);
    *out_len = file_size;
    return file_contents;
}

uint32_t djb2_hash(void *data, size_t data_size) {
    uint8_t *data_bytes = (uint8_t*)data;
    uint32_t hash = DJB2_INITIAL_HASH;
    for (size_t i = 0; i < data_size; i++) {
        uint8_t b = data_bytes[i];
        hash  = ((hash << 5) + hash) + b;
    }
    return hash;
}

uint32_t djb2_hash_incremental(uint32_t current_hash, uint32_t val) {
    uint32_t hash = current_hash;
    for (int i = 0; i < 4; i++) {
        hash = ((hash << 5) + hash) + (val & 0xff);
        val >>= 8;
    }
    return hash;
}
