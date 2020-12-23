#include "osdev64/uefi.h"


void k_acpi_read()
{
  int acpi_version;
  unsigned char* rsdp;
  unsigned char* rsdt;
  unsigned char* madt = NULL;

  // Get the RSDP from UEFI firmware.
  acpi_version = k_uefi_get_rsdp(&rsdp);
  if (!acpi_version)
  {
    return;
  }

  // Print the RSDP signature (bytes 0 - 7)
  Print(L"RSDP Signature: %c%c%c%c%c%c%c%c\n",
    rsdp[0],
    rsdp[1],
    rsdp[2],
    rsdp[3],
    rsdp[4],
    rsdp[5],
    rsdp[6],
    rsdp[7]
  );

  // Print the OEM ID (bytes 9 - 14)
  Print(L"OEMID: %c%c%c%c%c%c\n",
    rsdp[9],
    rsdp[10],
    rsdp[11],
    rsdp[12],
    rsdp[13],
    rsdp[14]
  );

  // If we're using ACPI >= 2.0, get the XSDT address,
  // otherwise get the RSDT address.
  if (acpi_version == 2)
  {
    // Alright, this probably looks a little silly.
    // Byte 24 of the RSDP structure is the beginning of 8
    // bytes of memory which contain the address of the XSDT.
    // So first, we cast the address of byte 24 of the RSDP as a
    // pointer to a 64-bit unsigned integer.
    // We then dereference that pointer to get the value, and we
    // cast that value as a pointer to the XSDT.
    rsdt = (unsigned char*)(*(uint64_t*)(rsdp + 24));
  }
  else
  {
    // If we're using the 32-bit RSDP, cast it as a 64-bit
    // integer before casting as a pointer, since addresses
    // are 64 bits.
    rsdt = (unsigned char*)((uint64_t)(*(uint32_t*)(rsdp + 16)));
  }

  // All SDTs start with an SDT header.
  // The SDT header is 36 bytes in the following structure:
  // ---------------------------------------------------------------------
  // bytes       purpose
  // ---------------------------------------------------------------------
  // 0 -3        signatrure
  // 4 - 7       length of the table (including the header)
  // 8           revision number
  // 9           checksum
  // 10 -15      OEM ID
  // 16 - 23     OEM table ID
  // 24 - 27     OEM revision number
  // 28 - 31     creator ID
  // 32 - 35     creator revision
  // ---------------------------------------------------------------------

  // Print the RSDT signature (bytes 0 - 4)
  Print(L"RSDT signature %c%c%c%c\n",
    rsdt[0],
    rsdt[1],
    rsdt[2],
    rsdt[3]
  );

  // Print the OEM ID (bytes 10 - 15)
  Print(L"RSDT OEMID: %c%c%c%c%c%c\n",
    rsdt[10],
    rsdt[11],
    rsdt[12],
    rsdt[13],
    rsdt[14],
    rsdt[15]
  );

  // Print the OEM table ID (bytes 16 - 23)
  Print(L"RSDT OEM Table ID: %c%c%c%c%c%c%c%c\n",
    rsdt[16],
    rsdt[17],
    rsdt[18],
    rsdt[19],
    rsdt[20],
    rsdt[21],
    rsdt[22],
    rsdt[23]
  );

  // Get the length of the RSDT.
  uint32_t rsdt_len = *((uint32_t*)(rsdt + 4));

  unsigned char rsdt_check = 0;
  for (uint32_t i = 0; i < rsdt_len; i++)
  {
    rsdt_check += rsdt[i];
  }

  Print(L"RSDT checksum: %u\n", rsdt_check);

  // Print the signatures of all the tables in the RSDT.
  for (uint32_t i = 0; i < (rsdt_len - 36) / 8; i++)
  {
    // Use our byte-to-pointer trick to get the pointer to
    // the next SDT.
    unsigned char* sdt = (unsigned char*)(*(uint64_t*)(rsdt + 36 + i * 8));

    Print(L"SDT signature %c%c%c%c\n",
      sdt[0],
      sdt[1],
      sdt[2],
      sdt[3]
    );

    // The signature of the MADT is "APIC".
    // If we found it, store the address for later.
    if (sdt[0] == 'A' && sdt[1] == 'P' && sdt[2] == 'I' && sdt[3] == 'C')
    {
      madt = sdt;
    }
  }


  // If we found the MADT, print its contents.
  if (madt != NULL)
  {
    uint32_t madt_len = *((uint32_t*)(madt + 4));
    uint32_t lapic = *((uint32_t*)(madt + 36));
    uint32_t apic_flags = *((uint32_t*)(madt + 40));

    // Calculate the checksum.
    unsigned char madt_check = 0;
    for (uint32_t i = 0; i < madt_len; i++)
    {
      madt_check += madt[i];
    }

    Print(L"MADT checksum: %u\n", madt_check);
    Print(L"MADT length:   %d\n", madt_len);
    Print(L"Local APIC:    %X\n", lapic);
    Print(L"APIC flags:    %X\n", apic_flags);
    
    for (uint32_t i = 0; i < madt_len - 44;)
    {
      unsigned char* e = (madt + 44 + i);

      uint8_t type = (uint8_t)(*((uint8_t*)(e)));
      uint8_t len = (uint8_t)(*((uint8_t*)(e + 1)));

      switch (type)
      {
      case 0:
        Print(L"Local APIC                 ");
        break;

      case 1:
        Print(L"I/O APIC                   ");
        break;

      case 2:
        Print(L"Interrupt Source Override  ");
        break;

      case 4:
        Print(L"Non-Maskable Interrupts    ");
        break;

      case 5:
        Print(L"Local APIC Address Override");
        break;

      default:
        Print(L"Unknown: %u ", type);
        break;
      }

      Print(L"length: %.3d i: %.3d\n", len, i);

      i += len;
    }
  }
}
