#ifndef JEP_BITMASK_H
#define JEP_BITMASK_H

#include <stdint.h>

// bits [7:0]
#define BM_0 ((uint64_t)0x1)
#define BM_1 ((uint64_t)0x2)
#define BM_2 ((uint64_t)0x4)
#define BM_3 ((uint64_t)0x8)
#define BM_4 ((uint64_t)0x10)
#define BM_5 ((uint64_t)0x20)
#define BM_6 ((uint64_t)0x40)
#define BM_7 ((uint64_t)0x80)

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


// multiple bits
#define BM_2_BITS ((uint64_t)0x3)
#define BM_3_BITS ((uint64_t)0x7)
#define BM_4_BITS ((uint64_t)0xF)

#define BM_5_BITS ((uint64_t)0x1F)
#define BM_6_BITS ((uint64_t)0x3F)
#define BM_7_BITS ((uint64_t)0x7F)
#define BM_8_BITS ((uint64_t)0xFF)

#define BM_9_BITS ((uint64_t)0x1FF)
#define BM_10_BITS ((uint64_t)0x3FF)
#define BM_11_BITS ((uint64_t)0x7FF)
#define BM_12_BITS ((uint64_t)0xFFF)

#define BM_13_BITS ((uint64_t)0x1FFF)
#define BM_14_BITS ((uint64_t)0x3FFF)
#define BM_15_BITS ((uint64_t)0x7FFF)
#define BM_16_BITS ((uint64_t)0xFFFF)

#define BM_17_BITS ((uint64_t)0x1FFFF)
#define BM_18_BITS ((uint64_t)0x3FFFF)
#define BM_19_BITS ((uint64_t)0x7FFFF)
#define BM_20_BITS ((uint64_t)0xFFFFF)

#define BM_21_BITS ((uint64_t)0x1FFFFF)
#define BM_22_BITS ((uint64_t)0x3FFFFF)
#define BM_23_BITS ((uint64_t)0x7FFFFF)
#define BM_24_BITS ((uint64_t)0xFFFFFF)

#define BM_25_BITS ((uint64_t)0x1FFFFFF)
#define BM_26_BITS ((uint64_t)0x3FFFFFF)
#define BM_27_BITS ((uint64_t)0x7FFFFFF)
#define BM_28_BITS ((uint64_t)0xFFFFFFF)

#define BM_29_BITS ((uint64_t)0x1FFFFFFF)
#define BM_30_BITS ((uint64_t)0x3FFFFFFF)
#define BM_31_BITS ((uint64_t)0x7FFFFFFF)
#define BM_32_BITS ((uint64_t)0xFFFFFFFF)

#define BM_33_BITS ((uint64_t)0x1FFFFFFFF)
#define BM_34_BITS ((uint64_t)0x3FFFFFFFF)
#define BM_35_BITS ((uint64_t)0x7FFFFFFFF)
#define BM_36_BITS ((uint64_t)0xFFFFFFFFF)

#define BM_37_BITS ((uint64_t)0x1FFFFFFFFF)
#define BM_38_BITS ((uint64_t)0x3FFFFFFFFF)
#define BM_39_BITS ((uint64_t)0x7FFFFFFFFF)
#define BM_40_BITS ((uint64_t)0xFFFFFFFFFF)

#endif