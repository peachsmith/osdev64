
#include "osdev64/syscall.h"
#include "osdev64/task.h"
#include "osdev64/instructor.h"
#include "osdev64/file.h"
#include "osdev64/serial.h"

#include "klibc/stdio.h"


k_regn k_syscall(
  k_regn id,
  k_regn data1,
  k_regn data2,
  k_regn data3,
  k_regn data4
)
{
  switch (id)
  {

  case SYSCALL_SLEEP_SYNC:
  {
    // data1 is the register stack
    // data2 is the synchronization type
    // data3 is the synchronization value
    k_regn* next = k_task_sleep((k_regn*)data1, (k_regn*)data3, data2, 0);

    return PTR_TO_N(next);
  }

  case SYSCALL_SLEEP_TICK:
  {
    // data1 is the register stack
    // data2 is the tick limit
    k_regn* next = k_task_sleep((k_regn*)data1, NULL, 3, data2);

    return PTR_TO_N(next);
  }

  case SYSCALL_STOP:
  {
    k_regn* next = k_task_stop((k_regn*)data1);
    return PTR_TO_N(next);
  }

  case SYSCALL_WRITE:
  {
    FILE* f = (FILE*)data1;   // file
    char* src = (char*)data2; // source buffer
    size_t n = (size_t)data3; // number of bytes to write
    k_regn count = 0;

    if (f == NULL)
    {
      return 0;
    }

    k_finfo* info = (k_finfo*)f->info;

    if (info == NULL)
    {
      return 0;
    }

    // Standard output or standard error.
    if (info->type == __FILE_NO_STDOUT || info->type == __FILE_NO_STDERR)
    {
      char* writer = info->writer;

      for (size_t i = 0; i < n; i++)
      {
        char* next = writer + 1;

        // Reset the writer pointer if we've reached the
        // end of the output stream buffer.
        if (next >= info->buf + (IO_BUF_SIZE - 1))
        {
          next = info->buf;
        }

        // If the writer is about to overtake the reader,
        // then don't write anything.
        if (next == info->reader)
        {
          return 0;
        }

        *writer = src[i];
        writer = next;
        count++;
      }

      // Update the writer pointer.
      info->writer = writer;
    }

    // Debug output
    if (info->type == __FILE_NO_STDDBG)
    {
      char* dst = info->buf;
      char* writer = info->writer;

      for (size_t i = 0; i < n; i++)
      {
        k_serial_com1_putc(src[i]);
        count++;
      }
    }

    return count;
  }


  case SYSCALL_READ:
  {
    FILE* f = (FILE*)data1;   // file
    char* dst = (char*)data2; // destination buffer
    size_t n = (size_t)data3; // number of bytes to read
    k_regn count = 0;

    if (f == NULL)
    {
      return 0;
    }

    k_finfo* info = (k_finfo*)f->info;

    if (info == NULL)
    {
      return 0;
    }

    // Standard output or standard error.
    if (info->type == __FILE_NO_STDOUT || info->type == __FILE_NO_STDERR)
    {
      char* reader = info->reader;

      // If the reader is equal to the writer, then there
      // are no new bytes to read.
      if (reader == info->writer)
      {
        return count;
      }

      for (size_t i = 0; i < n; i++)
      {
        char* next = reader + 1;

        // Reset the writer pointer if we've reached the
        // end of the output stream buffer.
        if (next >= info->buf + (IO_BUF_SIZE - 1))
        {
          next = info->buf;
        }

        dst[i] = *reader;
        reader = next;

        count++;

        // If the reader has overtaken the writer,
        // then we've finished reading.
        if (reader == info->writer)
        {
          // Update the writer pointer.
          info->reader = reader;
          return count;
        }
      }

      // Update the writer pointer.
      info->reader = reader;
    }

    return count;
  }


  // TODO: remove this
  case SYSCALL_FACE:
    // fprintf(stddbg, "This is the FACE syscall. Data: %llX\n", data1);
    return PTR_TO_N(NULL);

  default:
    return PTR_TO_N(NULL);
  }
}
