/******************************************************************************/
/* Copyright (c) 2013-2019 VectorChief (at github, bitbucket, sourceforge)    */
/* Distributed under the MIT software license, see the accompanying           */
/* file COPYING or http://www.opensource.org/licenses/mit-license.php         */
/******************************************************************************/

#ifndef RT_RTARCH_XHB_128X2V4_H
#define RT_RTARCH_XHB_128X2V4_H

/******************************************************************************/
/*********************************   LEGEND   *********************************/
/******************************************************************************/

/*
 * rtarch_xHB_128x2v4.h: Implementation of x86_64 half+byte SSE2/4 pairs.
 *
 * This file is a part of the unified SIMD assembler framework (rtarch.h)
 * designed to be compatible with different processor architectures,
 * while maintaining strictly defined common API.
 *
 * Recommended naming scheme for instructions:
 *
 * cmda*_rx - applies [cmd] to 256-bit packed-half: [r]egister (one operand)
 * cmda*_rr - applies [cmd] to 256-bit packed-half: [r]egister from [r]egister
 *
 * cmda*_rm - applies [cmd] to 256-bit packed-half: [r]egister from [m]emory
 * cmda*_ld - applies [cmd] to 256-bit packed-half: as above (friendly alias)
 *
 * Interpretation of instruction parameters:
 *
 * upper-case params have triplet structure and require W to pass-forward
 * lower-case params are singular and can be used/passed as such directly
 *
 * XD - SIMD register serving as destination only, if present
 * XG - SIMD register serving as destination and first source
 * XS - SIMD register serving as second source (first if any)
 * XT - SIMD register serving as third source (second if any)
 *
 * RD - BASE register serving as destination only, if present
 * RG - BASE register serving as destination and first source
 * RS - BASE register serving as second source (first if any)
 * RT - BASE register serving as third source (second if any)
 *
 * MD - BASE addressing mode (Oeax, M***, I***) (memory-dest)
 * MG - BASE addressing mode (Oeax, M***, I***) (memory-dsrc)
 * MS - BASE addressing mode (Oeax, M***, I***) (memory-src2)
 * MT - BASE addressing mode (Oeax, M***, I***) (memory-src3)
 *
 * DD - displacement value (DP, DF, DG, DH, DV) (memory-dest)
 * DG - displacement value (DP, DF, DG, DH, DV) (memory-dsrc)
 * DS - displacement value (DP, DF, DG, DH, DV) (memory-src2)
 * DT - displacement value (DP, DF, DG, DH, DV) (memory-src3)
 *
 * IS - immediate value (is used as a second or first source)
 * IT - immediate value (is used as a third or second source)
 */

/******************************************************************************/
/********************************   INTERNAL   ********************************/
/******************************************************************************/

#if (defined RT_SIMD_CODE)

#if (RT_128X2 == 4)

/******************************************************************************/
/********************************   EXTERNAL   ********************************/
/******************************************************************************/

/******************************************************************************/
/**********************************   SIMD   **********************************/
/******************************************************************************/

/****************   packed half-precision generic move/logic   ****************/

/* mov (D = S) */

#define movax_rr(XD, XS)                                                    \
        REX(0,             0) EMITB(0x0F) EMITB(0x28)                       \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        REX(1,             1) EMITB(0x0F) EMITB(0x28)                       \
        MRM(REG(XD), MOD(XS), REG(XS))

#define movax_ld(XD, MS, DS)                                                \
    ADR REX(0,       RXB(MS)) EMITB(0x0F) EMITB(0x28)                       \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
    ADR REX(1,       RXB(MS)) EMITB(0x0F) EMITB(0x28)                       \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VYL(DS)), EMPTY)

#define movax_st(XS, MD, DD)                                                \
    ADR REX(0,       RXB(MD)) EMITB(0x0F) EMITB(0x29)                       \
        MRM(REG(XS),    0x02, REG(MD))                                      \
        AUX(SIB(MD), EMITW(VAL(DD)), EMPTY)                                 \
    ADR REX(1,       RXB(MD)) EMITB(0x0F) EMITB(0x29)                       \
        MRM(REG(XS),    0x02, REG(MD))                                      \
        AUX(SIB(MD), EMITW(VYL(DD)), EMPTY)

