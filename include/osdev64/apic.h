#ifndef JEP_APIC_H
#define JEP_APIC_H

#include <stdint.h>

uint64_t k_lapic_get_id();

uint64_t k_lapic_get_version();

uint64_t k_lapic_get_maxlvt();

#endif