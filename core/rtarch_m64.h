/******************************************************************************/
/* Copyright (c) 2013-2016 VectorChief (at github, bitbucket, sourceforge)    */
/* Distributed under the MIT software license, see the accompanying           */
/* file COPYING or http://www.opensource.org/licenses/mit-license.php         */
/******************************************************************************/

#ifndef RT_RTARCH_M64_H
#define RT_RTARCH_M64_H

#define RT_BASE_REGS        16

/******************************************************************************/
/*********************************   LEGEND   *********************************/
/******************************************************************************/

/*
 * rtarch_m64.h: Implementation of MIPS64 r5/r6 BASE instructions.
 *
 * This file is a part of the unified SIMD assembler framework (rtarch.h)
 * designed to be compatible with different processor architectures,
 * while maintaining strictly defined common API.
 *
 * Recommended naming scheme for instructions:
 *
 * cmdxx_ri - applies [cmd] to [r]egister from [i]mmediate
 * cmdxx_mi - applies [cmd] to [m]emory   from [i]mmediate
 * cmdxx_rz - applies [cmd] to [r]egister from [z]ero-arg
 * cmdxx_mz - applies [cmd] to [m]emory   from [z]ero-arg
 *
 * cmdxx_rm - applies [cmd] to [r]egister from [m]emory
 * cmdxx_ld - applies [cmd] as above
 * cmdxx_mr - applies [cmd] to [m]emory   from [r]egister
 * cmdxx_st - applies [cmd] as above (arg list as cmdxx_ld)
 *
 * cmdxx_rr - applies [cmd] to [r]egister from [r]egister
 * cmdxx_mm - applies [cmd] to [m]emory   from [m]emory
 * cmdxx_rx - applies [cmd] to [r]egister (one-operand cmd)
 * cmdxx_mx - applies [cmd] to [m]emory   (one-operand cmd)
 *
 * cmdxx_rx - applies [cmd] to [r]egister from x-register
 * cmdxx_mx - applies [cmd] to [m]emory   from x-register
 * cmdxx_xr - applies [cmd] to x-register from [r]egister
 * cmdxx_xm - applies [cmd] to x-register from [m]emory
 *
 * cmdxx_rl - applies [cmd] to [r]egister from [l]abel
 * cmdxx_xl - applies [cmd] to x-register from [l]abel
 * cmdxx_lb - applies [cmd] as above
 * label_ld - applies [adr] as above
 *
 * stack_st - applies [mov] to stack from register (push)
 * stack_ld - applies [mov] to register from stack (pop)
 * stack_sa - applies [mov] to stack from all registers
 * stack_la - applies [mov] to all registers from stack
 *
 * cmdw*_** - applies [cmd] to 32-bit BASE register/memory/immediate args
 * cmdx*_** - applies [cmd] to A-size BASE register/memory/immediate args
 * cmdy*_** - applies [cmd] to L-size BASE register/memory/immediate args
 * cmdz*_** - applies [cmd] to 64-bit BASE register/memory/immediate args
 *
 * cmd*x_** - applies [cmd] to unsigned integer args, [x] - default
 * cmd*n_** - applies [cmd] to   signed integer args, [n] - negatable
 * cmd*p_** - applies [cmd] to   signed integer args, [p] - part-range
 *
 * cmd*z_** - applies [cmd] while setting condition flags, [z] - zero flag.
 * Regular cmd*x_**, cmd*n_** instructions may or may not set flags depending
 * on the target architecture, thus no assumptions can be made for jezxx/jnzxx.
 *
 * Interpretation of instruction parameters:
 *
 * upper-case params have triplet structure and require W to pass-forward
 * lower-case params are singular and can be used/passed as such directly
 *
 * RD - BASE register serving as destination only, if present
 * RG - BASE register serving as destination and fisrt source
 * RS - BASE register serving as second source (first if any)
 * RT - BASE register serving as third source (second if any)
 *
 * MD - BASE addressing mode (Oeax, M***, I***) (memory-dest)
 * MG - BASE addressing mode (Oeax, M***, I***) (memory-dsrc)
 * MS - BASE addressing mode (Oeax, M***, I***) (memory-src2)
 * MT - BASE addressing mode (Oeax, M***, I***) (memory-src3)
 *
 * DP - displacement value (of given size DP, DF, DG, DH, DV)
 * IS - immediate value (is used as a second or first source)
 * IT - immediate value (is used as a third or second source)
 *
 * Mixing of 64/32-bit fields in backend structures may lead to misalignment
 * of 64-bit fields to 4-byte boundary, which is not supported on some targets.
 * Place fields carefully to ensure proper alignment for all data types.
 * Note that within cmdx*_** subset most of the instructions follow in-heap
 * address size (RT_ADDRESS or A) and only label_ld/st, jmpxx_xr/xm follow
 * pointer size (RT_POINTER or P) as code/data/stack segments are fixed.
 * In 64/32-bit (ptr/adr) hybrid mode there is no way to move 64-bit registers,
 * thus label_ld has very limited use as jmpxx_xr(Reax) is the only matching op.
 * Stack ops always work with full registers regardless of the mode chosen.
 *
 * 32-bit and 64-bit BASE subsets are not easily compatible on all targets,
 * thus any register modified with 32-bit op cannot be used in 64-bit subset.
 * Alternatively, data flow must not exceed 31-bit range for 32-bit operations
 * to produce consistent results usable in 64-bit subset across all targets.
 * Only a64 and x64 have a complete 32-bit support in 64-bit mode both zeroing
 * the upper half of the result, while m64 sign-extending all 32-bit operations
 * and p64 overflowing 32-bit arithmetic into the upper half. Similar reasons
 * of inconsistency prohibit use of IW immediate type within 64-bit subset,
 * where a64 and p64 zero-extend, while x64 and m64 sign-extend 32-bit value.
 *
 * Note that offset correction for endianness E is only applicable for addresses
 * within pointer fields, when (in-heap) address and pointer sizes don't match.
 * Working with 32-bit data in 64-bit fields in any other circumstances must be
 * done consistently within a subset of one size (32-bit, 64-bit or C/C++).
 * Alternatively, data written natively in C/C++ can be worked on from within
 * a given (one) subset if appropriate offset correction is used from rtarch.h.
 *
 * Setting-flags instructions' naming scheme may change again in the future for
 * better orthogonality with operands size, type and args-list. It is therefore
 * recommended to use combined-arithmetic-jump (arj) for better API stability
 * and maximum efficiency across all supported targets. For similar reasons
 * of higher performance on certain targets use combined-compare-jump (cmj).
 * Not all canonical forms of BASE instructions have efficient implementation.
 * For example, some forms of shifts and division use stack ops on x86 targets,
 * while standalone remainder operations can only be done natively on MIPS.
 * Consider using special fixed-register forms for maximum performance.
 * Argument x-register (implied) is fixed by the implementation.
 * Some formal definitions are not given below to encourage
 * use of friendly aliases for better code readability.
 */