/* mmv (G = G mask-merge S) where (mask-elem: 0 keeps G, -1 picks S)
 * uses Xmm0 implicitly as a mask register, destroys Xmm0, 0-masked XS elems */

#define mmvax_rr(XG, XS)                                                    \
        andax_rr(W(XS), Xmm0)                                               \
        annax_rr(Xmm0, W(XG))                                               \
        orrax_rr(Xmm0, W(XS))                                               \
        movax_rr(W(XG), Xmm0)

#define mmvax_ld(XG, MS, DS)                                                \
        notax_rx(Xmm0)                                                      \
        andax_rr(W(XG), Xmm0)                                               \
        annax_ld(Xmm0, W(MS), W(DS))                                        \
        orrax_rr(W(XG), Xmm0)

#define mmvax_st(XS, MG, DG)                                                \
        andax_rr(W(XS), Xmm0)                                               \
        annax_ld(Xmm0, W(MG), W(DG))                                        \
        orrax_rr(Xmm0, W(XS))                                               \
        movax_st(Xmm0, W(MG), W(DG))

/* and (G = G & S), (D = S & T) if (#D != #S) */

#define andax_rr(XG, XS)                                                    \
    ESC REX(0,             0) EMITB(0x0F) EMITB(0xDB)                       \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
    ESC REX(1,             1) EMITB(0x0F) EMITB(0xDB)                       \
        MRM(REG(XG), MOD(XS), REG(XS))

#define andax_ld(XG, MS, DS)                                                \
ADR ESC REX(0,       RXB(MS)) EMITB(0x0F) EMITB(0xDB)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
ADR ESC REX(1,       RXB(MS)) EMITB(0x0F) EMITB(0xDB)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VYL(DS)), EMPTY)

#define andax3rr(XD, XS, XT)                                                \
        movax_rr(W(XD), W(XS))                                              \
        andax_rr(W(XD), W(XT))

#define andax3ld(XD, XS, MT, DT)                                            \
        movax_rr(W(XD), W(XS))                                              \
        andax_ld(W(XD), W(MT), W(DT))

/* ann (G = ~G & S), (D = ~S & T) if (#D != #S) */

#define annax_rr(XG, XS)                                                    \
    ESC REX(0,             0) EMITB(0x0F) EMITB(0xDF)                       \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
    ESC REX(1,             1) EMITB(0x0F) EMITB(0xDF)                       \
        MRM(REG(XG), MOD(XS), REG(XS))

#define annax_ld(XG, MS, DS)                                                \
ADR ESC REX(0,       RXB(MS)) EMITB(0x0F) EMITB(0xDF)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
ADR ESC REX(1,       RXB(MS)) EMITB(0x0F) EMITB(0xDF)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VYL(DS)), EMPTY)

#define annax3rr(XD, XS, XT)                                                \
        movax_rr(W(XD), W(XS))                                              \
        annax_rr(W(XD), W(XT))

#define annax3ld(XD, XS, MT, DT)                                            \
        movax_rr(W(XD), W(XS))                                              \
        annax_ld(W(XD), W(MT), W(DT))

/* orr (G = G | S), (D = S | T) if (#D != #S) */

#define orrax_rr(XG, XS)                                                    \
    ESC REX(0,             0) EMITB(0x0F) EMITB(0xEB)                       \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
    ESC REX(1,             1) EMITB(0x0F) EMITB(0xEB)                       \
        MRM(REG(XG), MOD(XS), REG(XS))

#define orrax_ld(XG, MS, DS)                                                \
ADR ESC REX(0,       RXB(MS)) EMITB(0x0F) EMITB(0xEB)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
ADR ESC REX(1,       RXB(MS)) EMITB(0x0F) EMITB(0xEB)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VYL(DS)), EMPTY)

#define orrax3rr(XD, XS, XT)                                                \
        movax_rr(W(XD), W(XS))                                              \
        orrax_rr(W(XD), W(XT))

