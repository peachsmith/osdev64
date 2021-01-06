#include "osdev64/apic.h"
#include "osdev64/bitmask.h"
#include "osdev64/acpi.h"


#define LAPIC_ID ((uint64_t)0x20)
#define LAPIC_VERSION ((uint64_t)0x30)


// Local APIC Base
extern volatile unsigned char* volatile g_lapic;

// IO APIC base
extern volatile unsigned char* volatile g_ioapic;


uint64_t k_lapic_get_id()
{
  uint64_t id = (uint64_t)(*(uint32_t*)(g_lapic + LAPIC_ID));

  // The ID is located in bits [31:24].
  return (id & (BM_8_BITS << 24)) >> 24;
}

uint64_t k_lapic_get_version()
{
  uint64_t version = (uint64_t)(*(uint32_t*)(g_lapic + LAPIC_VERSION));

  // The version is located in bits [7:0]
  return version & BM_8_BITS;
}

uint64_t k_lapic_get_maxlvt()
{
  uint64_t version = (uint64_t)(*(uint32_t*)(g_lapic + LAPIC_VERSION));

  // The max LVT is located in bits [23:16] of the version register.
  return (version & (BM_8_BITS << 16)) >> 16;
}