/******************************************************************************/
/********************************   INTERNAL   ********************************/
/******************************************************************************/

/******************************************************************************/
/********************************   EXTERNAL   ********************************/
/******************************************************************************/

#include "rtarch_m32.h"

/******************************************************************************/
/**********************************   M64   ***********************************/
/******************************************************************************/

/* mov
 * set-flags: no */

#define movzx_ri(RD, IS)                                                    \
        AUW(EMPTY,    VAL(IS), REG(RD), EMPTY,   EMPTY,   EMPTY2, G3(IS))

#define movzx_mi(MD, DP, IS)                                                \
        AUW(SIB(MD),  VAL(IS), TDxx,    MOD(MD), VAL(DP), C1(DP), G3(IS))   \
        EMITW(0xFC000000 | MDM(TDxx,    MOD(MD), VAL(DP), B1(DP), P1(DP)))

#define movzx_rr(RD, RS)                                                    \
        EMITW(0x00000025 | MRM(REG(RD), REG(RS), TZxx))

#define movzx_ld(RD, MS, DP)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(REG(RD), MOD(MS), VAL(DP), B1(DP), P1(DP)))

#define movzx_st(RS, MD, DP)                                                \
        AUW(SIB(MD),  EMPTY,  EMPTY,    MOD(MD), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xFC000000 | MDM(REG(RS), MOD(MD), VAL(DP), B1(DP), P1(DP)))

/* and
 * set-flags: undefined (*x), yes (*z) */

#define andzx_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G2(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x30000000) | (+(TP2(IS) != 0) & 0x00000024))    \
        /* if true ^ equals to -1 (not 1) */

#define andzx_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DP), C1(DP), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x30000000) | (+(TP2(IS) != 0) & 0x00000024))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define andzx_rr(RG, RS)                                                    \
        EMITW(0x00000024 | MRM(REG(RG), REG(RG), REG(RS)))

#define andzx_ld(RG, MS, DP)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000024 | MRM(REG(RG), REG(RG), TMxx))

#define andzx_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000024 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))


