#ifndef JEP_FILE_H
#define JEP_FILE_H

#include "osdev64/axiom.h"

#define IO_BUF_SIZE 1024

typedef struct k_finfo {
  int type;     // type
  char* buf;    // buffer
  char* writer; // write pointer
  char* reader; // read pointer
}k_finfo;


/**
 * Creates a new k_info object.
 *
 * Params:
 *   int - the type of file
 *
 * Returns:
 *   k_finfo* - a pointer to a new file info struct of NULL on failure
 */
k_finfo* k_file_create_info(int);

#endif