#ifndef JEP_UTIL_H
#define JEP_UTIL_H

#include <stdint.h>

// bits [7:0]
#define BM_0 0x1
#define BM_1 0x2
#define BM_2 0x4
#define BM_3 0x8
#define BM_4 0x10
#define BM_5 0x20
#define BM_6 0x40
#define BM_7 0x80

// bits [15:8]
#define BM_8  ((uint64_t)0x100)
#define BM_9  ((uint64_t)0x200)
#define BM_10 ((uint64_t)0x400)
#define BM_11 ((uint64_t)0x800)
#define BM_12 ((uint64_t)0x1000)
#define BM_13 ((uint64_t)0x2000)
#define BM_14 ((uint64_t)0x4000)
#define BM_15 ((uint64_t)0x8000)

// bits [23:16]
#define BM_16 ((uint64_t)0x10000)
#define BM_17 ((uint64_t)0x20000)
#define BM_18 ((uint64_t)0x40000)
#define BM_19 ((uint64_t)0x80000)
#define BM_20 ((uint64_t)0x100000)
#define BM_21 ((uint64_t)0x200000)
#define BM_22 ((uint64_t)0x400000)
#define BM_23 ((uint64_t)0x800000)

// bits [31:24]
#define BM_24 ((uint64_t)0x1000000)
#define BM_25 ((uint64_t)0x2000000)
#define BM_26 ((uint64_t)0x4000000)
#define BM_27 ((uint64_t)0x8000000)
#define BM_28 ((uint64_t)0x10000000)
#define BM_29 ((uint64_t)0x20000000)
#define BM_30 ((uint64_t)0x40000000)
#define BM_31 ((uint64_t)0x80000000)

// bits [39:32]
#define BM_32 ((uint64_t)0x100000000)
#define BM_33 ((uint64_t)0x200000000)
#define BM_34 ((uint64_t)0x400000000)
#define BM_35 ((uint64_t)0x800000000)
#define BM_36 ((uint64_t)0x1000000000)
#define BM_37 ((uint64_t)0x2000000000)
#define BM_38 ((uint64_t)0x4000000000)
#define BM_39 ((uint64_t)0x8000000000)

// bits [47:40]
#define BM_40 ((uint64_t)0x10000000000)
#define BM_41 ((uint64_t)0x20000000000)
#define BM_42 ((uint64_t)0x40000000000)
#define BM_43 ((uint64_t)0x80000000000)
#define BM_44 ((uint64_t)0x100000000000)
#define BM_45 ((uint64_t)0x200000000000)
#define BM_46 ((uint64_t)0x400000000000)
#define BM_47 ((uint64_t)0x800000000000)

// bits [55:48]
#define BM_48 ((uint64_t)0x1000000000000)
#define BM_49 ((uint64_t)0x2000000000000)
#define BM_50 ((uint64_t)0x4000000000000)
#define BM_51 ((uint64_t)0x8000000000000)
#define BM_52 ((uint64_t)0x10000000000000)
#define BM_53 ((uint64_t)0x20000000000000)
#define BM_54 ((uint64_t)0x40000000000000)
#define BM_55 ((uint64_t)0x80000000000000)

// bits [63:56]
#define BM_56 ((uint64_t)0x100000000000000)
#define BM_57 ((uint64_t)0x200000000000000)
#define BM_58 ((uint64_t)0x400000000000000)
#define BM_59 ((uint64_t)0x800000000000000)
#define BM_60 ((uint64_t)0x1000000000000000)
#define BM_61 ((uint64_t)0x2000000000000000)
#define BM_62 ((uint64_t)0x4000000000000000)
#define BM_63 ((uint64_t)0x8000000000000000)

#endif