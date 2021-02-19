#include "osdev64/file.h"
#include "osdev64/memory.h"
#include "osdev64/heap.h"

#include "klibc/stdio.h"

k_finfo* k_file_create_info(int type)
{
  k_finfo* info = (k_finfo*)k_heap_alloc(sizeof(k_finfo));

  if (info == NULL)
  {
    return NULL;
  }

  switch (type)
  {
  case __FILE_NO_STDIN:
  case __FILE_NO_STDOUT:
  case __FILE_NO_STDERR:
  {
    info->buf = (char*)k_heap_alloc(IO_BUF_SIZE);
    if (info->buf == NULL)
    {
      return NULL;
    }

    info->type = type;
    info->writer = info->buf;
    info->reader = info->buf;
  }
  break;

  default:
  {
    info->buf = NULL;
  }
  break;
  }

  info->writer = info->buf;
  info->reader = info->buf;

  return info;
}
