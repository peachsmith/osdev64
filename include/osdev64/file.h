#ifndef JEP_FILE_H
#define JEP_FILE_H

#include "osdev64/axiom.h"

#define __FILE_NO_STDIN  1
#define __FILE_NO_STDOUT 2
#define __FILE_NO_STDERR 3
#define __FILE_NO_STDDBG 4

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