#define orrax3ld(XD, XS, MT, DT)                                            \
        movax_rr(W(XD), W(XS))                                              \
        orrax_ld(W(XD), W(MT), W(DT))

/* orn (G = ~G | S), (D = ~S | T) if (#D != #S) */

#define ornax_rr(XG, XS)                                                    \
        notax_rx(W(XG))                                                     \
        orrax_rr(W(XG), W(XS))

#define ornax_ld(XG, MS, DS)                                                \
        notax_rx(W(XG))                                                     \
        orrax_ld(W(XG), W(MS), W(DS))

#define ornax3rr(XD, XS, XT)                                                \
        notax_rr(W(XD), W(XS))                                              \
        orrax_rr(W(XD), W(XT))

#define ornax3ld(XD, XS, MT, DT)                                            \
        notax_rr(W(XD), W(XS))                                              \
        orrax_ld(W(XD), W(MT), W(DT))

/* xor (G = G ^ S), (D = S ^ T) if (#D != #S) */

#define xorax_rr(XG, XS)                                                    \
    ESC REX(0,             0) EMITB(0x0F) EMITB(0xEF)                       \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
    ESC REX(1,             1) EMITB(0x0F) EMITB(0xEF)                       \
        MRM(REG(XG), MOD(XS), REG(XS))

#define xorax_ld(XG, MS, DS)                                                \
ADR ESC REX(0,       RXB(MS)) EMITB(0x0F) EMITB(0xEF)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
ADR ESC REX(1,       RXB(MS)) EMITB(0x0F) EMITB(0xEF)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VYL(DS)), EMPTY)

#define xorax3rr(XD, XS, XT)                                                \
        movax_rr(W(XD), W(XS))                                              \
        xorax_rr(W(XD), W(XT))

#define xorax3ld(XD, XS, MT, DT)                                            \
        movax_rr(W(XD), W(XS))                                              \
        xorax_ld(W(XD), W(MT), W(DT))

/* not (G = ~G), (D = ~S) */

#define notax_rx(XG)                                                        \
        annax_ld(W(XG), Mebp, inf_GPC07)

#define notax_rr(XD, XS)                                                    \
        movax_rr(W(XD), W(XS))                                              \
        notax_rx(W(XD))

/*************   packed half-precision integer arithmetic/shifts   ************/

/* add (G = G + S), (D = S + T) if (#D != #S) */

#define addax_rr(XG, XS)                                                    \
    ESC REX(0,             0) EMITB(0x0F) EMITB(0xFD)                       \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
    ESC REX(1,             1) EMITB(0x0F) EMITB(0xFD)                       \
        MRM(REG(XG), MOD(XS), REG(XS))

#define addax_ld(XG, MS, DS)                                                \
ADR ESC REX(0,       RXB(MS)) EMITB(0x0F) EMITB(0xFD)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
ADR ESC REX(1,       RXB(MS)) EMITB(0x0F) EMITB(0xFD)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VYL(DS)), EMPTY)

#define addax3rr(XD, XS, XT)                                                \
        movax_rr(W(XD), W(XS))                                              \
        addax_rr(W(XD), W(XT))

#define addax3ld(XD, XS, MT, DT)                                            \
        movax_rr(W(XD), W(XS))                                              \
        addax_ld(W(XD), W(MT), W(DT))

/* ads (G = G + S), (D = S + T) if (#D != #S) - saturate, unsigned */

#define adsax_rr(XG, XS)                                                    \
    ESC REX(0,             0) EMITB(0x0F) EMITB(0xDD)                       \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
    ESC REX(1,             1) EMITB(0x0F) EMITB(0xDD)                       \
        MRM(REG(XG), MOD(XS), REG(XS))

#define adsax_ld(XG, MS, DS)                                                \
ADR ESC REX(0,       RXB(MS)) EMITB(0x0F) EMITB(0xDD)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
ADR ESC REX(1,       RXB(MS)) EMITB(0x0F) EMITB(0xDD)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VYL(DS)), EMPTY)

#define adsax3rr(XD, XS, XT)                                                \
        movax_rr(W(XD), W(XS))                                              \
        adsax_rr(W(XD), W(XT))