#define andzz_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G2(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x30000000) | (+(TP2(IS) != 0) & 0x00000024))    \
        /* if true ^ equals to -1 (not 1) */                                \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define andzz_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DP), C1(DP), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x30000000) | (+(TP2(IS) != 0) & 0x00000024))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define andzz_rr(RG, RS)                                                    \
        EMITW(0x00000024 | MRM(REG(RG), REG(RG), REG(RS)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define andzz_ld(RG, MS, DP)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000024 | MRM(REG(RG), REG(RG), TMxx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define andzz_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000024 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

/* ann (G = ~G & S)
 * set-flags: undefined (*x), yes (*z) */

#define annzx_ri(RG, IS)                                                    \
        notzx_rx(W(RG))                                                     \
        andzx_ri(W(RG), W(IS))

#define annzx_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DP), C1(DP), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000027 | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x30000000) | (+(TP2(IS) != 0) & 0x00000024))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define annzx_rr(RG, RS)                                                    \
        notzx_rx(W(RG))                                                     \
        andzx_rr(W(RG), W(RS))

#define annzx_ld(RG, MS, DP)                                                \
        notzx_rx(W(RG))                                                     \
        andzx_ld(W(RG), W(MS), W(DP))

#define annzx_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000027 | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0x00000024 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define annzx_mr(MG, DP, RS)                                                \
        annzx_st(W(RS), W(MG), W(DP))


#define annzz_ri(RG, IS)                                                    \
        notzx_rx(W(RG))                                                     \
        andzz_ri(W(RG), W(IS))

#define annzz_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DP), C1(DP), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000027 | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x30000000) | (+(TP2(IS) != 0) & 0x00000024))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define annzz_rr(RG, RS)                                                    \
        notzx_rx(W(RG))                                                     \
        andzz_rr(W(RG), W(RS))

#define annzz_ld(RG, MS, DP)                                                \
        notzx_rx(W(RG))                                                     \
        andzz_ld(W(RG), W(MS), W(DP))

#define annzz_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000027 | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0x00000024 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define annzz_mr(MG, DP, RS)                                                \
        annzz_st(W(RS), W(MG), W(DP))

/* orr
 * set-flags: undefined (*x), yes (*z) */

#define orrzx_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G2(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x34000000) | (+(TP2(IS) != 0) & 0x00000025))    \
        /* if true ^ equals to -1 (not 1) */

#define orrzx_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DP), C1(DP), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x34000000) | (+(TP2(IS) != 0) & 0x00000025))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define orrzx_rr(RG, RS)                                                    \
        EMITW(0x00000025 | MRM(REG(RG), REG(RG), REG(RS)))

#define orrzx_ld(RG, MS, DP)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000025 | MRM(REG(RG), REG(RG), TMxx))

#define orrzx_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000025 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))


#define orrzz_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G2(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x34000000) | (+(TP2(IS) != 0) & 0x00000025))    \
        /* if true ^ equals to -1 (not 1) */                                \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define orrzz_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DP), C1(DP), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x34000000) | (+(TP2(IS) != 0) & 0x00000025))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define orrzz_rr(RG, RS)                                                    \
        EMITW(0x00000025 | MRM(REG(RG), REG(RG), REG(RS)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define orrzz_ld(RG, MS, DP)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000025 | MRM(REG(RG), REG(RG), TMxx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define orrzz_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000025 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

/* orn (G = ~G | S)
 * set-flags: undefined (*x), yes (*z) */

#define ornzx_ri(RG, IS)                                                    \
        notzx_rx(W(RG))                                                     \
        orrzx_ri(W(RG), W(IS))

#define ornzx_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DP), C1(DP), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000027 | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x34000000) | (+(TP2(IS) != 0) & 0x00000025))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define ornzx_rr(RG, RS)                                                    \
        notzx_rx(W(RG))                                                     \
        orrzx_rr(W(RG), W(RS))

#define ornzx_ld(RG, MS, DP)                                                \
        notzx_rx(W(RG))                                                     \
        orrzx_ld(W(RG), W(MS), W(DP))

#define ornzx_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000027 | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0x00000025 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define ornzx_mr(MG, DP, RS)                                                \
        ornzx_st(W(RS), W(MG), W(DP))


#define ornzz_ri(RG, IS)                                                    \
        notzx_rx(W(RG))                                                     \
        orrzz_ri(W(RG), W(IS))

#define ornzz_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DP), C1(DP), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000027 | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x34000000) | (+(TP2(IS) != 0) & 0x00000025))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define ornzz_rr(RG, RS)                                                    \
        notzx_rx(W(RG))                                                     \
        orrzz_rr(W(RG), W(RS))

#define ornzz_ld(RG, MS, DP)                                                \
        notzx_rx(W(RG))                                                     \
        orrzz_ld(W(RG), W(MS), W(DP))

#define ornzz_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000027 | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0x00000025 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define ornzz_mr(MG, DP, RS)                                                \
        ornzz_st(W(RS), W(MG), W(DP))

/* xor
 * set-flags: undefined (*x), yes (*z) */

#define xorzx_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G2(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x38000000) | (+(TP2(IS) != 0) & 0x00000026))    \
        /* if true ^ equals to -1 (not 1) */

#define xorzx_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DP), C1(DP), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x38000000) | (+(TP2(IS) != 0) & 0x00000026))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define xorzx_rr(RG, RS)                                                    \
        EMITW(0x00000026 | MRM(REG(RG), REG(RG), REG(RS)))

#define xorzx_ld(RG, MS, DP)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000026 | MRM(REG(RG), REG(RG), TMxx))

#define xorzx_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000026 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))


#define xorzz_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G2(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x38000000) | (+(TP2(IS) != 0) & 0x00000026))    \
        /* if true ^ equals to -1 (not 1) */                                \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define xorzz_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DP), C1(DP), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x38000000) | (+(TP2(IS) != 0) & 0x00000026))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define xorzz_rr(RG, RS)                                                    \
        EMITW(0x00000026 | MRM(REG(RG), REG(RG), REG(RS)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define xorzz_ld(RG, MS, DP)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000026 | MRM(REG(RG), REG(RG), TMxx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define xorzz_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000026 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

/* not
 * set-flags: no */

#define notzx_rx(RG)                                                        \
        EMITW(0x00000027 | MRM(REG(RG), TZxx,    REG(RG)))

#define notzx_mx(MG, DP)                                                    \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TDxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000027 | MRM(TDxx,    TZxx,    TDxx))                     \
        EMITW(0xFC000000 | MDM(TDxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

/* neg
 * set-flags: undefined (*x), yes (*z) */

#define negzx_rx(RG)                                                        \
        EMITW(0x0000002F | MRM(REG(RG), TZxx,    REG(RG)))

#define negzx_mx(MG, DP)                                                    \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000002F | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))


#define negzz_rx(RG)                                                        \
        EMITW(0x0000002F | MRM(REG(RG), TZxx,    REG(RG)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define negzz_mx(MG, DP)                                                    \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000002F | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

/* add
 * set-flags: undefined (*x), yes (*z) */

#define addzx_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G1(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), VAL(IS), T1(IS), M1(IS)) | \
        (+(TP1(IS) == 0) & 0x64000000) | (+(TP1(IS) != 0) & 0x0000002D))    \
        /* if true ^ equals to -1 (not 1) */

#define addzx_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DP), C1(DP), G1(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T1(IS), M1(IS)) | \
        (+(TP1(IS) == 0) & 0x64000000) | (+(TP1(IS) != 0) & 0x0000002D))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define addzx_rr(RG, RS)                                                    \
        EMITW(0x0000002D | MRM(REG(RG), REG(RG), REG(RS)))

#define addzx_ld(RG, MS, DP)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000002D | MRM(REG(RG), REG(RG), TMxx))

#define addzx_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000002D | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))


#define addzz_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G1(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), VAL(IS), T1(IS), M1(IS)) | \
        (+(TP1(IS) == 0) & 0x64000000) | (+(TP1(IS) != 0) & 0x0000002D))    \
        /* if true ^ equals to -1 (not 1) */                                \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define addzz_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DP), C1(DP), G1(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T1(IS), M1(IS)) | \
        (+(TP1(IS) == 0) & 0x64000000) | (+(TP1(IS) != 0) & 0x0000002D))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define addzz_rr(RG, RS)                                                    \
        EMITW(0x0000002D | MRM(REG(RG), REG(RG), REG(RS)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define addzz_ld(RG, MS, DP)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000002D | MRM(REG(RG), REG(RG), TMxx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define addzz_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000002D | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

/* sub
 * set-flags: undefined (*x), yes (*z) */

#define subzx_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G1(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), 0x00,    T1(IS), EMPTY1) | \
        (+(TP1(IS) == 0) & (0x64000000 | (0xFFFF & -VAL(IS)))) |            \
        (+(TP1(IS) != 0) & (0x0000002F | TIxx << 16)))                      \
        /* if true ^ equals to -1 (not 1) */

#define subzx_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DP), C1(DP), G1(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    0x00,    T1(IS), EMPTY1) | \
        (+(TP1(IS) == 0) & (0x64000000 | (0xFFFF & -VAL(IS)))) |            \
        (+(TP1(IS) != 0) & (0x0000002F | TIxx << 16)))                      \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define subzx_rr(RG, RS)                                                    \
        EMITW(0x0000002F | MRM(REG(RG), REG(RG), REG(RS)))

#define subzx_ld(RG, MS, DP)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000002F | MRM(REG(RG), REG(RG), TMxx))

#define subzx_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000002F | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define subzx_mr(MG, DP, RS)                                                \
        subzx_st(W(RS), W(MG), W(DP))


#define subzz_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G1(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), 0x00,    T1(IS), EMPTY1) | \
        (+(TP1(IS) == 0) & (0x64000000 | (0xFFFF & -VAL(IS)))) |            \
        (+(TP1(IS) != 0) & (0x0000002F | TIxx << 16)))                      \
        /* if true ^ equals to -1 (not 1) */                                \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define subzz_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DP), C1(DP), G1(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    0x00,    T1(IS), EMPTY1) | \
        (+(TP1(IS) == 0) & (0x64000000 | (0xFFFF & -VAL(IS)))) |            \
        (+(TP1(IS) != 0) & (0x0000002F | TIxx << 16)))                      \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define subzz_rr(RG, RS)                                                    \
        EMITW(0x0000002F | MRM(REG(RG), REG(RG), REG(RS)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define subzz_ld(RG, MS, DP)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000002F | MRM(REG(RG), REG(RG), TMxx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define subzz_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000002F | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define subzz_mr(MG, DP, RS)                                                \
        subzz_st(W(RS), W(MG), W(DP))

/* shl
 * set-flags: undefined (*x), yes (*z) */

#define shlzx_rx(RG)                     /* reads Recx for shift value */   \
        EMITW(0x00000014 | MRM(REG(RG), Tecx,    REG(RG)))

#define shlzx_mx(MG, DP)                 /* reads Recx for shift value */   \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000014 | MRM(TMxx,    Tecx,    TMxx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define shlzx_ri(RG, IS)                                                    \
        EMITW(0x00000000 | MRM(REG(RG), 0x00,    REG(RG)) |                 \
        (+(VAL(IS) < 32) & (0x00000038 | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003C | (0x1F & VAL(IS)) << 6)))           \
        /* if true ^ equals to -1 (not 1) */

#define shlzx_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000000 | MRM(TMxx,    0x00,    TMxx) |                    \
        (+(VAL(IS) < 32) & (0x00000038 | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003C | (0x1F & VAL(IS)) << 6)))           \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define shlzx_rr(RG, RS)       /* Recx cannot be used as first operand */   \
        EMITW(0x00000014 | MRM(REG(RG), REG(RS), REG(RG)))

#define shlzx_ld(RG, MS, DP)   /* Recx cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000014 | MRM(REG(RG), TMxx,    REG(RG)))

#define shlzx_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000014 | MRM(TMxx,    REG(RS), TMxx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define shlzx_mr(MG, DP, RS)                                                \
        shlzx_st(W(RS), W(MG), W(DP))


#define shlzz_rx(RG)                     /* reads Recx for shift value */   \
        EMITW(0x00000014 | MRM(REG(RG), Tecx,    REG(RG)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define shlzz_mx(MG, DP)                 /* reads Recx for shift value */   \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000014 | MRM(TMxx,    Tecx,    TMxx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define shlzz_ri(RG, IS)                                                    \
        EMITW(0x00000000 | MRM(REG(RG), 0x00,    REG(RG)) |                 \
        (+(VAL(IS) < 32) & (0x00000038 | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003C | (0x1F & VAL(IS)) << 6)))           \
        /* if true ^ equals to -1 (not 1) */                                \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define shlzz_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000000 | MRM(TMxx,    0x00,    TMxx) |                    \
        (+(VAL(IS) < 32) & (0x00000038 | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003C | (0x1F & VAL(IS)) << 6)))           \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define shlzz_rr(RG, RS)       /* Recx cannot be used as first operand */   \
        EMITW(0x00000014 | MRM(REG(RG), REG(RS), REG(RG)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define shlzz_ld(RG, MS, DP)   /* Recx cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000014 | MRM(REG(RG), TMxx,    REG(RG)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define shlzz_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000014 | MRM(TMxx,    REG(RS), TMxx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define shlzz_mr(MG, DP, RS)                                                \
        shlzz_st(W(RS), W(MG), W(DP))

/* shr
 * set-flags: undefined (*x), yes (*z) */

#define shrzx_rx(RG)                     /* reads Recx for shift value */   \
        EMITW(0x00000016 | MRM(REG(RG), Tecx,    REG(RG)))

#define shrzx_mx(MG, DP)                 /* reads Recx for shift value */   \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000016 | MRM(TMxx,    Tecx,    TMxx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define shrzx_ri(RG, IS)                                                    \
        EMITW(0x00000000 | MRM(REG(RG), 0x00,    REG(RG)) |                 \
        (+(VAL(IS) < 32) & (0x0000003A | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003E | (0x1F & VAL(IS)) << 6)))           \
        /* if true ^ equals to -1 (not 1) */

#define shrzx_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000000 | MRM(TMxx,    0x00,    TMxx) |                    \
        (+(VAL(IS) < 32) & (0x0000003A | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003E | (0x1F & VAL(IS)) << 6)))           \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define shrzx_rr(RG, RS)       /* Recx cannot be used as first operand */   \
        EMITW(0x00000016 | MRM(REG(RG), REG(RS), REG(RG)))

#define shrzx_ld(RG, MS, DP)   /* Recx cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000016 | MRM(REG(RG), TMxx,    REG(RG)))

#define shrzx_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000016 | MRM(TMxx,    REG(RS), TMxx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define shrzx_mr(MG, DP, RS)                                                \
        shrzx_st(W(RS), W(MG), W(DP))


#define shrzz_rx(RG)                     /* reads Recx for shift value */   \
        EMITW(0x00000016 | MRM(REG(RG), Tecx,    REG(RG)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define shrzz_mx(MG, DP)                 /* reads Recx for shift value */   \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000016 | MRM(TMxx,    Tecx,    TMxx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define shrzz_ri(RG, IS)                                                    \
        EMITW(0x00000000 | MRM(REG(RG), 0x00,    REG(RG)) |                 \
        (+(VAL(IS) < 32) & (0x0000003A | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003E | (0x1F & VAL(IS)) << 6)))           \
        /* if true ^ equals to -1 (not 1) */                                \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define shrzz_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000000 | MRM(TMxx,    0x00,    TMxx) |                    \
        (+(VAL(IS) < 32) & (0x0000003A | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003E | (0x1F & VAL(IS)) << 6)))           \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define shrzz_rr(RG, RS)       /* Recx cannot be used as first operand */   \
        EMITW(0x00000016 | MRM(REG(RG), REG(RS), REG(RG)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define shrzz_ld(RG, MS, DP)   /* Recx cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000016 | MRM(REG(RG), TMxx,    REG(RG)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define shrzz_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000016 | MRM(TMxx,    REG(RS), TMxx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define shrzz_mr(MG, DP, RS)                                                \
        shrzz_st(W(RS), W(MG), W(DP))


#define shrzn_rx(RG)                     /* reads Recx for shift value */   \
        EMITW(0x00000017 | MRM(REG(RG), Tecx,    REG(RG)))

#define shrzn_mx(MG, DP)                 /* reads Recx for shift value */   \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000017 | MRM(TMxx,    Tecx,    TMxx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define shrzn_ri(RG, IS)                                                    \
        EMITW(0x00000000 | MRM(REG(RG), 0x00,    REG(RG)) |                 \
        (+(VAL(IS) < 32) & (0x0000003B | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003F | (0x1F & VAL(IS)) << 6)))           \
        /* if true ^ equals to -1 (not 1) */

#define shrzn_mi(MG, DP, IS)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000000 | MRM(TMxx,    0x00,    TMxx) |                    \
        (+(VAL(IS) < 32) & (0x0000003B | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003F | (0x1F & VAL(IS)) << 6)))           \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define shrzn_rr(RG, RS)       /* Recx cannot be used as first operand */   \
        EMITW(0x00000017 | MRM(REG(RG), REG(RS), REG(RG)))

#define shrzn_ld(RG, MS, DP)   /* Recx cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000017 | MRM(REG(RG), TMxx,    REG(RG)))

#define shrzn_st(RS, MG, DP)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000017 | MRM(TMxx,    REG(RS), TMxx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DP), B1(DP), P1(DP)))

#define shrzn_mr(MG, DP, RS)                                                \
        shrzn_st(W(RS), W(MG), W(DP))

/* pre-r6 */
#if defined (RT_M64) && RT_M64 < 6

/* mul
 * set-flags: undefined */

#define mulzx_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x0000001D | MRM(0x00,    REG(RG), TIxx))                     \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))

#define mulzx_rr(RG, RS)                                                    \
        EMITW(0x0000001D | MRM(0x00,    REG(RG), REG(RS)))                  \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))

#define mulzx_ld(RG, MS, DP)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000001D | MRM(0x00,    REG(RG), TMxx))                     \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))


#define mulzx_xr(RS)     /* Reax is in/out, Redx is out(high)-zero-ext */   \
        EMITW(0x0000001D | MRM(0x00,    Teax,    REG(RS)))                  \
        EMITW(0x00000012 | MRM(Teax,    0x00,    0x00))                     \
        EMITW(0x00000010 | MRM(Tedx,    0x00,    0x00))

#define mulzx_xm(MS, DP) /* Reax is in/out, Redx is out(high)-zero-ext */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000001D | MRM(0x00,    Teax,    TMxx))                     \
        EMITW(0x00000012 | MRM(Teax,    0x00,    0x00))                     \
        EMITW(0x00000010 | MRM(Tedx,    0x00,    0x00))


#define mulzn_xr(RS)     /* Reax is in/out, Redx is out(high)-sign-ext */   \
        EMITW(0x0000001C | MRM(0x00,    Teax,    REG(RS)))                  \
        EMITW(0x00000012 | MRM(Teax,    0x00,    0x00))                     \
        EMITW(0x00000010 | MRM(Tedx,    0x00,    0x00))

#define mulzn_xm(MS, DP) /* Reax is in/out, Redx is out(high)-sign-ext */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000001C | MRM(0x00,    Teax,    TMxx))                     \
        EMITW(0x00000012 | MRM(Teax,    0x00,    0x00))                     \
        EMITW(0x00000010 | MRM(Tedx,    0x00,    0x00))


#define mulzp_xr(RS)     /* Reax is in/out, prepares Redx for divzn_x* */   \
        mulzx_rr(Reax, W(RS)) /* product must not exceed operands size */

#define mulzp_xm(MS, DP) /* Reax is in/out, prepares Redx for divzn_x* */   \
        mulzx_ld(Reax, W(MS), W(DP))  /* must not exceed operands size */

/* div
 * set-flags: undefined */

#define divzx_ri(RG, IS)       /* Reax cannot be used as first operand */   \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x0000001F | MRM(0x00,    REG(RG), TIxx))                     \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))

#define divzx_rr(RG, RS)                 /* RG, RS no Reax, RS no Redx */   \
        EMITW(0x0000001F | MRM(0x00,    REG(RG), REG(RS)))                  \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))

#define divzx_ld(RG, MS, DP)   /* Reax cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000001F | MRM(0x00,    REG(RG), TMxx))                     \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))


#define divzn_ri(RG, IS)       /* Reax cannot be used as first operand */   \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x0000001E | MRM(0x00,    REG(RG), TIxx))                     \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))

#define divzn_rr(RG, RS)                 /* RG, RS no Reax, RS no Redx */   \
        EMITW(0x0000001E | MRM(0x00,    REG(RG), REG(RS)))                  \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))

#define divzn_ld(RG, MS, DP)   /* Reax cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000001E | MRM(0x00,    REG(RG), TMxx))                     \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))


#define prezx_xx()          /* to be placed immediately prior divzx_x* */   \
                                     /* to prepare Redx for int-divide */

#define prezn_xx()          /* to be placed immediately prior divzn_x* */   \
                                     /* to prepare Redx for int-divide */


#define divzx_xr(RS)     /* Reax is in/out, Redx is in(zero)/out(junk) */   \
        EMITW(0x0000001F | MRM(0x00,    Teax,    REG(RS)))                  \
        EMITW(0x00000012 | MRM(Teax,    0x00,    0x00))

#define divzx_xm(MS, DP) /* Reax is in/out, Redx is in(zero)/out(junk) */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000001F | MRM(0x00,    Teax,    TMxx))                     \
        EMITW(0x00000012 | MRM(Teax,    0x00,    0x00))


#define divzn_xr(RS)     /* Reax is in/out, Redx is in-sign-ext-(Reax) */   \
        EMITW(0x0000001E | MRM(0x00,    Teax,    REG(RS)))                  \
        EMITW(0x00000012 | MRM(Teax,    0x00,    0x00))

#define divzn_xm(MS, DP) /* Reax is in/out, Redx is in-sign-ext-(Reax) */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000001E | MRM(0x00,    Teax,    TMxx))                     \
        EMITW(0x00000012 | MRM(Teax,    0x00,    0x00))


#define divzp_xr(RS)     /* Reax is in/out, Redx is in-sign-ext-(Reax) */   \
        divzn_xr(W(RS))              /* destroys Redx, Xmm0 (in ARMv7) */   \
                                     /* 24-bit int (fp32 div in ARMv7) */

#define divzp_xm(MS, DP) /* Reax is in/out, Redx is in-sign-ext-(Reax) */   \
        divzn_xm(W(MS), W(DP))       /* destroys Redx, Xmm0 (in ARMv7) */   \
                                     /* 24-bit int (fp32 div in ARMv7) */

/* rem
 * set-flags: undefined */

#define remzx_ri(RG, IS)       /* Redx cannot be used as first operand */   \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x0000001F | MRM(0x00,    REG(RG), TIxx))                     \
        EMITW(0x00000010 | MRM(REG(RG), 0x00,    0x00))

#define remzx_rr(RG, RS)                 /* RG, RS no Redx, RS no Reax */   \
        EMITW(0x0000001F | MRM(0x00,    REG(RG), REG(RS)))                  \
        EMITW(0x00000010 | MRM(REG(RG), 0x00,    0x00))

#define remzx_ld(RG, MS, DP)   /* Redx cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000001F | MRM(0x00,    REG(RG), TMxx))                     \
        EMITW(0x00000010 | MRM(REG(RG), 0x00,    0x00))


#define remzn_ri(RG, IS)       /* Redx cannot be used as first operand */   \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x0000001E | MRM(0x00,    REG(RG), TIxx))                     \
        EMITW(0x00000010 | MRM(REG(RG), 0x00,    0x00))

#define remzn_rr(RG, RS)                 /* RG, RS no Redx, RS no Reax */   \
        EMITW(0x0000001E | MRM(0x00,    REG(RG), REG(RS)))                  \
        EMITW(0x00000010 | MRM(REG(RG), 0x00,    0x00))

#define remzn_ld(RG, MS, DP)   /* Redx cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000001E | MRM(0x00,    REG(RG), TMxx))                     \
        EMITW(0x00000010 | MRM(REG(RG), 0x00,    0x00))


#define remzx_xx()          /* to be placed immediately prior divzx_x* */   \

#define remzx_xr(RS)        /* to be placed immediately after divzx_xr */   \
        EMITW(0x00000010 | MRM(Tedx,    0x00,    0x00))   /* Redx<-rem */

#define remzx_xm(MS, DP)    /* to be placed immediately after divzx_xm */   \
        EMITW(0x00000010 | MRM(Tedx,    0x00,    0x00))   /* Redx<-rem */


#define remzn_xx()          /* to be placed immediately prior divzn_x* */   \
                                     /* to prepare for rem calculation */

#define remzn_xr(RS)        /* to be placed immediately after divzn_xr */   \
        EMITW(0x00000010 | MRM(Tedx,    0x00,    0x00))   /* Redx<-rem */

#define remzn_xm(MS, DP)    /* to be placed immediately after divzn_xm */   \
        EMITW(0x00000010 | MRM(Tedx,    0x00,    0x00))   /* Redx<-rem */

#else  /* r6 */

/* mul
 * set-flags: undefined */

#define mulzx_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x0000009D | MRM(REG(RG), REG(RG), TIxx))

#define mulzx_rr(RG, RS)                                                    \
        EMITW(0x0000009D | MRM(REG(RG), REG(RG), REG(RS)))

#define mulzx_ld(RG, MS, DP)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000009D | MRM(REG(RG), REG(RG), TMxx))


#define mulzx_xr(RS)     /* Reax is in/out, Redx is out(high)-zero-ext */   \
        EMITW(0x000000DD | MRM(Tedx,    Teax,    REG(RS)))                  \
        EMITW(0x0000009D | MRM(Teax,    Teax,    REG(RS)))

#define mulzx_xm(MS, DP) /* Reax is in/out, Redx is out(high)-zero-ext */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x000000DD | MRM(Tedx,    Teax,    TMxx))                     \
        EMITW(0x0000009D | MRM(Teax,    Teax,    TMxx))


#define mulzn_xr(RS)     /* Reax is in/out, Redx is out(high)-sign-ext */   \
        EMITW(0x000000DC | MRM(Tedx,    Teax,    REG(RS)))                  \
        EMITW(0x0000009C | MRM(Teax,    Teax,    REG(RS)))

#define mulzn_xm(MS, DP) /* Reax is in/out, Redx is out(high)-sign-ext */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x000000DC | MRM(Tedx,    Teax,    TMxx))                     \
        EMITW(0x0000009C | MRM(Teax,    Teax,    TMxx))


#define mulzp_xr(RS)     /* Reax is in/out, prepares Redx for divzn_x* */   \
        mulzx_rr(Reax, W(RS)) /* product must not exceed operands size */

#define mulzp_xm(MS, DP) /* Reax is in/out, prepares Redx for divzn_x* */   \
        mulzx_ld(Reax, W(MS), W(DP))  /* must not exceed operands size */

/* div
 * set-flags: undefined */

#define divzx_ri(RG, IS)       /* Reax cannot be used as first operand */   \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x0000009F | MRM(REG(RG), REG(RG), TIxx))

#define divzx_rr(RG, RS)                 /* RG, RS no Reax, RS no Redx */   \
        EMITW(0x0000009F | MRM(REG(RG), REG(RG), REG(RS)))

#define divzx_ld(RG, MS, DP)   /* Reax cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000009F | MRM(REG(RG), REG(RG), TMxx))


#define divzn_ri(RG, IS)       /* Reax cannot be used as first operand */   \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x0000009E | MRM(REG(RG), REG(RG), TIxx))

#define divzn_rr(RG, RS)                 /* RG, RS no Reax, RS no Redx */   \
        EMITW(0x0000009E | MRM(REG(RG), REG(RG), REG(RS)))

#define divzn_ld(RG, MS, DP)   /* Reax cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000009E | MRM(REG(RG), REG(RG), TMxx))


#define prezx_xx()          /* to be placed immediately prior divzx_x* */   \
                                     /* to prepare Redx for int-divide */

#define prezn_xx()          /* to be placed immediately prior divzn_x* */   \
                                     /* to prepare Redx for int-divide */


#define divzx_xr(RS)     /* Reax is in/out, Redx is in(zero)/out(junk) */   \
        EMITW(0x0000009F | MRM(Teax,    Teax,    REG(RS)))

#define divzx_xm(MS, DP) /* Reax is in/out, Redx is in(zero)/out(junk) */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000009F | MRM(Teax,    Teax,    TMxx))


#define divzn_xr(RS)     /* Reax is in/out, Redx is in-sign-ext-(Reax) */   \
        EMITW(0x0000009E | MRM(Teax,    Teax,    REG(RS)))

#define divzn_xm(MS, DP) /* Reax is in/out, Redx is in-sign-ext-(Reax) */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x0000009E | MRM(Teax,    Teax,    TMxx))


#define divzp_xr(RS)     /* Reax is in/out, Redx is in-sign-ext-(Reax) */   \
        divzn_xr(W(RS))              /* destroys Redx, Xmm0 (in ARMv7) */   \
                                     /* 24-bit int (fp32 div in ARMv7) */

#define divzp_xm(MS, DP) /* Reax is in/out, Redx is in-sign-ext-(Reax) */   \
        divzn_xm(W(MS), W(DP))       /* destroys Redx, Xmm0 (in ARMv7) */   \
                                     /* 24-bit int (fp32 div in ARMv7) */

/* rem
 * set-flags: undefined */

#define remzx_ri(RG, IS)       /* Redx cannot be used as first operand */   \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x000000DF | MRM(REG(RG), REG(RG), TIxx))

#define remzx_rr(RG, RS)                 /* RG, RS no Redx, RS no Reax */   \
        EMITW(0x000000DF | MRM(REG(RG), REG(RG), REG(RS)))

#define remzx_ld(RG, MS, DP)   /* Redx cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x000000DF | MRM(REG(RG), REG(RG), TMxx))


#define remzn_ri(RG, IS)       /* Redx cannot be used as first operand */   \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x000000DE | MRM(REG(RG), REG(RG), TIxx))

#define remzn_rr(RG, RS)                 /* RG, RS no Redx, RS no Reax */   \
        EMITW(0x000000DE | MRM(REG(RG), REG(RG), REG(RS)))

#define remzn_ld(RG, MS, DP)   /* Redx cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x000000DE | MRM(REG(RG), REG(RG), TMxx))


#define remzx_xx()          /* to be placed immediately prior divzx_x* */   \
        movzx_rr(Redx, Reax)         /* to prepare for rem calculation */

#define remzx_xr(RS)        /* to be placed immediately after divzx_xr */   \
        EMITW(0x000000DF | MRM(Tedx,    Tedx,    REG(RS)))/* Redx<-rem */

#define remzx_xm(MS, DP)    /* to be placed immediately after divzx_xm */   \
        EMITW(0x000000DF | MRM(Tedx,    Tedx,    TMxx))   /* Redx<-rem */


#define remzn_xx()          /* to be placed immediately prior divzn_x* */   \
        movzx_rr(Redx, Reax)         /* to prepare for rem calculation */

#define remzn_xr(RS)        /* to be placed immediately after divzn_xr */   \
        EMITW(0x000000DE | MRM(Tedx,    Tedx,    REG(RS)))/* Redx<-rem */

#define remzn_xm(MS, DP)    /* to be placed immediately after divzn_xm */   \
        EMITW(0x000000DE | MRM(Tedx,    Tedx,    TMxx))   /* Redx<-rem */

#endif /* r6 */

/* arj
 * set-flags: undefined
 * refer to individual instructions' description
 * to stay within special register limitations */

#define arjzx_rx(RG, op, cc, lb)                                            \
        AR1(W(RG), op, zx_rx)                                               \
        CMZ(cc, MOD(RG), lb)

#define arjzx_mx(MG, DP, op, cc, lb)                                        \
        AR2(W(MG), W(DP), op, zz_mx)                                        \
        CMZ(cc, $t8,     lb)

#define arjzx_ri(RG, IS, op, cc, lb)                                        \
        AR2(W(RG), W(IS), op, zx_ri)                                        \
        CMZ(cc, MOD(RG), lb)

#define arjzx_mi(MG, DP, IS, op, cc, lb)                                    \
        AR3(W(MG), W(DP), W(IS), op, zz_mi)                                 \
        CMZ(cc, $t8,     lb)

#define arjzx_rr(RG, RS, op, cc, lb)                                        \
        AR2(W(RG), W(RS), op, zx_rr)                                        \
        CMZ(cc, MOD(RG), lb)

#define arjzx_ld(RG, MS, DP, op, cc, lb)                                    \
        AR3(W(RG), W(MS), W(DP), op, zx_ld)                                 \
        CMZ(cc, MOD(RG), lb)

#define arjzx_st(RS, MG, DP, op, cc, lb)                                    \
        AR3(W(RS), W(MG), W(DP), op, zz_st)                                 \
        CMZ(cc, $t8,     lb)

#define arjzx_mr(MG, DP, RS, op, cc, lb)                                    \
        arjzx_st(W(RS), W(MG), W(DP), op, cc, lb)

/* cmj
 * set-flags: undefined */

#define cmjzx_rz(RS, cc, lb)                                                \
        CMZ(cc, MOD(RS), lb)

#define cmjzx_mz(MS, DP, cc, lb)                                            \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        CMZ(cc, $t8,     lb)

#define cmjzx_ri(RS, IT, cc, lb)                                            \
        CMI(cc, MOD(RS), REG(RS), W(IT), lb)

#define cmjzx_mi(MS, DP, IT, cc, lb)                                        \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        CMI(cc, $t8,     TMxx,    W(IT), lb)

#define cmjzx_rr(RS, RT, cc, lb)                                            \
        CMR(cc, MOD(RS), MOD(RT), lb)

#define cmjzx_rm(RS, MT, DP, cc, lb)                                        \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MT), VAL(DP), B1(DP), P1(DP)))  \
        CMR(cc, MOD(RS), $t8,     lb)

#define cmjzx_mr(MS, DP, RT, cc, lb)                                        \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        CMR(cc, $t8,     MOD(RT), lb)

/* cmp
 * set-flags: yes */

#define cmpzx_ri(RS, IT)                                                    \
        AUW(EMPTY,    VAL(IT), TRxx,    EMPTY,   EMPTY,   EMPTY2, G3(IT))   \
        EMITW(0x00000025 | MRM(TLxx,    REG(RS), TZxx))

#define cmpzx_mi(MS, DP, IT)                                                \
        AUW(SIB(MS),  VAL(IT), TRxx,    MOD(MS), VAL(DP), C1(DP), G3(IT))   \
        EMITW(0xDC000000 | MDM(TLxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))

#define cmpzx_rr(RS, RT)                                                    \
        EMITW(0x00000025 | MRM(TRxx,    REG(RT), TZxx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RS), TZxx))

#define cmpzx_rm(RS, MT, DP)                                                \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TRxx,    MOD(MT), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RS), TZxx))

#define cmpzx_mr(MS, DP, RT)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DP), C1(DP), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TLxx,    MOD(MS), VAL(DP), B1(DP), P1(DP)))  \
        EMITW(0x00000025 | MRM(TRxx,    REG(RT), TZxx))

#endif /* RT_RTARCH_M64_H */

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