#define adsax3ld(XD, XS, MT, DT)                                            \
        movax_rr(W(XD), W(XS))                                              \
        adsax_ld(W(XD), W(MT), W(DT))

/* ads (G = G + S), (D = S + T) if (#D != #S) - saturate, signed */

#define adsan_rr(XG, XS)                                                    \
    ESC REX(0,             0) EMITB(0x0F) EMITB(0xED)                       \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
    ESC REX(1,             1) EMITB(0x0F) EMITB(0xED)                       \
        MRM(REG(XG), MOD(XS), REG(XS))

#define adsan_ld(XG, MS, DS)                                                \
ADR ESC REX(0,       RXB(MS)) EMITB(0x0F) EMITB(0xED)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
ADR ESC REX(1,       RXB(MS)) EMITB(0x0F) EMITB(0xED)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VYL(DS)), EMPTY)

#define adsan3rr(XD, XS, XT)                                                \
        movax_rr(W(XD), W(XS))                                              \
        adsan_rr(W(XD), W(XT))

#define adsan3ld(XD, XS, MT, DT)                                            \
        movax_rr(W(XD), W(XS))                                              \
        adsan_ld(W(XD), W(MT), W(DT))

/* sub (G = G - S), (D = S - T) if (#D != #S) */

#define subax_rr(XG, XS)                                                    \
    ESC REX(0,             0) EMITB(0x0F) EMITB(0xF9)                       \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
    ESC REX(1,             1) EMITB(0x0F) EMITB(0xF9)                       \
        MRM(REG(XG), MOD(XS), REG(XS))

#define subax_ld(XG, MS, DS)                                                \
ADR ESC REX(0,       RXB(MS)) EMITB(0x0F) EMITB(0xF9)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
ADR ESC REX(1,       RXB(MS)) EMITB(0x0F) EMITB(0xF9)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VYL(DS)), EMPTY)

#define subax3rr(XD, XS, XT)                                                \
        movax_rr(W(XD), W(XS))                                              \
        subax_rr(W(XD), W(XT))

#define subax3ld(XD, XS, MT, DT)                                            \
        movax_rr(W(XD), W(XS))                                              \
        subax_ld(W(XD), W(MT), W(DT))

/* sbs (G = G - S), (D = S - T) if (#D != #S) - saturate, unsigned */

#define sbsax_rr(XG, XS)                                                    \
    ESC REX(0,             0) EMITB(0x0F) EMITB(0xD9)                       \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
    ESC REX(1,             1) EMITB(0x0F) EMITB(0xD9)                       \
        MRM(REG(XG), MOD(XS), REG(XS))

#define sbsax_ld(XG, MS, DS)                                                \
ADR ESC REX(0,       RXB(MS)) EMITB(0x0F) EMITB(0xD9)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
ADR ESC REX(1,       RXB(MS)) EMITB(0x0F) EMITB(0xD9)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VYL(DS)), EMPTY)

#define sbsax3rr(XD, XS, XT)                                                \
        movax_rr(W(XD), W(XS))                                              \
        sbsax_rr(W(XD), W(XT))

#define sbsax3ld(XD, XS, MT, DT)                                            \
        movax_rr(W(XD), W(XS))                                              \
        sbsax_ld(W(XD), W(MT), W(DT))

/* sbs (G = G - S), (D = S - T) if (#D != #S) - saturate, signed */

#define sbsan_rr(XG, XS)                                                    \
    ESC REX(0,             0) EMITB(0x0F) EMITB(0xE9)                       \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
    ESC REX(1,             1) EMITB(0x0F) EMITB(0xE9)                       \
        MRM(REG(XG), MOD(XS), REG(XS))

#define sbsan_ld(XG, MS, DS)                                                \
ADR ESC REX(0,       RXB(MS)) EMITB(0x0F) EMITB(0xE9)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
ADR ESC REX(1,       RXB(MS)) EMITB(0x0F) EMITB(0xE9)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VYL(DS)), EMPTY)

#define sbsan3rr(XD, XS, XT)                                                \
        movax_rr(W(XD), W(XS))                                              \
        sbsan_rr(W(XD), W(XT))

#define sbsan3ld(XD, XS, MT, DT)                                            \
        movax_rr(W(XD), W(XS))                                              \
        sbsan_ld(W(XD), W(MT), W(DT))

/* mul (G = G * S), (D = S * T) if (#D != #S) */

#define mulax_rr(XG, XS)                                                    \
    ESC REX(0,             0) EMITB(0x0F) EMITB(0xD5)                       \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
    ESC REX(1,             1) EMITB(0x0F) EMITB(0xD5)                       \
        MRM(REG(XG), MOD(XS), REG(XS))

#define mulax_ld(XG, MS, DS)                                                \
ADR ESC REX(0,       RXB(MS)) EMITB(0x0F) EMITB(0xD5)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
ADR ESC REX(1,       RXB(MS)) EMITB(0x0F) EMITB(0xD5)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VYL(DS)), EMPTY)

#define mulax3rr(XD, XS, XT)                                                \
        movax_rr(W(XD), W(XS))                                              \
        mulax_rr(W(XD), W(XT))

#define mulax3ld(XD, XS, MT, DT)                                            \
        movax_rr(W(XD), W(XS))                                              \
        mulax_ld(W(XD), W(MT), W(DT))

/* shl (G = G << S), (D = S << T) if (#D != #S) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shlax_ri(XG, IS)                                                    \
    ESC REX(0,             0) EMITB(0x0F) EMITB(0x71)                       \
        MRM(0x06,    MOD(XG), REG(XG))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IS)))                               \
    ESC REX(0,             1) EMITB(0x0F) EMITB(0x71)                       \
        MRM(0x06,    MOD(XG), REG(XG))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IS)))

#define shlax_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
ADR ESC REX(0,       RXB(MS)) EMITB(0x0F) EMITB(0xF1)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
ADR ESC REX(1,       RXB(MS)) EMITB(0x0F) EMITB(0xF1)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)

#define shlax3ri(XD, XS, IT)                                                \
        movax_rr(W(XD), W(XS))                                              \
        shlax_ri(W(XD), W(IT))

#define shlax3ld(XD, XS, MT, DT)                                            \
        movax_rr(W(XD), W(XS))                                              \
        shlax_ld(W(XD), W(MT), W(DT))

/* shr (G = G >> S), (D = S >> T) if (#D != #S) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrax_ri(XG, IS)                                                    \
    ESC REX(0,             0) EMITB(0x0F) EMITB(0x71)                       \
        MRM(0x02,    MOD(XG), REG(XG))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IS)))                               \
    ESC REX(0,             1) EMITB(0x0F) EMITB(0x71)                       \
        MRM(0x02,    MOD(XG), REG(XG))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IS)))

#define shrax_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
ADR ESC REX(0,       RXB(MS)) EMITB(0x0F) EMITB(0xD1)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
ADR ESC REX(1,       RXB(MS)) EMITB(0x0F) EMITB(0xD1)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)

#define shrax3ri(XD, XS, IT)                                                \
        movax_rr(W(XD), W(XS))                                              \
        shrax_ri(W(XD), W(IT))

#define shrax3ld(XD, XS, MT, DT)                                            \
        movax_rr(W(XD), W(XS))                                              \
        shrax_ld(W(XD), W(MT), W(DT))

/* shr (G = G >> S), (D = S >> T) if (#D != #S) - plain, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define shran_ri(XG, IS)                                                    \
    ESC REX(0,             0) EMITB(0x0F) EMITB(0x71)                       \
        MRM(0x04,    MOD(XG), REG(XG))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IS)))                               \
    ESC REX(0,             1) EMITB(0x0F) EMITB(0x71)                       \
        MRM(0x04,    MOD(XG), REG(XG))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IS)))

#define shran_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
ADR ESC REX(0,       RXB(MS)) EMITB(0x0F) EMITB(0xE1)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
ADR ESC REX(1,       RXB(MS)) EMITB(0x0F) EMITB(0xE1)                       \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)

#define shran3ri(XD, XS, IT)                                                \
        movax_rr(W(XD), W(XS))                                              \
        shran_ri(W(XD), W(IT))

#define shran3ld(XD, XS, MT, DT)                                            \
        movax_rr(W(XD), W(XS))                                              \
        shran_ld(W(XD), W(MT), W(DT))

/* svl (G = G << S), (D = S << T) if (#D != #S) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svlax_rr(XG, XS)     /* variable shift with per-elem count */       \
        svlax3rr(W(XG), W(XG), W(XS))

#define svlax_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svlax3ld(W(XG), W(XG), W(MS), W(DS))

#define svlax3rr(XD, XS, XT)                                                \
        movax_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movax_st(W(XT), Mebp, inf_SCR02(0))                                 \
        svlax_xx()                                                          \
        movax_ld(W(XD), Mebp, inf_SCR01(0))

#define svlax3ld(XD, XS, MT, DT)                                            \
        movax_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR02(0))                                 \
        svlax_xx()                                                          \
        movax_ld(W(XD), Mebp, inf_SCR01(0))

#define svlax_xx() /* not portable, do not use outside */                   \
        stack_st(Recx)                                                      \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x00))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x02))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x02))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x04))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x04))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x06))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x06))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x08))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0A))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x0A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0C))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x0C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0E))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x0E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x10))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x12))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x12))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x14))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x14))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x16))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x16))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x18))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1A))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x1A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1C))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x1C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1E))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x1E))                                    \
        stack_ld(Recx)

/* svr (G = G >> S), (D = S >> T) if (#D != #S) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrax_rr(XG, XS)     /* variable shift with per-elem count */       \
        svrax3rr(W(XG), W(XG), W(XS))

#define svrax_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svrax3ld(W(XG), W(XG), W(MS), W(DS))

#define svrax3rr(XD, XS, XT)                                                \
        movax_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movax_st(W(XT), Mebp, inf_SCR02(0))                                 \
        svrax_xx()                                                          \
        movax_ld(W(XD), Mebp, inf_SCR01(0))

#define svrax3ld(XD, XS, MT, DT)                                            \
        movax_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR02(0))                                 \
        svrax_xx()                                                          \
        movax_ld(W(XD), Mebp, inf_SCR01(0))

#define svrax_xx() /* not portable, do not use outside */                   \
        stack_st(Recx)                                                      \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x00))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x02))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x02))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x04))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x04))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x06))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x06))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x08))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0A))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x0A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0C))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x0C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0E))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x0E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x10))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x12))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x12))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x14))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x14))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x16))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x16))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x18))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1A))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x1A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1C))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x1C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1E))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x1E))                                    \
        stack_ld(Recx)

/* svr (G = G >> S), (D = S >> T) if (#D != #S) - variable, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define svran_rr(XG, XS)     /* variable shift with per-elem count */       \
        svran3rr(W(XG), W(XG), W(XS))

#define svran_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svran3ld(W(XG), W(XG), W(MS), W(DS))

#define svran3rr(XD, XS, XT)                                                \
        movax_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movax_st(W(XT), Mebp, inf_SCR02(0))                                 \
        svran_xx()                                                          \
        movax_ld(W(XD), Mebp, inf_SCR01(0))

#define svran3ld(XD, XS, MT, DT)                                            \
        movax_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR02(0))                                 \
        svran_xx()                                                          \
        movax_ld(W(XD), Mebp, inf_SCR01(0))

#define svran_xx() /* not portable, do not use outside */                   \
        stack_st(Recx)                                                      \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x00))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x02))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x02))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x04))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x04))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x06))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x06))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x08))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0A))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x0A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0C))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x0C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0E))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x0E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x10))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x12))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x12))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x14))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x14))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x16))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x16))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x18))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1A))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x1A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1C))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x1C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1E))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x1E))                                    \
        stack_ld(Recx)

/******************************************************************************/
/********************************   INTERNAL   ********************************/
/******************************************************************************/

#endif /* RT_128X2 */

#endif /* RT_SIMD_CODE */

#endif /* RT_RTARCH_XHB_128X2V4_H */

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
