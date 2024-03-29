/*
 * MIPS ASE DSP Instruction emulation helpers for QEMU.
 *
 * Copyright (c) 2012  Jia Liu <proljc@gmail.com>
 *                     Dongxue Zhang <elat.era@gmail.com>
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "cpu.h"
#include "helper.h"

/*** MIPS DSP internal functions begin ***/
static inline int not_word_value(target_long value)
{
    target_ulong temp;
    temp = (target_long)(int32_t)(value & 0xFFFFFFFF);
    return value == temp ? 0 : 1;
}

static inline void set_DSPControl_overflow_flag(CPUMIPSState *env,
                                                uint32_t flag, int position)
{
    env->active_tc.DSPControl |= (target_ulong)flag << position;
}

static inline void set_DSPControl_carryflag(CPUMIPSState *env, uint32_t flag)
{
    env->active_tc.DSPControl |= (target_ulong)flag << 13;
}

static inline uint32_t get_DSPControl_carryflag(CPUMIPSState *env)
{
    return (env->active_tc.DSPControl >> 13) & 0x01;
}

static inline void set_DSPControl_24(CPUMIPSState *env, uint32_t flag, int len)
{
  uint32_t filter;

  filter = ((0x01 << len) - 1) << 24;
  filter = ~filter;

  env->active_tc.DSPControl &= filter;
  env->active_tc.DSPControl |= (target_ulong)flag << 24;
}

static inline uint32_t get_DSPControl_24(CPUMIPSState *env, int len)
{
  uint32_t filter;

  filter = (0x01 << len) - 1;

  return (env->active_tc.DSPControl >> 24) & filter;
}

static inline void set_DSPControl_pos(CPUMIPSState *env, uint32_t pos)
{
    target_ulong dspc;

    dspc = env->active_tc.DSPControl;
#ifndef TARGET_MIPS64
    dspc = dspc & 0xFFFFFFC0;
    dspc |= pos;
#else
    dspc = dspc & 0xFFFFFF80;
    dspc |= pos;
#endif
    env->active_tc.DSPControl = dspc;
}

static inline uint32_t get_DSPControl_pos(CPUMIPSState *env)
{
    target_ulong dspc;
    uint32_t pos;

    dspc = env->active_tc.DSPControl;

#ifndef TARGET_MIPS64
    pos = dspc & 0x3F;
#else
    pos = dspc & 0x7F;
#endif

    return pos;
}

static inline void set_DSPControl_efi(CPUMIPSState *env, uint32_t flag)
{
    env->active_tc.DSPControl &= 0xFFFFBFFF;
    env->active_tc.DSPControl |= (target_ulong)flag << 14;
}

/* get abs value */
static inline int8_t mipsdsp_sat_abs_u8(CPUMIPSState *env, uint8_t a)
{
    if ((uint8_t)a == 0x80) {
        set_DSPControl_overflow_flag(env, 1, 20);
        a = 0x7f;
    } else {
        if (((uint8_t)a & 0x80) == 0x80) {
            a = -a;
        }
    }

    return a;
}

static inline int16_t mipsdsp_sat_abs_u16(CPUMIPSState *env, int16_t a)
{
    if ((uint16_t)a == 0x8000) {
        set_DSPControl_overflow_flag(env, 1, 20);
        a = 0x7fff;
    } else {
        if (((uint16_t)a & 0x8000) == 0x8000) {
            a = -a;
        }
    }

    return a;
}

static inline int32_t mipsdsp_sat_abs_u32(CPUMIPSState *env, int32_t a)
{
    if (a == 0x80000000) {
        set_DSPControl_overflow_flag(env, 1, 20);
        a = 0x7FFFFFFF;
    } else {
        if ((a & 0x80000000) == 0x80000000) {
            a = -a;
        }
    }

    return a;
}

/* get sum value */
static inline int16_t mipsdsp_add_i16(CPUMIPSState *env, int16_t a, int16_t b)
{
    int32_t tempI, temp15, temp16;

    tempI = a + b;
    temp15 = (tempI & 0x8000) >> 15;
    temp16 = (tempI & 0x10000) >> 16;

    if (temp15 != temp16) {
        set_DSPControl_overflow_flag(env, 1, 20);
    }

    return a + b;
}

static inline int16_t mipsdsp_sat_add_i16(CPUMIPSState *env,
                                          int16_t a, int16_t b)
{
    int16_t tempS;
    int32_t tempI, temp15, temp16;

    tempS = a + b;
    tempI = (int32_t)a + (int32_t)b;
    temp15 = (tempI & 0x8000) >> 15;
    temp16 = (tempI & 0x10000) >> 16;

    if (temp15 != temp16) {
        if (temp16 == 0) {
            tempS = 0x7FFF;
        } else {
            tempS = 0x8000;
        }
        set_DSPControl_overflow_flag(env, 1, 20);
    }

    return tempS;
}

static inline int32_t mipsdsp_sat_add_i32(CPUMIPSState *env,
                                          int32_t a, int32_t b)
{
    int32_t tempI;
    int64_t tempL, temp31, temp32;

    tempI = a + b;
    tempL = (int64_t)a + (int64_t)b;
    temp31 = (tempL & 0x80000000) >> 31;
    temp32 = (tempL & 0x100000000ull) >> 32;

    if (temp31 != temp32) {
        if (temp32 == 0) {
            tempI = 0x7FFFFFFF;
        } else {
            tempI = 0x80000000;
        }
        set_DSPControl_overflow_flag(env, 1, 20);
    }

    return tempI;
}

static inline uint8_t mipsdsp_add_u8(CPUMIPSState *env, uint8_t a, uint8_t b)
{
    uint16_t temp;

    temp = (uint16_t)a + (uint16_t)b;

    if ((temp & 0x0100) == 0x0100) {
        set_DSPControl_overflow_flag(env, 1, 20);
    }

    return temp & 0xFF;
}

static inline uint16_t mipsdsp_add_u16(CPUMIPSState *env,
                                       uint16_t a, uint16_t b)
{
    uint32_t temp;

    temp = (uint32_t)a + (uint32_t)b;

    if ((temp & 0x00010000) == 0x00010000) {
        set_DSPControl_overflow_flag(env, 1, 20);
    }

    return temp & 0xFFFF;
}

static inline uint8_t mipsdsp_sat_add_u8(CPUMIPSState *env,
                                         uint8_t a, uint8_t b)
{
    uint8_t  result;
    uint16_t temp;

    temp = (uint16_t)a + (uint16_t)b;
    result = temp & 0xFF;

    if ((0x0100 & temp) == 0x0100) {
        result = 0xFF;
        set_DSPControl_overflow_flag(env, 1, 20);
    }

    return result;
}

static inline uint16_t mipsdsp_sat_add_u16(CPUMIPSState *env,
                                           uint16_t a, uint16_t b)
{
    uint16_t result;
    uint32_t temp;

    temp = (uint32_t)a + (uint32_t)b;
    result = temp & 0xFFFF;

    if ((0x00010000 & temp) == 0x00010000) {
        result = 0xFFFF;
        set_DSPControl_overflow_flag(env, 1, 20);
    }

    return result;
}

static inline int32_t mipsdsp_sat32_acc_q31(CPUMIPSState *env,
                                            int32_t acc, int32_t a)
{
    int64_t temp;
    int32_t temp32, temp31, result;
    int64_t temp_sum;

#ifndef TARGET_MIPS64
    temp = ((uint64_t)env->active_tc.HI[acc] << 32) |
           (uint64_t)env->active_tc.LO[acc];
#else
    temp = (uint64_t)env->active_tc.LO[acc];
#endif

    temp_sum = (int64_t)a + temp;

    temp32 = (temp_sum >> 32) & 0x01;
    temp31 = (temp_sum >> 31) & 0x01;
    result = temp_sum & 0xFFFFFFFF;

    if (temp32 != temp31) {
        if (temp32 == 0) {
            result = 0x80000000;
        } else {
            result = 0x7FFFFFFF;
        }
        set_DSPControl_overflow_flag(env, 1, 16 + acc);
    }

    return result;
}

/* a[0] is LO, a[1] is HI. */
static inline void mipsdsp_sat64_acc_add_q63(CPUMIPSState *env,
                                             int64_t *ret,
                                             int32_t ac,
                                             int64_t *a)
{
    uint32_t temp64, temp63;
    int64_t temp[3];
    int64_t acc[3];
    int64_t temp_sum;

    temp[0] = a[0];
    temp[1] = a[1];
    if (((temp[1] >> 63) & 0x01) == 0) {
        temp[2] = 0x00;
    } else {
        temp[2] = ~0ull;
    }

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];
    if (((acc[1] >> 63) & 0x01) == 0) {
        acc[2] = 0x00;
    } else {
        acc[2] = ~0ull;
    }

    temp_sum = temp[0] + acc[0];
    if (((uint64_t)temp_sum < (uint64_t)temp[0]) &&
       ((uint64_t)temp_sum < (uint64_t)acc[0])) {
        temp[1] += 1;
        if (temp[1] == 0) {
            temp[2] += 1;
        }
    }
    temp[0] = temp_sum;

    temp_sum = temp[1] + acc[1];
    if (((uint64_t)temp_sum < (uint64_t)temp[1]) &&
       ((uint64_t)temp_sum < (uint64_t)acc[1])) {
        temp[2] += 1;
    }

    temp[1] = temp_sum;
    temp[2] += acc[2];
    temp64 = temp[1] & 0x01;
    temp63 = (temp[0] >> 63) & 0x01;

    if (temp64 != temp63) {
        if (temp64 == 1) {
            ret[0] = 0x8000000000000000ull;
            ret[1] = ~0ull;
        } else {
            ret[0] = 0x0;
            ret[1] = 0x7FFFFFFFFFFFFFFFull;
        }
        set_DSPControl_overflow_flag(env, 1, 16 + ac);
    } else {
        ret[0] = temp[0];
        ret[1] = temp[1];
    }
}

/* a[0] is LO, a[1] is HI. */
static inline void mipsdsp_sat64_acc_sub_q63(CPUMIPSState *env,
                                             int64_t *ret,
                                             int32_t ac,
                                             int64_t *a)
{
    uint32_t temp64, temp63;
    int64_t temp[3];
    int64_t acc[3];
    int64_t temp_sum;

    temp[0] = a[0];
    temp[1] = a[1];
    if (((temp[1] >> 63) & 0x01) == 0) {
        temp[2] = 0x00;
    } else {
        temp[2] = ~0ull;
    }

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];
    if (((acc[1] >> 63) & 0x01) == 0) {
        acc[2] = 0x00;
    } else {
        acc[2] = ~0ull;
    }

    temp_sum = acc[0] - temp[0];
    if ((uint64_t)temp_sum > (uint64_t)acc[0]) {
        acc[1] -= 1;
        if (acc[1] == ~0ull) {
            acc[2] -= 1;
        }
    }
    acc[0] = temp_sum;

    temp_sum = acc[1] - temp[1];
    if ((uint64_t)temp_sum > (uint64_t)acc[1]) {
        acc[2] -= 1;
    }
    acc[1] = temp_sum;
    acc[2] -= temp[2];

    temp64 = acc[1] & 0x01;
    temp63 = (acc[0] >> 63) & 0x01;

    if (temp64 != temp63) {
        if (temp64 == 1) {
            ret[0] = 0x8000000000000000ull;
            ret[1] = ~0ull;
        } else {
            ret[0] = 0x0;
            ret[1] = 0x7FFFFFFFFFFFFFFFull;
        }
        set_DSPControl_overflow_flag(env, 1, 16 + ac);
    } else {
        ret[0] = acc[0];
        ret[1] = acc[1];
    }
}

static inline int32_t mipsdsp_mul_i16_i16(CPUMIPSState *env,
                                          int16_t a, int16_t b)
{
    int32_t temp;

    temp = (int32_t)a * (int32_t)b;

    if ((temp > 0x7FFF) || (temp < 0xFFFF8000)) {
        set_DSPControl_overflow_flag(env, 1, 21);
    }
    temp &= 0x0000FFFF;

    return temp;
}

static inline int32_t mipsdsp_sat16_mul_i16_i16(CPUMIPSState *env,
                                                int16_t a, int16_t b)
{
    int32_t temp;

    temp = (int32_t)a * (int32_t)b;

    if (temp > 0x7FFF) {
        temp = 0x00007FFF;
        set_DSPControl_overflow_flag(env, 1, 21);
    } else if (temp < 0x00007FFF) {
        temp = 0xFFFF8000;
        set_DSPControl_overflow_flag(env, 1, 21);
    }
    temp &= 0x0000FFFF;

    return temp;
}

static inline int32_t mipsdsp_mul_q15_q15_overflowflag21(CPUMIPSState *env,
                                                         uint16_t a, uint16_t b)
{
    int32_t temp;

    if ((a == 0x8000) && (b == 0x8000)) {
        temp = 0x7FFFFFFF;
        set_DSPControl_overflow_flag(env, 1, 21);
    } else {
        temp = ((int32_t)(int16_t)a * (int32_t)(int16_t)b) << 1;
    }

    return temp;
}

/* right shift */
static inline int16_t mipsdsp_rshift1_add_q16(int16_t a, int16_t b)
{
    int32_t temp;

    temp = (int32_t)a + (int32_t)b;

    return (temp >> 1) & 0xFFFF;
}

/* round right shift */
static inline int16_t mipsdsp_rrshift1_add_q16(int16_t a, int16_t b)
{
    int32_t temp;

    temp = (int32_t)a + (int32_t)b;
    temp += 1;

    return (temp >> 1) & 0xFFFF;
}

static inline int32_t mipsdsp_rshift1_add_q32(int32_t a, int32_t b)
{
    int64_t temp;

    temp = (int64_t)a + (int64_t)b;

    return (temp >> 1) & 0xFFFFFFFF;
}

static inline int32_t mipsdsp_rrshift1_add_q32(int32_t a, int32_t b)
{
    int64_t temp;

    temp = (int64_t)a + (int64_t)b;
    temp += 1;

    return (temp >> 1) & 0xFFFFFFFF;
}

static inline uint8_t mipsdsp_rshift1_add_u8(uint8_t a, uint8_t b)
{
    uint16_t temp;

    temp = (uint16_t)a + (uint16_t)b;

    return (temp >> 1) & 0x00FF;
}

static inline uint8_t mipsdsp_rrshift1_add_u8(uint8_t a, uint8_t b)
{
    uint16_t temp;

    temp = (uint16_t)a + (uint16_t)b + 1;

    return (temp >> 1) & 0x00FF;
}

static inline int64_t mipsdsp_rashift_short_acc(CPUMIPSState *env,
                                                int32_t ac,
                                                int32_t shift)
{
    int32_t sign, temp31;
    int64_t temp, acc;

    sign = (env->active_tc.HI[ac] >> 31) & 0x01;
    acc = ((int64_t)env->active_tc.HI[ac] << 32) |
          ((int64_t)env->active_tc.LO[ac] & 0xFFFFFFFF);
    if (shift == 0) {
        temp = acc;
    } else {
        if (sign == 0) {
            temp = (((int64_t)0x01 << (32 - shift + 1)) - 1) & (acc >> shift);
        } else {
            temp = ((((int64_t)0x01 << (shift + 1)) - 1) << (32 - shift)) |
                   (acc >> shift);
        }
    }

    temp31 = (temp >> 31) & 0x01;
    if (sign != temp31) {
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    return temp;
}

/*  128 bits long. p[0] is LO, p[1] is HI. */
static inline void mipsdsp_rndrashift_short_acc(CPUMIPSState *env,
                                                int64_t *p,
                                                int32_t ac,
                                                int32_t shift)
{
    int64_t acc;

    acc = ((int64_t)env->active_tc.HI[ac] << 32) |
          ((int64_t)env->active_tc.LO[ac] & 0xFFFFFFFF);
    if (shift == 0) {
        p[0] = acc << 1;
        p[1] = (acc >> 63) & 0x01;
    } else {
        p[0] = acc >> (shift - 1);
        p[1] = 0;
    }
}

/* 128 bits long. p[0] is LO, p[1] is HI */
static inline void mipsdsp_rashift_acc(CPUMIPSState *env,
                                       uint64_t *p,
                                       uint32_t ac,
                                       uint32_t shift)
{
    uint64_t tempB, tempA;

    tempB = env->active_tc.HI[ac];
    tempA = env->active_tc.LO[ac];
    shift = shift & 0x1F;

    if (shift == 0) {
        p[1] = tempB;
        p[0] = tempA;
    } else {
        p[0] = (tempB << (64 - shift)) | (tempA >> shift);
        p[1] = (int64_t)tempB >> shift;
    }
}

/* 128 bits long. p[0] is LO, p[1] is HI , p[2] is sign of HI.*/
static inline void mipsdsp_rndrashift_acc(CPUMIPSState *env,
                                       uint64_t *p,
                                       uint32_t ac,
                                       uint32_t shift)
{
    uint64_t tempB, tempA;

    tempB = env->active_tc.HI[ac];
    tempA = env->active_tc.LO[ac];
    shift = shift & 0x3F;

    if (shift == 0) {
        p[2] = tempB >> 63;
        p[1] = (tempB << 1) | (tempA >> 63);
        p[0] = tempA << 1;
    } else {
        p[0] = (tempB << (65 - shift)) | (tempA >> (shift - 1));
        p[1] = (int64_t)tempB >> (shift - 1);
        if (((tempB >> 63) & 0x01) == 1) {
            p[2] = ~0ull;
        } else {
            p[2] = 0x0;
        }
    }
}

static inline int32_t mipsdsp_mul_q15_q15(CPUMIPSState *env,
                                          int32_t ac, uint16_t a, uint16_t b)
{
    int32_t temp;

    if ((a == 0x8000) && (b == 0x8000)) {
        temp = 0x7FFFFFFF;
        set_DSPControl_overflow_flag(env, 1, 16 + ac);
    } else {
        temp = ((uint32_t)a * (uint32_t)b) << 1;
    }

    return temp;
}

static inline int64_t mipsdsp_mul_q31_q31(CPUMIPSState *env,
                                          int32_t ac, uint32_t a, uint32_t b)
{
    uint64_t temp;

    if ((a == 0x80000000) && (b == 0x80000000)) {
        temp = 0x7FFFFFFFFFFFFFFFull;
        set_DSPControl_overflow_flag(env, 1, 16 + ac);
    } else {
        temp = ((uint64_t)a * (uint64_t)b) << 1;
    }

    return temp;
}

static inline uint16_t mipsdsp_mul_u8_u8(uint8_t a, uint8_t b)
{
    return (uint16_t)a * (uint16_t)b;
}

static inline uint16_t mipsdsp_mul_u8_u16(CPUMIPSState *env,
                                          uint8_t a, uint16_t b)
{
    uint32_t tempI;

    tempI = (uint32_t)a * (uint32_t)b;
    if (tempI > 0x0000FFFF) {
        tempI = 0x0000FFFF;
        set_DSPControl_overflow_flag(env, 1, 21);
    }

    return tempI & 0x0000FFFF;
}

static inline uint64_t mipsdsp_mul_u32_u32(CPUMIPSState *env,
                                           uint32_t a, uint32_t b)
{
    return (uint64_t)a * (uint64_t)b;
}

static inline int16_t mipsdsp_rndq15_mul_q15_q15(CPUMIPSState *env,
                                                 uint16_t a, uint16_t b)
{
    uint32_t temp;

    if ((a == 0x8000) && (b == 0x8000)) {
        temp = 0x7FFF0000;
        set_DSPControl_overflow_flag(env, 1, 21);
    } else {
        temp = (a * b) << 1;
        temp = temp + 0x00008000;
    }

    return (temp & 0xFFFF0000) >> 16;
}

static inline int32_t mipsdsp_sat16_mul_q15_q15(CPUMIPSState *env,
                                                uint16_t a, uint16_t b)
{
    int32_t temp;

    if ((a == 0x8000) && (b == 0x8000)) {
        temp = 0x7FFF0000;
        set_DSPControl_overflow_flag(env, 1, 21);
    } else {
        temp = ((uint32_t)a * (uint32_t)b);
        temp = temp << 1;
    }

    return (temp >> 16) & 0x0000FFFF;
}

static inline uint16_t mipsdsp_trunc16_sat16_round(CPUMIPSState *env,
                                                   uint32_t a)
{
    uint32_t temp32, temp31;
    int64_t temp;

    temp = (int32_t)a + 0x00008000;
    temp32 = (temp >> 32) & 0x01;
    temp31 = (temp >> 31) & 0x01;

    if (temp32 != temp31) {
        temp = 0x7FFFFFFF;
        set_DSPControl_overflow_flag(env, 1, 22);
    }

    return (temp >> 16) & 0xFFFF;
}

static inline uint8_t mipsdsp_sat8_reduce_precision(CPUMIPSState *env,
                                                    uint16_t a)
{
    uint16_t mag;
    uint32_t sign;

    sign = (a >> 15) & 0x01;
    mag = a & 0x7FFF;

    if (sign == 0) {
        if (mag > 0x7F80) {
            set_DSPControl_overflow_flag(env, 1, 22);
            return 0xFF;
        } else {
            return (mag >> 7) & 0xFFFF;
        }
    } else {
        set_DSPControl_overflow_flag(env, 1, 22);
        return 0x00;
    }
}

static inline uint8_t mipsdsp_lshift8(CPUMIPSState *env, uint8_t a, uint8_t s)
{
    uint8_t sign;
    uint8_t discard;

    if (s == 0) {
        return a;
    } else {
        sign = (a >> 7) & 0x01;
        if (sign != 0) {
            discard = (((0x01 << (8 - s)) - 1) << s) |
                      ((a >> (6 - (s - 1))) & ((0x01 << s) - 1));
        } else {
            discard = a >> (6 - (s - 1));
        }

        if (discard != 0x00) {
            set_DSPControl_overflow_flag(env, 1, 22);
        }
        return a << s;
    }
}

static inline uint16_t mipsdsp_lshift16(CPUMIPSState *env,
                                        uint16_t a, uint8_t s)
{
    uint8_t  sign;
    uint16_t discard;

    if (s == 0) {
        return a;
    } else {
        sign = (a >> 15) & 0x01;
        if (sign != 0) {
            discard = (((0x01 << (16 - s)) - 1) << s) |
                      ((a >> (14 - (s - 1))) & ((0x01 << s) - 1));
        } else {
            discard = a >> (14 - (s - 1));
        }

        if ((discard != 0x0000) && (discard != 0xFFFF)) {
            set_DSPControl_overflow_flag(env, 1, 22);
        }
        return a << s;
    }
}


static inline uint32_t mipsdsp_lshift32(CPUMIPSState *env,
                                        uint32_t a, uint8_t s)
{
    uint32_t discard;

    if (s == 0) {
        return a;
    } else {
        discard = (int32_t)a >> (31 - (s - 1));

        if ((discard != 0x00000000) && (discard != 0xFFFFFFFF)) {
            set_DSPControl_overflow_flag(env, 1, 22);
        }
        return a << s;
    }
}

static inline uint16_t mipsdsp_sat16_lshift(CPUMIPSState *env,
                                            uint16_t a, uint8_t s)
{
    uint8_t  sign;
    uint16_t discard;

    if (s == 0) {
        return a;
    } else {
        sign = (a >> 15) & 0x01;
        if (sign != 0) {
            discard = (((0x01 << (16 - s)) - 1) << s) |
                      ((a >> (14 - (s - 1))) & ((0x01 << s) - 1));
        } else {
            discard = a >> (14 - (s - 1));
        }

        if ((discard != 0x0000) && (discard != 0xFFFF)) {
            set_DSPControl_overflow_flag(env, 1, 22);
            return (sign == 0) ? 0x7FFF : 0x8000;
        } else {
            return a << s;
        }
    }
}

static inline uint32_t mipsdsp_sat32_lshift(CPUMIPSState *env,
                                            uint32_t a, uint8_t s)
{
    uint8_t  sign;
    uint32_t discard;

    if (s == 0) {
        return a;
    } else {
        sign = (a >> 31) & 0x01;
        if (sign != 0) {
            discard = (((0x01 << (32 - s)) - 1) << s) |
                      ((a >> (30 - (s - 1))) & ((0x01 << s) - 1));
        } else {
            discard = a >> (30 - (s - 1));
        }

        if ((discard != 0x00000000) && (discard != 0xFFFFFFFF)) {
            set_DSPControl_overflow_flag(env, 1, 22);
            return (sign == 0) ? 0x7FFFFFFF : 0x80000000;
        } else {
            return a << s;
        }
    }
}

static inline uint16_t mipsdsp_rnd16_rashift(uint16_t a, uint8_t s)
{
    uint32_t temp;

    if (s == 0) {
        temp = (uint32_t)a << 1;
    } else {
        temp = (int32_t)(int16_t)a >> (s - 1);
    }

    return (temp + 1) >> 1;
}

static inline uint32_t mipsdsp_rnd32_rashift(uint32_t a, uint8_t s)
{
    int64_t temp;

    if (s == 0) {
        temp = a << 1;
    } else {
        temp = (int64_t)(int32_t)a >> (s - 1);
    }
    temp += 1;

    return (temp >> 1) & 0x00000000FFFFFFFFull;
}

static inline uint16_t mipsdsp_sub_i16(CPUMIPSState *env, int16_t a, int16_t b)
{
    uint8_t  temp16, temp15;
    int32_t  temp;

    temp = (int32_t)a - (int32_t)b;
    temp16 = (temp >> 16) & 0x01;
    temp15 = (temp >> 15) & 0x01;
    if (temp16 != temp15) {
        set_DSPControl_overflow_flag(env, 1, 20);
    }

    return temp & 0x0000FFFF;
}

static inline uint16_t mipsdsp_sat16_sub(CPUMIPSState *env,
                                         int16_t a, int16_t b)
{
    uint8_t  temp16, temp15;
    int32_t  temp;

    temp = (int32_t)a - (int32_t)b;
    temp16 = (temp >> 16) & 0x01;
    temp15 = (temp >> 15) & 0x01;
    if (temp16 != temp15) {
        if (temp16 == 0) {
            temp = 0x7FFF;
        } else {
            temp = 0x8000;
        }
        set_DSPControl_overflow_flag(env, 1, 20);
    }

    return temp & 0x0000FFFF;
}

static inline uint32_t mipsdsp_sat32_sub(CPUMIPSState *env,
                                         int32_t a, int32_t b)
{
    uint8_t  temp32, temp31;
    int64_t  temp;

    temp = (int64_t)a - (int64_t)b;
    temp32 = (temp >> 32) & 0x01;
    temp31 = (temp >> 31) & 0x01;
    if (temp32 != temp31) {
        if (temp32 == 0) {
            temp = 0x7FFFFFFF;
        } else {
            temp = 0x80000000;
        }
        set_DSPControl_overflow_flag(env, 1, 20);
    }

    return temp & 0x00000000FFFFFFFFull;
}

static inline uint16_t mipsdsp_rshift1_sub_q16(int16_t a, int16_t b)
{
    int32_t  temp;

    temp = (int32_t)a - (int32_t)b;

    return (temp >> 1) & 0x0000FFFF;
}

static inline uint16_t mipsdsp_rrshift1_sub_q16(int16_t a, int16_t b)
{
    int32_t  temp;

    temp = (int32_t)a - (int32_t)b;
    temp += 1;

    return (temp >> 1) & 0x0000FFFF;
}

static inline uint32_t mipsdsp_rshift1_sub_q32(int32_t a, int32_t b)
{
    int64_t  temp;

    temp = (int64_t)a - (int64_t)b;

    return (temp >> 1) & 0x00000000FFFFFFFFull;
}

static inline uint32_t mipsdsp_rrshift1_sub_q32(int32_t a, int32_t b)
{
    int64_t  temp;

    temp = (int64_t)a - (int64_t)b;
    temp += 1;

    return (temp >> 1) & 0x00000000FFFFFFFFull;
}

static inline uint16_t mipsdsp_sub_u16_u16(CPUMIPSState *env,
                                           uint16_t a, uint16_t b)
{
    uint8_t  temp16;
    uint32_t temp;

    temp = (uint32_t)a - (uint32_t)b;
    temp16 = (temp >> 16) & 0x01;
    if (temp16 == 1) {
        set_DSPControl_overflow_flag(env, 1, 20);
    }
    return temp & 0x0000FFFF;
}

static inline uint16_t mipsdsp_satu16_sub_u16_u16(CPUMIPSState *env,
                                                  uint16_t a, uint16_t b)
{
    uint8_t  temp16;
    uint32_t temp;

    temp   = (uint32_t)a - (uint32_t)b;
    temp16 = (temp >> 16) & 0x01;

    if (temp16 == 1) {
        temp = 0x0000;
        set_DSPControl_overflow_flag(env, 1, 20);
    }

    return temp & 0x0000FFFF;
}

static inline uint8_t mipsdsp_sub_u8(CPUMIPSState *env, uint8_t a, uint8_t b)
{
    uint8_t  temp8;
    uint16_t temp;

    temp = (uint16_t)a - (uint16_t)b;
    temp8 = (temp >> 8) & 0x01;
    if (temp8 == 1) {
        set_DSPControl_overflow_flag(env, 1, 20);
    }

    return temp & 0x00FF;
}

static inline uint8_t mipsdsp_satu8_sub(CPUMIPSState *env, uint8_t a, uint8_t b)
{
    uint8_t  temp8;
    uint16_t temp;

    temp = (uint16_t)a - (uint16_t)b;
    temp8 = (temp >> 8) & 0x01;
    if (temp8 == 1) {
        temp = 0x00;
        set_DSPControl_overflow_flag(env, 1, 20);
    }

    return temp & 0x00FF;
}

static inline uint32_t mipsdsp_sub32(CPUMIPSState *env, int32_t a, int32_t b)
{
    uint32_t temp32, temp31;
    int64_t temp;

    temp = (int64_t)a - (int64_t)b;
    temp32 = (temp >> 32) & 0x01;
    temp31 = (temp >> 31) & 0x01;
    if (temp32 != temp31) {
        set_DSPControl_overflow_flag(env, 1, 20);
    }

    return temp & 0xFFFFFFFF;
}

static inline int32_t mipsdsp_add_i32(CPUMIPSState *env, int32_t a, int32_t b)
{
    int32_t temp32, temp31;
    int64_t temp;

    temp = (int64_t)a + (int64_t)b;

    temp32 = (temp >> 32) & 0x01;
    temp31 = (temp >> 31) & 0x01;
    if (temp32 != temp31) {
        set_DSPControl_overflow_flag(env, 1, 20);
    }

    return temp & 0xFFFFFFFF;
}

static inline int32_t mipsdsp_cmp_eq(uint32_t a, uint32_t b)
{
    return a == b;
}

static inline int32_t mipsdsp_cmp_le(uint32_t a, uint32_t b)
{
    return a <= b;
}

static inline int32_t mipsdsp_cmp_lt(uint32_t a, uint32_t b)
{
    return a < b;
}
/*** MIPS DSP internal functions end ***/

#define MIPSDSP_LHI 0xFFFFFFFF00000000ull
#define MIPSDSP_LLO 0x00000000FFFFFFFFull
#define MIPSDSP_HI  0xFFFF0000
#define MIPSDSP_LO  0x0000FFFF
#define MIPSDSP_Q3  0xFF000000
#define MIPSDSP_Q2  0x00FF0000
#define MIPSDSP_Q1  0x0000FF00
#define MIPSDSP_Q0  0x000000FF

/** DSP Arithmetic Sub-class insns **/
target_ulong helper_addq_ph(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    int16_t  rsh, rsl, rth, rtl, temph, templ;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    temph = mipsdsp_add_i16(env, rsh, rth);
    templ = mipsdsp_add_i16(env, rsl, rtl);

    return (target_long)(int32_t)(((unsigned int)temph << 16) \
                                  | ((unsigned int)templ & 0xFFFF));
}

target_ulong helper_addq_s_ph(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    int16_t rsh, rsl, rth, rtl, temph, templ;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    temph = mipsdsp_sat_add_i16(env, rsh, rth);
    templ = mipsdsp_sat_add_i16(env, rsl, rtl);

    return (target_long)(int32_t)(((uint32_t)temph << 16) \
                                  | ((uint32_t)templ & 0xFFFF));
}

#if defined(TARGET_MIPS64)
target_ulong helper_addq_qh(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint16_t rs3, rs2, rs1, rs0;
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_add_i16(env, rs3, rt3);
    tempC = mipsdsp_add_i16(env, rs2, rt2);
    tempB = mipsdsp_add_i16(env, rs1, rt1);
    tempA = mipsdsp_add_i16(env, rs0, rt0);

    return ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
        ((uint64_t)tempB << 16) | (uint64_t)tempA;
}

target_ulong helper_addq_s_qh(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    uint16_t rs3, rs2, rs1, rs0;
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_sat_add_i16(env, rs3, rt3);
    tempC = mipsdsp_sat_add_i16(env, rs2, rt2);
    tempB = mipsdsp_sat_add_i16(env, rs1, rt1);
    tempA = mipsdsp_sat_add_i16(env, rs0, rt0);

    return ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
        ((uint64_t)tempB << 16) | (uint64_t)tempA;
}
#endif

target_ulong helper_addq_s_w(CPUMIPSState *env,
                             target_ulong rs, target_ulong rt)
{
    uint32_t rd;
    rd = mipsdsp_sat_add_i32(env, rs, rt);
    return (target_long)(int32_t)rd;
}

#if defined(TARGET_MIPS64)
target_ulong helper_addq_pw(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint32_t rs1, rs0;
    uint32_t rt1, rt0;
    uint32_t tempB, tempA;

    rs1 = (rs >> 32) & MIPSDSP_LLO;
    rs0 = rs & MIPSDSP_LLO;
    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB = mipsdsp_add_i32(env, rs1, rt1);
    tempA = mipsdsp_add_i32(env, rs0, rt0);

    return ((uint64_t)tempB << 32) | (uint64_t)tempA;
}

target_ulong helper_addq_s_pw(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    uint32_t rs1, rs0;
    uint32_t rt1, rt0;
    uint32_t tempB, tempA;

    rs1 = (rs >> 32) & MIPSDSP_LLO;
    rs0 = rs & MIPSDSP_LLO;
    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB = mipsdsp_sat_add_i32(env, rs1, rt1);
    tempA = mipsdsp_sat_add_i32(env, rs0, rt0);

    return ((uint64_t)tempB << 32) | (uint64_t)tempA;
}
#endif

target_ulong helper_addu_qb(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint32_t rd;
    uint8_t  rs0, rs1, rs2, rs3;
    uint8_t  rt0, rt1, rt2, rt3;
    uint8_t  temp0, temp1, temp2, temp3;

    rs0 =  rs & MIPSDSP_Q0;
    rs1 = (rs & MIPSDSP_Q1) >>  8;
    rs2 = (rs & MIPSDSP_Q2) >> 16;
    rs3 = (rs & MIPSDSP_Q3) >> 24;

    rt0 =  rt & MIPSDSP_Q0;
    rt1 = (rt & MIPSDSP_Q1) >>  8;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt3 = (rt & MIPSDSP_Q3) >> 24;

    temp0 = mipsdsp_add_u8(env, rs0, rt0);
    temp1 = mipsdsp_add_u8(env, rs1, rt1);
    temp2 = mipsdsp_add_u8(env, rs2, rt2);
    temp3 = mipsdsp_add_u8(env, rs3, rt3);

    rd = (((uint32_t)temp3 << 24) & MIPSDSP_Q3) |
         (((uint32_t)temp2 << 16) & MIPSDSP_Q2) |
         (((uint32_t)temp1 <<  8) & MIPSDSP_Q1) |
         ((uint32_t)temp0         & MIPSDSP_Q0);

    return (target_long)(int32_t)rd;
}

target_ulong helper_addu_s_qb(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    uint32_t rd;
    uint8_t rs0, rs1, rs2, rs3;
    uint8_t rt0, rt1, rt2, rt3;
    uint8_t temp0, temp1, temp2, temp3;

    rs0 =  rs & MIPSDSP_Q0;
    rs1 = (rs & MIPSDSP_Q1) >>  8;
    rs2 = (rs & MIPSDSP_Q2) >> 16;
    rs3 = (rs & MIPSDSP_Q3) >> 24;

    rt0 =  rt & MIPSDSP_Q0;
    rt1 = (rt & MIPSDSP_Q1) >>  8;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt3 = (rt & MIPSDSP_Q3) >> 24;

    temp0 = mipsdsp_sat_add_u8(env, rs0, rt0);
    temp1 = mipsdsp_sat_add_u8(env, rs1, rt1);
    temp2 = mipsdsp_sat_add_u8(env, rs2, rt2);
    temp3 = mipsdsp_sat_add_u8(env, rs3, rt3);

    rd = (((uint8_t)temp3 << 24) & MIPSDSP_Q3) |
         (((uint8_t)temp2 << 16) & MIPSDSP_Q2) |
         (((uint8_t)temp1 <<  8) & MIPSDSP_Q1) |
         ((uint8_t)temp0         & MIPSDSP_Q0);

    return (target_long)(int32_t)rd;
}

target_ulong helper_adduh_qb(target_ulong rs, target_ulong rt)
{
    uint32_t rd;
    uint8_t  rs0, rs1, rs2, rs3;
    uint8_t  rt0, rt1, rt2, rt3;
    uint8_t  temp0, temp1, temp2, temp3;

    rs0 =  rs & MIPSDSP_Q0;
    rs1 = (rs & MIPSDSP_Q1) >>  8;
    rs2 = (rs & MIPSDSP_Q2) >> 16;
    rs3 = (rs & MIPSDSP_Q3) >> 24;

    rt0 =  rt & MIPSDSP_Q0;
    rt1 = (rt & MIPSDSP_Q1) >>  8;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt3 = (rt & MIPSDSP_Q3) >> 24;

    temp0 = mipsdsp_rshift1_add_u8(rs0, rt0);
    temp1 = mipsdsp_rshift1_add_u8(rs1, rt1);
    temp2 = mipsdsp_rshift1_add_u8(rs2, rt2);
    temp3 = mipsdsp_rshift1_add_u8(rs3, rt3);

    rd = (((uint32_t)temp3 << 24) & MIPSDSP_Q3) |
         (((uint32_t)temp2 << 16) & MIPSDSP_Q2) |
         (((uint32_t)temp1 <<  8) & MIPSDSP_Q1) |
         ((uint32_t)temp0         & MIPSDSP_Q0);

    return (target_long)(int32_t)rd;
}

target_ulong helper_adduh_r_qb(target_ulong rs, target_ulong rt)
{
    uint32_t rd;
    uint8_t  rs0, rs1, rs2, rs3;
    uint8_t  rt0, rt1, rt2, rt3;
    uint8_t  temp0, temp1, temp2, temp3;

    rs0 =  rs & MIPSDSP_Q0;
    rs1 = (rs & MIPSDSP_Q1) >>  8;
    rs2 = (rs & MIPSDSP_Q2) >> 16;
    rs3 = (rs & MIPSDSP_Q3) >> 24;

    rt0 =  rt & MIPSDSP_Q0;
    rt1 = (rt & MIPSDSP_Q1) >>  8;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt3 = (rt & MIPSDSP_Q3) >> 24;

    temp0 = mipsdsp_rrshift1_add_u8(rs0, rt0);
    temp1 = mipsdsp_rrshift1_add_u8(rs1, rt1);
    temp2 = mipsdsp_rrshift1_add_u8(rs2, rt2);
    temp3 = mipsdsp_rrshift1_add_u8(rs3, rt3);

    rd = (((uint32_t)temp3 << 24) & MIPSDSP_Q3) |
         (((uint32_t)temp2 << 16) & MIPSDSP_Q2) |
         (((uint32_t)temp1 <<  8) & MIPSDSP_Q1) |
         ((uint32_t)temp0         & MIPSDSP_Q0);

    return (target_long)(int32_t)rd;
}

target_ulong helper_addu_ph(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint16_t rsh, rsl, rth, rtl, temph, templ;
    uint32_t rd;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;
    temph = mipsdsp_add_u16(env, rsh, rth);
    templ = mipsdsp_add_u16(env, rsl, rtl);
    rd = ((uint32_t)temph << 16) | ((uint32_t)templ & MIPSDSP_LO);

    return (target_long)(int32_t)rd;
}

target_ulong helper_addu_s_ph(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    uint16_t rsh, rsl, rth, rtl, temph, templ;
    uint32_t rd;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;
    temph = mipsdsp_sat_add_u16(env, rsh, rth);
    templ = mipsdsp_sat_add_u16(env, rsl, rtl);
    rd = ((uint32_t)temph << 16) | ((uint32_t)templ & MIPSDSP_LO);

    return (target_long)(int32_t)rd;
}

target_ulong helper_addqh_ph(target_ulong rs, target_ulong rt)
{
    uint32_t rd;
    int16_t rsh, rsl, rth, rtl, temph, templ;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    temph = mipsdsp_rshift1_add_q16(rsh, rth);
    templ = mipsdsp_rshift1_add_q16(rsl, rtl);
    rd = ((uint32_t)temph << 16) | ((uint32_t)templ & MIPSDSP_LO);

    return (target_long)(int32_t)rd;
}

target_ulong helper_addqh_r_ph(target_ulong rs, target_ulong rt)
{
    uint32_t rd;
    int16_t rsh, rsl, rth, rtl, temph, templ;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    temph = mipsdsp_rrshift1_add_q16(rsh, rth);
    templ = mipsdsp_rrshift1_add_q16(rsl, rtl);
    rd = ((uint32_t)temph << 16) | ((uint32_t)templ & MIPSDSP_LO);

    return (target_long)(int32_t)rd;
}

target_ulong helper_addqh_w(target_ulong rs, target_ulong rt)
{
    uint32_t rd;

    rd = mipsdsp_rshift1_add_q32(rs, rt);

    return (target_long)(int32_t)rd;
}

target_ulong helper_addqh_r_w(target_ulong rs, target_ulong rt)
{
    uint32_t rd;

    rd = mipsdsp_rrshift1_add_q32(rs, rt);

    return (target_long)(int32_t)rd;
}

#if defined(TARGET_MIPS64)
target_ulong helper_addu_ob(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    int i;
    uint8_t rs_t[8], rt_t[8];
    uint8_t temp[8];
    uint64_t result;

    result = 0;

    for (i = 0; i < 8; i++) {
        rs_t[i] = (rs >> (8 * i)) & MIPSDSP_Q0;
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;
        temp[i] = mipsdsp_add_u8(env, rs_t[i], rt_t[i]);
    }

    for (i = 0; i < 8; i++) {
        result |= (uint64_t)temp[i] << (8 * i);
    }

    return result;
}

target_ulong helper_addu_s_ob(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    int i;
    uint8_t rs_t[8], rt_t[8];
    uint8_t temp[8];
    uint64_t result;

    result = 0;

    for (i = 0; i < 8; i++) {
        rs_t[i] = (rs >> (8 * i)) & MIPSDSP_Q0;
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;
        temp[i] = mipsdsp_sat_add_u8(env, rs_t[i], rt_t[i]);
    }

    for (i = 0; i < 8; i++) {
        result |= (uint64_t)temp[i] << (8 * i);
    }

    return result;
}

target_ulong helper_adduh_ob(target_ulong rs, target_ulong rt)
{
    int i;
    uint8_t rs_t[8], rt_t[8];
    uint8_t temp[8];
    uint64_t result;

    result = 0;

    for (i = 0; i < 8; i++) {
        rs_t[i] = (rs >> (8 * i)) & MIPSDSP_Q0;
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;
        temp[i] = mipsdsp_rshift1_add_u8(rs_t[i], rt_t[i]);
    }

    for (i = 0; i < 8; i++) {
        result |= (uint64_t)temp[i] << (8 * i);
    }

    return result;
}

target_ulong helper_adduh_r_ob(target_ulong rs, target_ulong rt)
{
    int i;
    uint8_t rs_t[8], rt_t[8];
    uint8_t temp[8];
    uint64_t result;

    result = 0;

    for (i = 0; i < 8; i++) {
        rs_t[i] = (rs >> (8 * i)) & MIPSDSP_Q0;
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;
        temp[i] = mipsdsp_rrshift1_add_u8(rs_t[i], rt_t[i]);
    }

    for (i = 0; i < 8; i++) {
        result |= (uint64_t)temp[i] << (8 * i);
    }

    return result;
}

target_ulong helper_addu_qh(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint16_t rs3, rs2, rs1, rs0;
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_add_u16(env, rs3, rt3);
    tempC = mipsdsp_add_u16(env, rs2, rt2);
    tempB = mipsdsp_add_u16(env, rs1, rt1);
    tempA = mipsdsp_add_u16(env, rs0, rt0);

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
           ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return temp;
}

target_ulong helper_addu_s_qh(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    uint16_t rs3, rs2, rs1, rs0;
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_sat_add_u16(env, rs3, rt3);
    tempC = mipsdsp_sat_add_u16(env, rs2, rt2);
    tempB = mipsdsp_sat_add_u16(env, rs1, rt1);
    tempA = mipsdsp_sat_add_u16(env, rs0, rt0);

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
           ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return temp;
}
#endif

target_ulong helper_subq_ph(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint16_t rsh, rsl;
    uint16_t rth, rtl;
    uint16_t tempB, tempA;
    uint32_t rd;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB = mipsdsp_sub_i16(env, rsh, rth);
    tempA = mipsdsp_sub_i16(env, rsl, rtl);
    rd = ((uint32_t)tempB << 16) | (uint32_t)tempA;

    return (target_long)(int32_t)rd;
}

target_ulong helper_subq_s_ph(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    uint16_t rsh, rsl;
    uint16_t rth, rtl;
    uint16_t tempB, tempA;
    uint32_t rd;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB = mipsdsp_sat16_sub(env, rsh, rth);
    tempA = mipsdsp_sat16_sub(env, rsl, rtl);
    rd = ((uint32_t)tempB << 16) | (uint32_t)tempA;

    return (target_long)(int32_t)rd;
}

#if defined(TARGET_MIPS64)
target_ulong helper_subq_qh(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint16_t rs3, rs2, rs1, rs0;
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_sub_i16(env, rs3, rt3);
    tempC = mipsdsp_sub_i16(env, rs2, rt2);
    tempB = mipsdsp_sub_i16(env, rs1, rt1);
    tempA = mipsdsp_sub_i16(env, rs0, rt0);

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
      ((uint64_t)tempB << 16) | (uint64_t)tempA;
    return temp;
}

target_ulong helper_subq_s_qh(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    uint16_t rs3, rs2, rs1, rs0;
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;
    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_sat16_sub(env, rs3, rt3);
    tempC = mipsdsp_sat16_sub(env, rs2, rt2);
    tempB = mipsdsp_sat16_sub(env, rs1, rt1);
    tempA = mipsdsp_sat16_sub(env, rs0, rt0);

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
           ((uint64_t)tempB << 16) | (uint64_t)tempA;
    return temp;
}
#endif

target_ulong helper_subq_s_w(CPUMIPSState *env,
                             target_ulong rs, target_ulong rt)
{
    uint32_t rd;

    rd = mipsdsp_sat32_sub(env, rs, rt);

    return (target_long)(int32_t)rd;
}

#if defined(TARGET_MIPS64)
target_ulong helper_subq_pw(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint32_t rs1, rs0;
    uint32_t rt1, rt0;
    uint32_t tempB, tempA;

    rs1 = (rs >> 32) & MIPSDSP_LLO;
    rs0 = rs & MIPSDSP_LLO;
    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB = mipsdsp_sub32(env, rs1, rt1);
    tempA = mipsdsp_sub32(env, rs0, rt0);

    return ((uint64_t)tempB << 32) | (uint64_t)tempA;
}

target_ulong helper_subq_s_pw(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    uint32_t rs1, rs0;
    uint32_t rt1, rt0;
    uint32_t tempB, tempA;

    rs1 = (rs >> 32) & MIPSDSP_LLO;
    rs0 = rs & MIPSDSP_LLO;
    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB = mipsdsp_sat32_sub(env, rs1, rt1);
    tempA = mipsdsp_sat32_sub(env, rs0, rt0);

    return ((uint64_t)tempB << 32) | (uint64_t)tempA;
}
#endif

target_ulong helper_subu_qb(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint8_t rs3, rs2, rs1, rs0;
    uint8_t rt3, rt2, rt1, rt0;
    uint8_t tempD, tempC, tempB, tempA;
    uint32_t rd;

    rs3 = (rs & MIPSDSP_Q3) >> 24;
    rs2 = (rs & MIPSDSP_Q2) >> 16;
    rs1 = (rs & MIPSDSP_Q1) >>  8;
    rs0 =  rs & MIPSDSP_Q0;

    rt3 = (rt & MIPSDSP_Q3) >> 24;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt1 = (rt & MIPSDSP_Q1) >>  8;
    rt0 =  rt & MIPSDSP_Q0;

    tempD = mipsdsp_sub_u8(env, rs3, rt3);
    tempC = mipsdsp_sub_u8(env, rs2, rt2);
    tempB = mipsdsp_sub_u8(env, rs1, rt1);
    tempA = mipsdsp_sub_u8(env, rs0, rt0);

    rd = ((uint32_t)tempD << 24) | ((uint32_t)tempC << 16) |
         ((uint32_t)tempB <<  8) | (uint32_t)tempA;
    return (target_long)(int32_t)rd;
}

target_ulong helper_subu_s_qb(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    uint8_t rs3, rs2, rs1, rs0;
    uint8_t rt3, rt2, rt1, rt0;
    uint8_t tempD, tempC, tempB, tempA;
    uint32_t rd;

    rs3 = (rs & MIPSDSP_Q3) >> 24;
    rs2 = (rs & MIPSDSP_Q2) >> 16;
    rs1 = (rs & MIPSDSP_Q1) >>  8;
    rs0 =  rs & MIPSDSP_Q0;

    rt3 = (rt & MIPSDSP_Q3) >> 24;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt1 = (rt & MIPSDSP_Q1) >>  8;
    rt0 =  rt & MIPSDSP_Q0;

    tempD = mipsdsp_satu8_sub(env, rs3, rt3);
    tempC = mipsdsp_satu8_sub(env, rs2, rt2);
    tempB = mipsdsp_satu8_sub(env, rs1, rt1);
    tempA = mipsdsp_satu8_sub(env, rs0, rt0);

    rd = ((uint32_t)tempD << 24) | ((uint32_t)tempC << 16) |
         ((uint32_t)tempB <<  8) | (uint32_t)tempA;

    return (target_long)(int32_t)rd;
}

target_ulong helper_subuh_qb(target_ulong rs, target_ulong rt)
{
    uint8_t rs3, rs2, rs1, rs0;
    uint8_t rt3, rt2, rt1, rt0;
    uint8_t tempD, tempC, tempB, tempA;
    uint32_t rd;

    rs3 = (rs & MIPSDSP_Q3) >> 24;
    rs2 = (rs & MIPSDSP_Q2) >> 16;
    rs1 = (rs & MIPSDSP_Q1) >>  8;
    rs0 =  rs & MIPSDSP_Q0;

    rt3 = (rt & MIPSDSP_Q3) >> 24;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt1 = (rt & MIPSDSP_Q1) >>  8;
    rt0 =  rt & MIPSDSP_Q0;

    tempD = ((uint16_t)rs3 - (uint16_t)rt3) >> 1;
    tempC = ((uint16_t)rs2 - (uint16_t)rt2) >> 1;
    tempB = ((uint16_t)rs1 - (uint16_t)rt1) >> 1;
    tempA = ((uint16_t)rs0 - (uint16_t)rt0) >> 1;

    rd = ((uint32_t)tempD << 24) | ((uint32_t)tempC << 16) |
         ((uint32_t)tempB <<  8) | (uint32_t)tempA;

    return (target_ulong)rd;
}

target_ulong helper_subuh_r_qb(target_ulong rs, target_ulong rt)
{
    uint8_t rs3, rs2, rs1, rs0;
    uint8_t rt3, rt2, rt1, rt0;
    uint8_t tempD, tempC, tempB, tempA;
    uint32_t rd;

    rs3 = (rs & MIPSDSP_Q3) >> 24;
    rs2 = (rs & MIPSDSP_Q2) >> 16;
    rs1 = (rs & MIPSDSP_Q1) >>  8;
    rs0 =  rs & MIPSDSP_Q0;

    rt3 = (rt & MIPSDSP_Q3) >> 24;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt1 = (rt & MIPSDSP_Q1) >>  8;
    rt0 =  rt & MIPSDSP_Q0;

    tempD = ((uint16_t)rs3 - (uint16_t)rt3 + 1) >> 1;
    tempC = ((uint16_t)rs2 - (uint16_t)rt2 + 1) >> 1;
    tempB = ((uint16_t)rs1 - (uint16_t)rt1 + 1) >> 1;
    tempA = ((uint16_t)rs0 - (uint16_t)rt0 + 1) >> 1;

    rd = ((uint32_t)tempD << 24) | ((uint32_t)tempC << 16) |
         ((uint32_t)tempB <<  8) | (uint32_t)tempA;

    return (target_ulong)rd;
}

target_ulong helper_subu_ph(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint16_t rsh, rsl, rth, rtl;
    uint16_t tempB, tempA;
    uint32_t rd;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB = mipsdsp_sub_u16_u16(env, rth, rsh);
    tempA = mipsdsp_sub_u16_u16(env, rtl, rsl);
    rd = ((uint32_t)tempB << 16) | (uint32_t)tempA;
    return (target_long)(int32_t)rd;
}

target_ulong helper_subu_s_ph(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    uint16_t rsh, rsl, rth, rtl;
    uint16_t tempB, tempA;
    uint32_t rd;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB = mipsdsp_satu16_sub_u16_u16(env, rth, rsh);
    tempA = mipsdsp_satu16_sub_u16_u16(env, rtl, rsl);
    rd = ((uint32_t)tempB << 16) | (uint32_t)tempA;

    return (target_long)(int32_t)rd;
}

target_ulong helper_subqh_ph(target_ulong rs, target_ulong rt)
{
    uint16_t rsh, rsl;
    uint16_t rth, rtl;
    uint16_t tempB, tempA;
    uint32_t rd;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;
    tempB = mipsdsp_rshift1_sub_q16(rsh, rth);
    tempA = mipsdsp_rshift1_sub_q16(rsl, rtl);
    rd = ((uint32_t)tempB << 16) | (uint32_t)tempA;

    return (target_long)(int32_t)rd;
}

target_ulong helper_subqh_r_ph(target_ulong rs, target_ulong rt)
{
    uint16_t rsh, rsl;
    uint16_t rth, rtl;
    uint16_t tempB, tempA;
    uint32_t rd;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;
    tempB = mipsdsp_rrshift1_sub_q16(rsh, rth);
    tempA = mipsdsp_rrshift1_sub_q16(rsl, rtl);
    rd = ((uint32_t)tempB << 16) | (uint32_t)tempA;

    return (target_long)(int32_t)rd;
}

target_ulong helper_subqh_w(target_ulong rs, target_ulong rt)
{
    uint32_t rd;

    rd = mipsdsp_rshift1_sub_q32(rs, rt);

    return (target_long)(int32_t)rd;
}

target_ulong helper_subqh_r_w(target_ulong rs, target_ulong rt)
{
    uint32_t rd;

    rd = mipsdsp_rrshift1_sub_q32(rs, rt);

    return (target_long)(int32_t)rd;
}

#if defined(TARGET_MIPS64)
target_ulong helper_subu_ob(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    int i;
    uint8_t rs_t[8], rt_t[8];
    uint8_t temp[8];
    uint64_t result;

    result = 0;

    for (i = 0; i < 8; i++) {
        rs_t[i] = (rs >> (8 * i)) & MIPSDSP_Q0;
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;
        temp[i] = mipsdsp_sub_u8(env, rs_t[i], rt_t[i]);
    }

    for (i = 0; i < 8; i++) {
        result += (uint64_t)temp[i] << (i * 8);
    }

    return result;
}

target_ulong helper_subu_s_ob(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    int i;
    uint8_t rs_t[8], rt_t[8];
    uint8_t temp[8];
    uint64_t result;

    result = 0;

    for (i = 0; i < 8; i++) {
        rs_t[i] = (rs >> (8 * i)) & MIPSDSP_Q0;
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;
        temp[i] = mipsdsp_satu8_sub(env, rs_t[i], rt_t[i]);
    }

    for (i = 0; i < 8; i++) {
        result += (uint64_t)temp[i] << (i * 8);
    }

    return result;
}

target_ulong helper_subuh_ob(target_ulong rs, target_ulong rt)
{
    int i;
    uint8_t rs_t[8], rt_t[8];
    uint8_t temp[8];
    uint64_t result;

    result = 0;

    for (i = 0; i < 8; i++) {
        rs_t[i] = (rs >> (8 * i)) & MIPSDSP_Q0;
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;
        temp[i] = ((uint16_t)rs_t[i] - (uint16_t)rt_t[i]) >> 1;
      }

    for (i = 0; i < 8; i++) {
        result |= (uint64_t)temp[i] << (8 * i);
    }

    return result;
}

target_ulong helper_subuh_r_ob(target_ulong rs, target_ulong rt)
{
    int i;
    uint8_t rs_t[8], rt_t[8];
    uint8_t temp[8];
    uint64_t result;

    result = 0;

    for (i = 0; i < 8; i++) {
        rs_t[i] = (rs >> (8 * i)) & MIPSDSP_Q0;
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;
        temp[i] = ((uint16_t)rs_t[i] - (uint16_t)rt_t[i] + 1) >> 1;
    }

    for (i = 0; i < 8; i++) {
        result |= (uint64_t)temp[i] << (8 * i);
    }

    return result;
}

target_ulong helper_subu_qh(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint16_t rs3, rs2, rs1, rs0;
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_sub_u16_u16(env, rs3, rt3);
    tempC = mipsdsp_sub_u16_u16(env, rs2, rt2);
    tempB = mipsdsp_sub_u16_u16(env, rs1, rt1);
    tempA = mipsdsp_sub_u16_u16(env, rs0, rt0);

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
      ((uint64_t)tempB << 16) | (uint64_t)tempA;
    return temp;
}


target_ulong helper_subu_s_qh(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    uint16_t rs3, rs2, rs1, rs0;
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_satu16_sub_u16_u16(env, rs3, rt3);
    tempC = mipsdsp_satu16_sub_u16_u16(env, rs2, rt2);
    tempB = mipsdsp_satu16_sub_u16_u16(env, rs1, rt1);
    tempA = mipsdsp_satu16_sub_u16_u16(env, rs0, rt0);

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
           ((uint64_t)tempB << 16) | (uint64_t)tempA;
    return temp;
}
#endif

target_ulong helper_addsc(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint64_t temp, tempRs, tempRt;
    int32_t flag;

    tempRs = (uint64_t)rs & MIPSDSP_LLO;
    tempRt = (uint64_t)rt & MIPSDSP_LLO;

    temp = tempRs + tempRt;
    flag = (temp & 0x0100000000ull) >> 32;
    set_DSPControl_carryflag(env, flag);

    return (target_long)(int32_t)(temp & MIPSDSP_LLO);
}

target_ulong helper_addwc(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint32_t rd;
    int32_t temp32, temp31;
    int64_t tempL;

    tempL = (int32_t)rs + (int32_t)rt + get_DSPControl_carryflag(env);
    temp31 = (tempL >> 31) & 0x01;
    temp32 = (tempL >> 32) & 0x01;

    if (temp31 != temp32) {
        set_DSPControl_overflow_flag(env, 1, 20);
    }

    rd = tempL & MIPSDSP_LLO;

    return (target_long)(int32_t)rd;
}

target_ulong helper_modsub(target_ulong rs, target_ulong rt)
{
    int32_t decr;
    uint16_t lastindex;
    target_ulong rd;

    decr = rt & MIPSDSP_Q0;
    lastindex = (rt >> 8) & MIPSDSP_LO;

    if ((rs & MIPSDSP_LLO) == 0x00000000) {
        rd = (target_ulong)lastindex;
    } else {
        rd = rs - decr;
    }

    return rd;
}

target_ulong helper_raddu_w_qb(target_ulong rs)
{
    uint8_t  rs3, rs2, rs1, rs0;
    uint16_t temp;
    uint32_t rd;

    rs3 = (rs & MIPSDSP_Q3) >> 24;
    rs2 = (rs & MIPSDSP_Q2) >> 16;
    rs1 = (rs & MIPSDSP_Q1) >>  8;
    rs0 =  rs & MIPSDSP_Q0;

    temp = (uint16_t)rs3 + (uint16_t)rs2 + (uint16_t)rs1 + (uint16_t)rs0;
    rd = temp;

    return (target_ulong)rd;
}

#if defined(TARGET_MIPS64)
target_ulong helper_raddu_l_ob(target_ulong rs)
{
    int i;
    uint16_t rs_t[8];
    uint64_t temp;

    temp = 0;

    for (i = 0; i < 8; i++) {
        rs_t[i] = (rs >> (8 * i)) & MIPSDSP_Q0;
        temp += (uint64_t)rs_t[i];
    }

    return temp;
}
#endif

target_ulong helper_absq_s_qb(CPUMIPSState *env, target_ulong rt)
{
    uint32_t rd;
    int8_t tempD, tempC, tempB, tempA;

    tempD = (rt & MIPSDSP_Q3) >> 24;
    tempC = (rt & MIPSDSP_Q2) >> 16;
    tempB = (rt & MIPSDSP_Q1) >>  8;
    tempA =  rt & MIPSDSP_Q0;

    rd = (((uint32_t)mipsdsp_sat_abs_u8 (env, tempD) << 24) & MIPSDSP_Q3) |
         (((uint32_t)mipsdsp_sat_abs_u8 (env, tempC) << 16) & MIPSDSP_Q2) |
         (((uint32_t)mipsdsp_sat_abs_u8 (env, tempB) <<  8) & MIPSDSP_Q1) |
         ((uint32_t)mipsdsp_sat_abs_u8 (env, tempA) & MIPSDSP_Q0);

    return (target_ulong)rd;
}

target_ulong helper_absq_s_ph(CPUMIPSState *env, target_ulong rt)
{
    uint32_t rd;
    int16_t tempA, tempB;

    tempA = (rt & MIPSDSP_HI) >> 16;
    tempB =  rt & MIPSDSP_LO;

    rd = ((uint32_t)(uint16_t)mipsdsp_sat_abs_u16 (env, tempA) << 16) |
        ((uint32_t)((uint16_t)mipsdsp_sat_abs_u16 (env, tempB)) & 0xFFFF);

    return (target_long)(int32_t)rd;
}

target_ulong helper_absq_s_w(CPUMIPSState *env, target_ulong rt)
{
    uint32_t rd;
    int32_t temp;

    temp = rt;
    rd = mipsdsp_sat_abs_u32(env, temp);

    return (target_ulong)rd;
}

#if defined(TARGET_MIPS64)
target_ulong helper_absq_s_ob(CPUMIPSState *env, target_ulong rt)
{
    int8_t tempH, tempG, tempF, tempE;
    int8_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    tempH = (rt >> 56) & MIPSDSP_Q0;
    tempG = (rt >> 48) & MIPSDSP_Q0;
    tempF = (rt >> 40) & MIPSDSP_Q0;
    tempE = (rt >> 32) & MIPSDSP_Q0;
    tempD = (rt >> 24) & MIPSDSP_Q0;
    tempC = (rt >> 16) & MIPSDSP_Q0;
    tempB = (rt >> 8) & MIPSDSP_Q0;
    tempA = rt & MIPSDSP_Q0;

    tempH = mipsdsp_sat_abs_u8(env, tempH);
    tempG = mipsdsp_sat_abs_u8(env, tempG);
    tempF = mipsdsp_sat_abs_u8(env, tempF);
    tempE = mipsdsp_sat_abs_u8(env, tempE);
    tempD = mipsdsp_sat_abs_u8(env, tempD);
    tempC = mipsdsp_sat_abs_u8(env, tempC);
    tempB = mipsdsp_sat_abs_u8(env, tempB);
    tempA = mipsdsp_sat_abs_u8(env, tempA);

    temp = ((uint64_t)(uint8_t)tempH << 56) | ((uint64_t)(uint8_t)tempG << 48) |
        ((uint64_t)(uint8_t)tempF << 40) | ((uint64_t)(uint8_t)tempE << 32) |
        ((uint64_t)(uint8_t)tempD << 24) | ((uint64_t)(uint8_t)tempC << 16) |
        ((uint64_t)(uint8_t)tempB << 8) | (uint64_t)(uint8_t)tempA;

    return temp;
}

target_ulong helper_absq_s_qh(CPUMIPSState *env, target_ulong rt)
{
    int16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    tempD = (rt >> 48) & MIPSDSP_LO;
    tempC = (rt >> 32) & MIPSDSP_LO;
    tempB = (rt >> 16) & MIPSDSP_LO;
    tempA = rt & MIPSDSP_LO;

    tempD = mipsdsp_sat_abs_u16(env, tempD);
    tempC = mipsdsp_sat_abs_u16(env, tempC);
    tempB = mipsdsp_sat_abs_u16(env, tempB);
    tempA = mipsdsp_sat_abs_u16(env, tempA);

    temp = ((uint64_t)(uint16_t)tempD << 48) | \
           ((uint64_t)(uint16_t)tempC << 32) | \
           ((uint64_t)(uint16_t)tempB << 16) | \
           (uint64_t)(uint16_t)tempA;

    return temp;
}

target_ulong helper_absq_s_pw(CPUMIPSState *env, target_ulong rt)
{
    int32_t tempB, tempA;
    uint64_t temp;

    tempB = (rt >> 32) & MIPSDSP_LLO;
    tempA = rt & MIPSDSP_LLO;

    tempB = mipsdsp_sat_abs_u32(env, tempB);
    tempA = mipsdsp_sat_abs_u32(env, tempA);

    temp = ((uint64_t)(uint32_t)tempB << 32) | (uint64_t)(uint32_t)tempA;

    return temp;
}
#endif

target_ulong helper_precr_qb_ph(target_ulong rs, target_ulong rt)
{
    uint8_t  rs2, rs0, rt2, rt0;
    uint32_t rd;

    rs2 = (rs & MIPSDSP_Q2) >> 16;
    rs0 =  rs & MIPSDSP_Q0;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt0 =  rt & MIPSDSP_Q0;
    rd = ((uint32_t)rs2 << 24) | ((uint32_t)rs0 << 16) |
         ((uint32_t)rt2 <<  8) | (uint32_t)rt0;

    return (target_long)(int32_t)rd;
}

target_ulong helper_precrq_qb_ph(target_ulong rs, target_ulong rt)
{
    uint8_t tempD, tempC, tempB, tempA;
    uint32_t rd;

    tempD = (rs & MIPSDSP_Q3) >> 24;
    tempC = (rs & MIPSDSP_Q1) >>  8;
    tempB = (rt & MIPSDSP_Q3) >> 24;
    tempA = (rt & MIPSDSP_Q1) >>  8;

    rd = ((uint32_t)tempD << 24) | ((uint32_t)tempC << 16) |
         ((uint32_t)tempB <<  8) | (uint32_t)tempA;

    return (target_long)(int32_t)rd;
}

target_ulong helper_precr_sra_ph_w(uint32_t sa, target_ulong rs,
                                   target_ulong rt)
{
    uint16_t tempB, tempA;

    tempB = ((int32_t)rt >> sa) & MIPSDSP_LO;
    tempA = ((int32_t)rs >> sa) & MIPSDSP_LO;
    rt = ((uint32_t)tempB << 16) | ((uint32_t)tempA & MIPSDSP_LO);

    return (target_long)(int32_t)rt;
}

target_ulong helper_precr_sra_r_ph_w(uint32_t sa,
                                     target_ulong rs, target_ulong rt)
{
    uint64_t tempB, tempA;

    /* If sa = 0, then (sa - 1) = -1 will case shift error, so we need else. */
    if (sa == 0) {
        tempB = (rt & MIPSDSP_LO) << 1;
        tempA = (rs & MIPSDSP_LO) << 1;
    } else {
        tempB = ((int32_t)rt >> (sa - 1)) + 1;
        tempA = ((int32_t)rs >> (sa - 1)) + 1;
    }
    rt = (((tempB >> 1) & MIPSDSP_LO) << 16) | ((tempA >> 1) & MIPSDSP_LO);

    return (target_long)(int32_t)rt;
}

target_ulong helper_precrq_ph_w(target_ulong rs, target_ulong rt)
{
    uint16_t tempB, tempA;
    uint32_t rd;

    tempB = (rs & MIPSDSP_HI) >> 16;
    tempA = (rt & MIPSDSP_HI) >> 16;
    rd = ((uint32_t)tempB << 16) | ((uint32_t)tempA & MIPSDSP_LO);

    return (target_long)(int32_t)rd;
}

target_ulong helper_precrq_rs_ph_w(CPUMIPSState *env,
                                   target_ulong rs, target_ulong rt)
{
    uint16_t tempB, tempA;
    uint32_t rd;

    tempB = mipsdsp_trunc16_sat16_round(env, rs);
    tempA = mipsdsp_trunc16_sat16_round(env, rt);
    rd = ((uint32_t)tempB << 16) | (uint32_t)tempA;

    return (target_long)(int32_t)rd;
}

#if defined(TARGET_MIPS64)
target_ulong helper_precr_ob_qh(target_ulong rs, target_ulong rt)
{
    uint8_t rs6, rs4, rs2, rs0;
    uint8_t rt6, rt4, rt2, rt0;
    uint64_t temp;

    rs6 = (rs >> 48) & MIPSDSP_Q0;
    rs4 = (rs >> 32) & MIPSDSP_Q0;
    rs2 = (rs >> 16) & MIPSDSP_Q0;
    rs0 = rs & MIPSDSP_Q0;
    rt6 = (rt >> 48) & MIPSDSP_Q0;
    rt4 = (rt >> 32) & MIPSDSP_Q0;
    rt2 = (rt >> 16) & MIPSDSP_Q0;
    rt0 = rt & MIPSDSP_Q0;

    temp = ((uint64_t)rs6 << 56) | ((uint64_t)rs4 << 48) |
           ((uint64_t)rs2 << 40) | ((uint64_t)rs0 << 32) |
           ((uint64_t)rt6 << 24) | ((uint64_t)rt4 << 16) |
           ((uint64_t)rt2 << 8) | (uint64_t)rt0;

    return temp;
}

target_ulong helper_precr_sra_qh_pw(target_ulong rs, target_ulong rt,
                                    uint32_t sa)
{
    uint16_t rs3, rs2, rs1, rs0;
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t result;

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    /* When sa = 0, we use rt2, rt0, rs2, rs0;
     * when sa != 0, we use rt3, rt1, rs3, rs1. */
    if (sa == 0) {
        tempD = rt2;
        tempC = rt0;
        tempB = rs2;
        tempA = rs0;
    } else {
        tempD = (int16_t)rt3 >> sa;
        tempC = (int16_t)rt1 >> sa;
        tempB = (int16_t)rs3 >> sa;
        tempA = (int16_t)rs1 >> sa;
    }

    result = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
             ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return result;
}


target_ulong helper_precr_sra_r_qh_pw(target_ulong rs, target_ulong rt,
                                      uint32_t sa)
{
    uint16_t rs3, rs2, rs1, rs0;
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t result;

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    /* When sa = 0, we use rt2, rt0, rs2, rs0;
     * when sa != 0, we use rt3, rt1, rs3, rs1. */
    if (sa == 0) {
        tempD = rt2 << 1;
        tempC = rt0 << 1;
        tempB = rs2 << 1;
        tempA = rs0 << 1;
    } else {
        tempD = (((int16_t)rt3 >> sa) + 1) >> 1;
        tempC = (((int16_t)rt1 >> sa) + 1) >> 1;
        tempB = (((int16_t)rs3 >> sa) + 1) >> 1;
        tempA = (((int16_t)rs1 >> sa) + 1) >> 1;
    }

    result = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
             ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return result;
}

target_ulong helper_precrq_ob_qh(target_ulong rs, target_ulong rt)
{
    uint8_t rs6, rs4, rs2, rs0;
    uint8_t rt6, rt4, rt2, rt0;
    uint64_t temp;

    rs6 = (rs >> 56) & MIPSDSP_Q0;
    rs4 = (rs >> 40) & MIPSDSP_Q0;
    rs2 = (rs >> 24) & MIPSDSP_Q0;
    rs0 = (rs >> 8) & MIPSDSP_Q0;
    rt6 = (rt >> 56) & MIPSDSP_Q0;
    rt4 = (rt >> 40) & MIPSDSP_Q0;
    rt2 = (rt >> 24) & MIPSDSP_Q0;
    rt0 = (rt >> 8) & MIPSDSP_Q0;

    temp = ((uint64_t)rs6 << 56) | ((uint64_t)rs4 << 48) |
           ((uint64_t)rs2 << 40) | ((uint64_t)rs0 << 32) |
           ((uint64_t)rt6 << 24) | ((uint64_t)rt4 << 16) |
           ((uint64_t)rt2 << 8) | (uint64_t)rt0;

    return temp;
}

target_ulong helper_precrq_qh_pw(target_ulong rs, target_ulong rt)
{
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    tempD = (rs >> 48) & MIPSDSP_LO;
    tempC = (rs >> 16) & MIPSDSP_LO;
    tempB = (rt >> 48) & MIPSDSP_LO;
    tempA = (rt >> 16) & MIPSDSP_LO;

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
           ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return temp;
}

target_ulong helper_precrq_rs_qh_pw(CPUMIPSState *env,
                                    target_ulong rs, target_ulong rt)
{
    uint32_t rs2, rs0;
    uint32_t rt2, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    rs2 = (rs >> 32) & MIPSDSP_LLO;
    rs0 = rs & MIPSDSP_LLO;
    rt2 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempD = mipsdsp_trunc16_sat16_round(env, rs2);
    tempC = mipsdsp_trunc16_sat16_round(env, rs0);
    tempB = mipsdsp_trunc16_sat16_round(env, rt2);
    tempA = mipsdsp_trunc16_sat16_round(env, rt0);

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
           ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return temp;
}

target_ulong helper_precrq_pw_l(target_ulong rs, target_ulong rt)
{
    uint32_t tempB, tempA;
    uint64_t temp;

    tempB = (rs >> 32) & MIPSDSP_LLO;
    tempA = (rt >> 32) & MIPSDSP_LLO;

    temp = ((uint64_t)tempB << 32) | (uint64_t)tempA;

    return temp;
}
#endif

target_ulong helper_precrqu_s_qb_ph(CPUMIPSState *env,
                                    target_ulong rs, target_ulong rt)
{
    uint8_t  tempD, tempC, tempB, tempA;
    uint16_t rsh, rsl, rth, rtl;
    uint32_t rd;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempD = mipsdsp_sat8_reduce_precision(env, rsh);
    tempC = mipsdsp_sat8_reduce_precision(env, rsl);
    tempB = mipsdsp_sat8_reduce_precision(env, rth);
    tempA = mipsdsp_sat8_reduce_precision(env, rtl);

    rd = ((uint32_t)tempD << 24) | ((uint32_t)tempC << 16) |
         ((uint32_t)tempB <<  8) | (uint32_t)tempA;

    return (target_long)(int32_t)rd;
}

#if defined(TARGET_MIPS64)
target_ulong helper_precrqu_s_ob_qh(CPUMIPSState *env,
                                    target_ulong rs, target_ulong rt)
{
    int i;
    uint16_t rs3, rs2, rs1, rs0;
    uint16_t rt3, rt2, rt1, rt0;
    uint8_t temp[8];
    uint64_t result;

    result = 0;

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    temp[7] = mipsdsp_sat8_reduce_precision(env, rs3);
    temp[6] = mipsdsp_sat8_reduce_precision(env, rs2);
    temp[5] = mipsdsp_sat8_reduce_precision(env, rs1);
    temp[4] = mipsdsp_sat8_reduce_precision(env, rs0);
    temp[3] = mipsdsp_sat8_reduce_precision(env, rt3);
    temp[2] = mipsdsp_sat8_reduce_precision(env, rt2);
    temp[1] = mipsdsp_sat8_reduce_precision(env, rt1);
    temp[0] = mipsdsp_sat8_reduce_precision(env, rt0);

    for (i = 0; i < 8; i++) {
        result |= (uint64_t)temp[i] << (8 * i);
    }

    return result;
}

target_ulong helper_preceq_pw_qhl(target_ulong rt)
{
    uint16_t tempB, tempA;
    uint32_t tempBI, tempAI;

    tempB = (rt >> 48) & MIPSDSP_LO;
    tempA = (rt >> 32) & MIPSDSP_LO;

    tempBI = (uint32_t)tempB << 16;
    tempAI = (uint32_t)tempA << 16;

    return ((uint64_t)tempBI << 32) | ((uint64_t)tempAI);
}

target_ulong helper_preceq_pw_qhr(target_ulong rt)
{
    uint16_t tempB, tempA;
    uint32_t tempBI, tempAI;

    tempB = (rt >> 16) & MIPSDSP_LO;
    tempA = rt & MIPSDSP_LO;

    tempBI = (uint32_t)tempB << 16;
    tempAI = (uint32_t)tempA << 16;

    return ((uint64_t)tempBI << 32) | ((uint64_t)tempAI);
}

target_ulong helper_preceq_pw_qhla(target_ulong rt)
{
    uint16_t tempB, tempA;
    uint32_t tempBI, tempAI;

    tempB = (rt >> 48) & MIPSDSP_LO;
    tempA = (rt >> 16) & MIPSDSP_LO;

    tempBI = (uint32_t)tempB << 16;
    tempAI = (uint32_t)tempA << 16;

    return ((uint64_t)tempBI << 32) | ((uint64_t)tempAI);
}

target_ulong helper_preceq_pw_qhra(target_ulong rt)
{
    uint16_t tempB, tempA;
    uint32_t tempBI, tempAI;

    tempB = (rt >> 32) & MIPSDSP_LO;
    tempA = rt & MIPSDSP_LO;

    tempBI = (uint32_t)tempB << 16;
    tempAI = (uint32_t)tempA << 16;

    return ((uint64_t)tempBI << 32) | ((uint64_t)tempAI);
}

#endif

target_ulong helper_precequ_ph_qbl(target_ulong rt)
{
    uint8_t  rt3, rt2;
    uint16_t tempB, tempA;

    rt3 = (rt & MIPSDSP_Q3) >> 24;
    rt2 = (rt & MIPSDSP_Q2) >> 16;

    tempB = (uint16_t)rt3 << 7;
    tempA = (uint16_t)rt2 << 7;

    return (target_long)(int32_t)(((uint32_t)tempB << 16) | (uint32_t)tempA);
}

target_ulong helper_precequ_ph_qbr(target_ulong rt)
{
    uint8_t  rt1, rt0;
    uint16_t tempB, tempA;

    rt1 = (rt & MIPSDSP_Q1) >> 8;
    rt0 =  rt & MIPSDSP_Q0;
    tempB = (uint16_t)rt1 << 7;
    tempA = (uint16_t)rt0 << 7;

    return (target_long)(int32_t)(((uint32_t)tempB << 16) | (uint32_t)tempA);
}

target_ulong helper_precequ_ph_qbla(target_ulong rt)
{
    uint8_t  rt3, rt1;
    uint16_t tempB, tempA;

    rt3 = (rt & MIPSDSP_Q3) >> 24;
    rt1 = (rt & MIPSDSP_Q1) >>  8;

    tempB = (uint16_t)rt3 << 7;
    tempA = (uint16_t)rt1 << 7;

    return (target_long)(int32_t)(((uint32_t)tempB << 16) | (uint32_t)tempA);
}

target_ulong helper_precequ_ph_qbra(target_ulong rt)
{
    uint8_t  rt2, rt0;
    uint16_t tempB, tempA;

    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt0 =  rt & MIPSDSP_Q0;
    tempB = (uint16_t)rt2 << 7;
    tempA = (uint16_t)rt0 << 7;

    return (target_long)(int32_t)(((uint32_t)tempB << 16) | (uint32_t)tempA);
}

#if defined(TARGET_MIPS64)
target_ulong helper_precequ_qh_obl(target_ulong rt)
{
    uint8_t tempDC, tempCC, tempBC, tempAC;
    uint16_t tempDS, tempCS, tempBS, tempAS;
    uint64_t temp;

    tempDC = (rt >> 56) & MIPSDSP_Q0;
    tempCC = (rt >> 48) & MIPSDSP_Q0;
    tempBC = (rt >> 40) & MIPSDSP_Q0;
    tempAC = (rt >> 32) & MIPSDSP_Q0;

    tempDS = (uint16_t)tempDC << 7;
    tempCS = (uint16_t)tempCC << 7;
    tempBS = (uint16_t)tempBC << 7;
    tempAS = (uint16_t)tempAC << 7;

    temp = ((uint64_t)tempDS << 48) | ((uint64_t)tempCS << 32) |
           ((uint64_t)tempBS << 16) | (uint64_t)tempAS;
    return temp;
}

target_ulong helper_precequ_qh_obr(target_ulong rt)
{
    uint8_t tempDC, tempCC, tempBC, tempAC;
    uint16_t tempDS, tempCS, tempBS, tempAS;
    uint64_t temp;

    tempDC = (rt >> 24) & MIPSDSP_Q0;
    tempCC = (rt >> 16) & MIPSDSP_Q0;
    tempBC = (rt >> 8) & MIPSDSP_Q0;
    tempAC = rt & MIPSDSP_Q0;

    tempDS = (uint16_t)tempDC << 7;
    tempCS = (uint16_t)tempCC << 7;
    tempBS = (uint16_t)tempBC << 7;
    tempAS = (uint16_t)tempAC << 7;

    temp = ((uint64_t)tempDS << 48) | ((uint64_t)tempCS << 32) |
           ((uint64_t)tempBS << 16) | (uint64_t)tempAS;
    return temp;
}

target_ulong helper_precequ_qh_obla(target_ulong rt)
{
    uint8_t tempDC, tempCC, tempBC, tempAC;
    uint16_t tempDS, tempCS, tempBS, tempAS;
    uint64_t temp;

    tempDC = (rt >> 56) & MIPSDSP_Q0;
    tempCC = (rt >> 40) & MIPSDSP_Q0;
    tempBC = (rt >> 24) & MIPSDSP_Q0;
    tempAC = (rt >> 8) & MIPSDSP_Q0;

    tempDS = (uint16_t)tempDC << 7;
    tempCS = (uint16_t)tempCC << 7;
    tempBS = (uint16_t)tempBC << 7;
    tempAS = (uint16_t)tempAC << 7;

    temp = ((uint64_t)tempDS << 48) | ((uint64_t)tempCS << 32) |
           ((uint64_t)tempBS << 16) | (uint64_t)tempAS;
    return temp;
}

target_ulong helper_precequ_qh_obra(target_ulong rt)
{
    uint8_t tempDC, tempCC, tempBC, tempAC;
    uint16_t tempDS, tempCS, tempBS, tempAS;
    uint64_t temp;

    tempDC = (rt >> 48) & MIPSDSP_Q0;
    tempCC = (rt >> 32) & MIPSDSP_Q0;
    tempBC = (rt >> 16) & MIPSDSP_Q0;
    tempAC = rt & MIPSDSP_Q0;

    tempDS = (uint16_t)tempDC << 7;
    tempCS = (uint16_t)tempCC << 7;
    tempBS = (uint16_t)tempBC << 7;
    tempAS = (uint16_t)tempAC << 7;

    temp = ((uint64_t)tempDS << 48) | ((uint64_t)tempCS << 32) |
           ((uint64_t)tempBS << 16) | (uint64_t)tempAS;
    return temp;
}
#endif

target_ulong helper_preceu_ph_qbl(target_ulong rt)
{
    uint8_t  rt3, rt2;
    uint32_t rd;

    rt3 = (rt & MIPSDSP_Q3) >> 24;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rd = ((uint32_t)(uint16_t)rt3 << 16) | \
         ((uint32_t)(uint16_t)rt2 & MIPSDSP_LO);

    return (target_long)(int32_t)rd;
}

target_ulong helper_preceu_ph_qbr(target_ulong rt)
{
    uint8_t  rt1, rt0;
    uint32_t rd;

    rt1 = (rt & MIPSDSP_Q1) >> 8;
    rt0 =  rt & MIPSDSP_Q0;
    rd = ((uint32_t)(uint16_t)rt1 << 16) | \
         ((uint32_t)(uint16_t)rt0 & MIPSDSP_LO);
    return (target_long)(int32_t)rd;
}

target_ulong helper_preceu_ph_qbla(target_ulong rt)
{
    uint8_t  rt3, rt1;
    uint32_t rd;

    rt3 = (rt & MIPSDSP_Q3) >> 24;
    rt1 = (rt & MIPSDSP_Q1) >>  8;
    rd = ((uint32_t)(uint16_t)rt3 << 16) | \
         ((uint32_t)(uint16_t)rt1 & MIPSDSP_LO);

    return (target_long)(int32_t)rd;
}

target_ulong helper_preceu_ph_qbra(target_ulong rt)
{
    uint8_t  rt2, rt0;
    uint32_t rd;

    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt0 =  rt & MIPSDSP_Q0;
    rd = ((uint32_t)(uint16_t)rt2 << 16) | \
         ((uint32_t)(uint16_t)rt0 & MIPSDSP_LO);
    return (target_long)(int32_t)rd;
}

#if defined(TARGET_MIPS64)
target_ulong helper_preceu_qh_obl(target_ulong rt)
{
    uint8_t tempDC, tempCC, tempBC, tempAC;
    uint64_t temp;

    tempDC = (rt >> 56) & MIPSDSP_Q0;
    tempCC = (rt >> 48) & MIPSDSP_Q0;
    tempBC = (rt >> 40) & MIPSDSP_Q0;
    tempAC = (rt >> 32) & MIPSDSP_Q0;

    temp = ((uint64_t)(uint16_t)tempDC << 48) | \
           ((uint64_t)(uint16_t)tempCC << 32) | \
           ((uint64_t)(uint16_t)tempBC << 16) | \
           (uint64_t)(uint16_t)tempAC;
    return temp;
}

target_ulong helper_preceu_qh_obr(target_ulong rt)
{
    uint8_t tempDC, tempCC, tempBC, tempAC;
    uint64_t temp;

    tempDC = (rt >> 24) & MIPSDSP_Q0;
    tempCC = (rt >> 16) & MIPSDSP_Q0;
    tempBC = (rt >> 8) & MIPSDSP_Q0;
    tempAC = rt & MIPSDSP_Q0;

    temp = ((uint64_t)(uint16_t)tempDC << 48) | \
           ((uint64_t)(uint16_t)tempCC << 32) | \
           ((uint64_t)(uint16_t)tempBC << 16) | \
           (uint64_t)(uint16_t)tempAC;

    return temp;
}

target_ulong helper_preceu_qh_obla(target_ulong rt)
{
    uint8_t tempDC, tempCC, tempBC, tempAC;
    uint64_t temp;

    tempDC = (rt >> 56) & MIPSDSP_Q0;
    tempCC = (rt >> 40) & MIPSDSP_Q0;
    tempBC = (rt >> 24) & MIPSDSP_Q0;
    tempAC = (rt >> 8) & MIPSDSP_Q0;

    temp = ((uint64_t)(uint16_t)tempDC << 48) | \
           ((uint64_t)(uint16_t)tempCC << 32) | \
           ((uint64_t)(uint16_t)tempBC << 16) | \
           (uint64_t)(uint16_t)tempAC;

    return temp;
}

target_ulong helper_preceu_qh_obra(target_ulong rt)
{
    uint8_t tempDC, tempCC, tempBC, tempAC;
    uint64_t temp;

    tempDC = (rt >> 48) & MIPSDSP_Q0;
    tempCC = (rt >> 32) & MIPSDSP_Q0;
    tempBC = (rt >> 16) & MIPSDSP_Q0;
    tempAC = rt & MIPSDSP_Q0;

    temp = ((uint64_t)(uint16_t)tempDC << 48) | \
           ((uint64_t)(uint16_t)tempCC << 32) | \
           ((uint64_t)(uint16_t)tempBC << 16) | \
           (uint64_t)(uint16_t)tempAC;

    return temp;
}
#endif

/** DSP GPR-Based Shift Sub-class insns **/
target_ulong helper_shll_qb(CPUMIPSState *env, uint32_t sa, target_ulong rt)
{
    uint8_t  rt3, rt2, rt1, rt0;
    uint8_t  tempD, tempC, tempB, tempA;
    uint32_t rd;

    rt3 = (rt & MIPSDSP_Q3) >> 24;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt1 = (rt & MIPSDSP_Q1) >>  8;
    rt0 =  rt & MIPSDSP_Q0;

    tempD = mipsdsp_lshift8(env, rt3, sa);
    tempC = mipsdsp_lshift8(env, rt2, sa);
    tempB = mipsdsp_lshift8(env, rt1, sa);
    tempA = mipsdsp_lshift8(env, rt0, sa);
    rd = ((uint32_t)tempD << 24) | ((uint32_t)tempC << 16) |
         ((uint32_t)tempB <<  8) | ((uint32_t)tempA);

    return (target_long)(int32_t)rd;
}

target_ulong helper_shllv_qb(CPUMIPSState *env,
                             target_ulong rs, target_ulong rt)
{
    uint8_t  rs2_0;
    uint8_t  rt3, rt2, rt1, rt0;
    uint8_t  tempD, tempC, tempB, tempA;
    uint32_t rd;

    rs2_0 = rs & 0x07;
    rt3   = (rt & MIPSDSP_Q3) >> 24;
    rt2   = (rt & MIPSDSP_Q2) >> 16;
    rt1   = (rt & MIPSDSP_Q1) >>  8;
    rt0   =  rt & MIPSDSP_Q0;

    tempD = mipsdsp_lshift8(env, rt3, rs2_0);
    tempC = mipsdsp_lshift8(env, rt2, rs2_0);
    tempB = mipsdsp_lshift8(env, rt1, rs2_0);
    tempA = mipsdsp_lshift8(env, rt0, rs2_0);

    rd = ((uint32_t)tempD << 24) | ((uint32_t)tempC << 16) |
         ((uint32_t)tempB <<  8) | (uint32_t)tempA;

    return (target_long)(int32_t)rd;
}

#if defined(TARGET_MIPS64)
target_ulong helper_shll_ob(CPUMIPSState *env, target_ulong rt, uint32_t sa)
{
    int i;
    uint8_t rt_t[8];
    uint8_t temp_t[8];
    uint64_t temp;

    sa = sa & 0x07;
    temp = 0;

    for (i = 0; i < 8; i++) {
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;
        temp_t[i] = mipsdsp_lshift8(env, rt_t[i], sa);

        temp |= (uint64_t)temp_t[i] << (8 * i);
    }

    return temp;
}

target_ulong helper_shllv_ob(CPUMIPSState *env,
                             target_ulong rt, target_ulong sa)
{
    int i;
    uint8_t rt_t[8];
    uint8_t temp_t[8];
    uint64_t temp;

    sa = sa & 0x07;
    temp = 0;

    for (i = 0; i < 8; i++) {
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;
        temp_t[i] = mipsdsp_lshift8(env, rt_t[i], sa);

        temp |= (uint64_t)temp_t[i] << (8 * i);
    }

    return temp;
}
#endif

target_ulong helper_shll_ph(CPUMIPSState *env, uint32_t sa, target_ulong rt)
{
    uint16_t rth, rtl;
    uint16_t tempB, tempA;

    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;
    tempB = mipsdsp_lshift16(env, rth, sa);
    tempA = mipsdsp_lshift16(env, rtl, sa);

    return (target_long)(int32_t)(((uint32_t)tempB << 16) | (uint32_t)tempA);
}

target_ulong helper_shllv_ph(CPUMIPSState *env,
                             target_ulong rs, target_ulong rt)
{
    uint8_t  rs3_0;
    uint16_t rth, rtl, tempB, tempA;

    rth   = (rt & MIPSDSP_HI) >> 16;
    rtl   =  rt & MIPSDSP_LO;
    rs3_0 = rs & 0x0F;

    tempB = mipsdsp_lshift16(env, rth, rs3_0);
    tempA = mipsdsp_lshift16(env, rtl, rs3_0);

    return (target_long)(int32_t)(((uint32_t)tempB << 16) | (uint32_t)tempA);
}

target_ulong helper_shll_s_ph(CPUMIPSState *env, uint32_t sa, target_ulong rt)
{
    uint16_t rth, rtl;
    uint16_t tempB, tempA;

    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;
    tempB = mipsdsp_sat16_lshift(env, rth, sa);
    tempA = mipsdsp_sat16_lshift(env, rtl, sa);

    return (target_long)(int32_t)(((uint32_t)tempB << 16) | (uint32_t)tempA);
}

target_ulong helper_shllv_s_ph(CPUMIPSState *env,
                               target_ulong rs, target_ulong rt)
{
    uint8_t  rs3_0;
    uint16_t rth, rtl, tempB, tempA;

    rth   = (rt & MIPSDSP_HI) >> 16;
    rtl   =  rt & MIPSDSP_LO;
    rs3_0 = rs & 0x0F;

    tempB = mipsdsp_sat16_lshift(env, rth, rs3_0);
    tempA = mipsdsp_sat16_lshift(env, rtl, rs3_0);

    return (target_long)(int32_t)(((uint32_t)tempB << 16) | (uint32_t)tempA);
}

#if defined(TARGET_MIPS64)
target_ulong helper_shll_qh(CPUMIPSState *env, target_ulong rt, uint32_t sa)
{
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    sa = sa & 0x0F;

    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_lshift16(env, rt3, sa);
    tempC = mipsdsp_lshift16(env, rt2, sa);
    tempB = mipsdsp_lshift16(env, rt1, sa);
    tempA = mipsdsp_lshift16(env, rt0, sa);

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
           ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return temp;
}

target_ulong helper_shllv_qh(CPUMIPSState *env,
                             target_ulong rt, target_ulong sa)
{
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    sa = sa & 0x0F;

    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_lshift16(env, rt3, sa);
    tempC = mipsdsp_lshift16(env, rt2, sa);
    tempB = mipsdsp_lshift16(env, rt1, sa);
    tempA = mipsdsp_lshift16(env, rt0, sa);

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
           ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return temp;
}

target_ulong helper_shll_s_qh(CPUMIPSState *env, target_ulong rt, uint32_t sa)
{
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    sa = sa & 0x0F;

    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_sat16_lshift(env, rt3, sa);
    tempC = mipsdsp_sat16_lshift(env, rt2, sa);
    tempB = mipsdsp_sat16_lshift(env, rt1, sa);
    tempA = mipsdsp_sat16_lshift(env, rt0, sa);

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
           ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return temp;
}

target_ulong helper_shllv_s_qh(CPUMIPSState *env,
                               target_ulong rt, target_ulong sa)
{
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    sa = sa & 0x0F;

    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_sat16_lshift(env, rt3, sa);
    tempC = mipsdsp_sat16_lshift(env, rt2, sa);
    tempB = mipsdsp_sat16_lshift(env, rt1, sa);
    tempA = mipsdsp_sat16_lshift(env, rt0, sa);

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
           ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return temp;
}
#endif

target_ulong helper_shll_s_w(CPUMIPSState *env, uint32_t sa, target_ulong rt)
{
    uint32_t temp;

    temp = mipsdsp_sat32_lshift(env, rt, sa);

    return (target_long)(int32_t)temp;
}

target_ulong helper_shllv_s_w(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    uint8_t  rs4_0;
    uint32_t rd;

    rs4_0 = rs & 0x1F;
    rd = mipsdsp_sat32_lshift(env, rt, rs4_0);

    return (target_long)(int32_t)rd;
}

#if defined(TARGET_MIPS64)
target_ulong helper_shll_pw(CPUMIPSState *env, target_ulong rt, uint32_t sa)
{
    uint32_t rt1, rt0;
    uint32_t tempB, tempA;

    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB = mipsdsp_lshift32(env, rt1, sa);
    tempA = mipsdsp_lshift32(env, rt0, sa);

    return ((uint64_t)tempB << 32) | (uint64_t)tempA;
}

target_ulong helper_shllv_pw(CPUMIPSState *env,
                             target_ulong rt, target_ulong sa)
{
    uint32_t rt1, rt0;
    uint32_t tempB, tempA;

    sa = sa & 0x1F;

    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB = mipsdsp_lshift32(env, rt1, sa);
    tempA = mipsdsp_lshift32(env, rt0, sa);

    return ((uint64_t)tempB << 32) | (uint64_t)tempA;
}

target_ulong helper_shll_s_pw(CPUMIPSState *env, target_ulong rt, uint32_t sa)
{
    uint32_t rt1, rt0;
    uint32_t tempB, tempA;

    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB = mipsdsp_sat32_lshift(env, rt1, sa);
    tempA = mipsdsp_sat32_lshift(env, rt0, sa);

    return ((uint64_t)tempB << 32) | (uint64_t)tempA;
}

target_ulong helper_shllv_s_pw(CPUMIPSState *env,
                               target_ulong rt, target_ulong sa)
{
    uint32_t rt1, rt0;
    uint32_t tempB, tempA;

    sa = sa & 0x1F;

    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB = mipsdsp_sat32_lshift(env, rt1, sa);
    tempA = mipsdsp_sat32_lshift(env, rt0, sa);

    return ((uint64_t)tempB << 32) | (uint64_t)tempA;
}
#endif

target_ulong helper_shrl_qb(uint32_t sa, target_ulong rt)
{
    uint8_t  rt3, rt2, rt1, rt0;
    uint8_t  tempD, tempC, tempB, tempA;
    uint32_t rd;

    rt3 = (rt & MIPSDSP_Q3) >> 24;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt1 = (rt & MIPSDSP_Q1) >>  8;
    rt0 =  rt & MIPSDSP_Q0;

    tempD = rt3 >> sa;
    tempC = rt2 >> sa;
    tempB = rt1 >> sa;
    tempA = rt0 >> sa;

    rd = ((uint32_t)tempD << 24) | ((uint32_t)tempC << 16) |
         ((uint32_t)tempB <<  8) | (uint32_t)tempA;

    return (target_long)(int32_t)rd;
}

target_ulong helper_shrlv_qb(target_ulong rs, target_ulong rt)
{
    uint8_t rs2_0;
    uint8_t rt3, rt2, rt1, rt0;
    uint8_t tempD, tempC, tempB, tempA;
    uint32_t rd;

    rs2_0 = rs & 0x07;
    rt3   = (rt & MIPSDSP_Q3) >> 24;
    rt2   = (rt & MIPSDSP_Q2) >> 16;
    rt1   = (rt & MIPSDSP_Q1) >>  8;
    rt0   =  rt & MIPSDSP_Q0;

    tempD = rt3 >> rs2_0;
    tempC = rt2 >> rs2_0;
    tempB = rt1 >> rs2_0;
    tempA = rt0 >> rs2_0;
    rd = ((uint32_t)tempD << 24) | ((uint32_t)tempC << 16) |
         ((uint32_t)tempB <<  8) | (uint32_t)tempA;

    return (target_long)(int32_t)rd;
}

target_ulong helper_shrl_ph(uint32_t sa, target_ulong rt)
{
    uint16_t rth, rtl;
    uint16_t tempB, tempA;

    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;
    tempB = rth >> sa;
    tempA = rtl >> sa;

    return (target_long)(int32_t)(((uint32_t)tempB << 16) | (uint32_t)tempA);
}

target_ulong helper_shrlv_ph(target_ulong rs, target_ulong rt)
{
    uint8_t  rs3_0;
    uint16_t rth, rtl;
    uint16_t tempB, tempA;

    rs3_0 = rs & 0x0F;
    rth   = (rt & MIPSDSP_HI) >> 16;
    rtl   =  rt & MIPSDSP_LO;

    tempB = rth >> rs3_0;
    tempA = rtl >> rs3_0;

    return (target_long)(int32_t)(((uint32_t)tempB << 16) | (uint32_t)tempA);
}

#if defined(TARGET_MIPS64)
target_ulong helper_shrl_ob(target_ulong rt, uint32_t sa)
{
    int i;
    uint8_t rt_t[8];
    uint8_t temp[8];
    uint64_t result;

    sa = sa & 0x07;
    result = 0;

    for (i = 0; i < 8; i++) {
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;
        temp[i] = rt_t[i] >> sa;
        result |= (uint64_t)temp[i] << (8 * i);
    }

    return result;
}

target_ulong helper_shrlv_ob(target_ulong rt, target_ulong sa)
{
    int i;
    uint8_t rt_t[8];
    uint8_t temp[8];
    uint64_t result;

    sa = sa & 0x07;
    result = 0;

    for (i = 0; i < 8; i++) {
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;
        temp[i] = rt_t[i] >> sa;
        result |= (uint64_t)temp[i] << (8 * i);
    }

    return result;
}

target_ulong helper_shrl_qh(target_ulong rt, uint32_t sa)
{
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    sa = sa & 0x0F;

    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = rt3 >> sa;
    tempC = rt2 >> sa;
    tempB = rt1 >> sa;
    tempA = rt0 >> sa;

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
           ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return temp;
}

target_ulong helper_shrlv_qh(target_ulong rt, target_ulong sa)
{
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    sa = sa & 0x0F;

    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = rt3 >> sa;
    tempC = rt2 >> sa;
    tempB = rt1 >> sa;
    tempA = rt0 >> sa;

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
           ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return temp;
}
#endif

target_ulong helper_shra_qb(uint32_t sa, target_ulong rt)
{
    int8_t  rt3, rt2, rt1, rt0;
    uint8_t tempD, tempC, tempB, tempA;
    uint32_t rd;

    rt3 = (rt & MIPSDSP_Q3) >> 24;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt1 = (rt & MIPSDSP_Q1) >>  8;
    rt0 =  rt & MIPSDSP_Q0;

    tempD = rt3 >> sa;
    tempC = rt2 >> sa;
    tempB = rt1 >> sa;
    tempA = rt0 >> sa;

    rd = ((uint32_t)tempD << 24) | ((uint32_t)tempC << 16) |
          ((uint32_t)tempB << 8) | (uint32_t)tempA;

    return (target_long)(int32_t)rd;
}

target_ulong helper_shra_r_qb(uint32_t sa, target_ulong rt)
{
    int8_t  rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint32_t rd;

    rt3 = (rt & MIPSDSP_Q3) >> 24;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt1 = (rt & MIPSDSP_Q1) >>  8;
    rt0 =  rt & MIPSDSP_Q0;

    if (sa == 0) {
        tempD = rt3 & 0x00FF;
        tempC = rt2 & 0x00FF;
        tempB = rt1 & 0x00FF;
        tempA = rt0 & 0x00FF;
    } else {
        tempD = ((int16_t)rt3 >> (sa - 1)) + 1;
        tempC = ((int16_t)rt2 >> (sa - 1)) + 1;
        tempB = ((int16_t)rt1 >> (sa - 1)) + 1;
        tempA = ((int16_t)rt0 >> (sa - 1)) + 1;
    }

    rd = ((uint32_t)((tempD >> 1) & 0x00FF) << 24) |
         ((uint32_t)((tempC >> 1) & 0x00FF) << 16) |
         ((uint32_t)((tempB >> 1) & 0x00FF) <<  8) |
         (uint32_t)((tempA >>  1) & 0x00FF) ;

    return (target_long)(int32_t)rd;
}

target_ulong helper_shrav_qb(target_ulong rs, target_ulong rt)
{
    uint8_t  rs2_0;
    int8_t   rt3, rt2, rt1, rt0;
    uint8_t  tempD, tempC, tempB, tempA;
    uint32_t rd;

    rs2_0 = rs & 0x07;
    rt3   = (rt & MIPSDSP_Q3) >> 24;
    rt2   = (rt & MIPSDSP_Q2) >> 16;
    rt1   = (rt & MIPSDSP_Q1) >>  8;
    rt0   =  rt & MIPSDSP_Q0;

    if (rs2_0 == 0) {
        tempD = rt3;
        tempC = rt2;
        tempB = rt1;
        tempA = rt0;
    } else {
        tempD = rt3 >> rs2_0;
        tempC = rt2 >> rs2_0;
        tempB = rt1 >> rs2_0;
        tempA = rt0 >> rs2_0;
    }

    rd = ((uint32_t)tempD << 24) | ((uint32_t)tempC << 16) |
         ((uint32_t)tempB <<  8) | (uint32_t)tempA;

    return (target_long)(int32_t)rd;
}

target_ulong helper_shrav_r_qb(target_ulong rs, target_ulong rt)
{
    uint8_t rs2_0;
    int8_t  rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint32_t rd;

    rs2_0 = rs & 0x07;
    rt3 = (rt & MIPSDSP_Q3) >> 24;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt1 = (rt & MIPSDSP_Q1) >>  8;
    rt0 =  rt & MIPSDSP_Q0;

    if (rs2_0 == 0) {
        tempD = (int16_t)rt3 << 1;
        tempC = (int16_t)rt2 << 1;
        tempB = (int16_t)rt1 << 1;
        tempA = (int16_t)rt0 << 1;
    } else {
        tempD = ((int16_t)rt3 >> (rs2_0 - 1)) + 1;
        tempC = ((int16_t)rt2 >> (rs2_0 - 1)) + 1;
        tempB = ((int16_t)rt1 >> (rs2_0 - 1)) + 1;
        tempA = ((int16_t)rt0 >> (rs2_0 - 1)) + 1;
    }

    rd = ((uint32_t)((tempD >> 1) & 0x00FF) << 24) |
         ((uint32_t)((tempC >> 1) & 0x00FF) << 16) |
         ((uint32_t)((tempB >> 1) & 0x00FF) <<  8) |
         (uint32_t)((tempA >>  1) & 0x00FF) ;

    return (target_long)(int32_t)rd;
}

#if defined(TARGET_MIPS64)
target_ulong helper_shra_ob(target_ulong rt, uint32_t sa)
{
    int i;
    int8_t rt_t[8];
    uint8_t temp[8];
    uint64_t result;

    result = 0;
    sa = sa & 0x07;

    for (i = 0; i < 8; i++) {
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;
        temp[i] = rt_t[i] >> sa;
        result |= (uint64_t)temp[i] << (8 * i);
    }

    return result;
}

target_ulong helper_shrav_ob(target_ulong rt, target_ulong sa)
{
    int i;
    int8_t rt_t[8];
    uint8_t temp[8];
    uint64_t result;

    result = 0;
    sa = sa & 0x07;

    for (i = 0; i < 8; i++) {
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;
        temp[i] = rt_t[i] >> sa;
        result |= (uint64_t)temp[i] << (8 * i);
    }

    return result;
}

target_ulong helper_shra_r_ob(target_ulong rt, uint32_t sa)
{
    int i;
    int8_t rt_t[8];
    int16_t rt_t_S[8];
    uint8_t temp[8];
    uint64_t result;

    result = 0;
    sa = sa & 0x07;

    if (sa == 0) {
        result = rt;
    } else {
        for (i = 0; i < 8; i++) {
            rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;
            rt_t_S[i] = ((int16_t)rt_t[i] >> (sa - 1)) + 1;
            temp[i] = (rt_t_S[i] >> 1) & MIPSDSP_Q0;
            result |= (uint64_t)temp[i] << (8 * i);
        }
    }

    return result;
}

target_ulong helper_shrav_r_ob(target_ulong rt, target_ulong sa)
{
    int i;
    int8_t rt_t[8];
    int16_t rt_t_S[8];
    uint8_t temp[8];
    uint64_t result;

    result = 0;
    sa = sa & 0x07;

    if (sa == 0) {
        result = rt;
    } else {
        for (i = 0; i < 8; i++) {
            rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;
            rt_t_S[i] = ((int16_t)rt_t[i] >> (sa - 1)) + 1;
            temp[i] = (rt_t_S[i] >> 1) & MIPSDSP_Q0;
            result |= (uint64_t)temp[i] << (8 * i);
        }
    }

    return result;
}
#endif

target_ulong helper_shra_ph(uint32_t sa, target_ulong rt)
{
    uint16_t rth, rtl;
    uint16_t tempB, tempA;

    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;
    tempB = (int16_t)rth >> sa;
    tempA = (int16_t)rtl >> sa;

    return (target_long)(int32_t)(((uint32_t)tempB << 16) | (uint32_t) tempA);
}

target_ulong helper_shrav_ph(target_ulong rs, target_ulong rt)
{
    uint8_t  rs3_0;
    uint16_t rth, rtl;
    uint16_t tempB, tempA;

    rs3_0 = rs & 0x0F;
    rth   = (rt & MIPSDSP_HI) >> 16;
    rtl   =  rt & MIPSDSP_LO;
    tempB = (int16_t)rth >> rs3_0;
    tempA = (int16_t)rtl >> rs3_0;

    return (target_long)(int32_t)(((uint32_t)tempB << 16) | (uint32_t)tempA);
}

target_ulong helper_shra_r_ph(uint32_t sa, target_ulong rt)
{
    uint16_t rth, rtl;
    uint16_t tempB, tempA;

    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;
    tempB = mipsdsp_rnd16_rashift(rth, sa);
    tempA = mipsdsp_rnd16_rashift(rtl, sa);

    return (target_long)(int32_t)(((uint32_t)tempB << 16) | (uint32_t) tempA);
}

target_ulong helper_shrav_r_ph(target_ulong rs, target_ulong rt)
{
    uint8_t  rs3_0;
    uint16_t rth, rtl;
    uint16_t tempB, tempA;

    rs3_0 = rs & 0x0F;
    rth   = (rt & MIPSDSP_HI) >> 16;
    rtl   =  rt & MIPSDSP_LO;
    tempB = mipsdsp_rnd16_rashift(rth, rs3_0);
    tempA = mipsdsp_rnd16_rashift(rtl, rs3_0);

    return (target_long)(int32_t)(((uint32_t)tempB << 16) | (uint32_t)tempA);
}

target_ulong helper_shra_r_w(uint32_t sa, target_ulong rt)
{
    uint32_t rd;

    rd = mipsdsp_rnd32_rashift(rt, sa);

    return (target_long)(int32_t)rd;
}

target_ulong helper_shrav_r_w(target_ulong rs, target_ulong rt)
{
    uint8_t rs4_0;
    uint32_t rd;

    rs4_0 = rs & 0x1F;
    rd = mipsdsp_rnd32_rashift(rt, rs4_0);

    return (target_long)(int32_t)rd;
}

#if defined(TARGET_MIPS64)
target_ulong helper_shra_qh(target_ulong rt, uint32_t sa)
{
    int16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t result;

    result = 0;
    sa = sa & 0xF;

    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = rt3 >> sa;
    tempC = rt2 >> sa;
    tempB = rt1 >> sa;
    tempA = rt0 >> sa;

    result = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
             ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return result;
}

target_ulong helper_shrav_qh(target_ulong rt, target_ulong sa)
{
    int16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t result;

    result = 0;
    sa = sa & 0xF;

    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = rt3 >> sa;
    tempC = rt2 >> sa;
    tempB = rt1 >> sa;
    tempA = rt0 >> sa;

    result = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
             ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return result;
}


target_ulong helper_shra_r_qh(target_ulong rt, uint32_t sa)
{
    int32_t rt3, rt2, rt1, rt0;
    uint32_t tempD, tempC, tempB, tempA;
    uint64_t result;

    result = 0;
    sa = sa & 0xF;

    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_rnd16_rashift(rt3, sa);
    tempC = mipsdsp_rnd16_rashift(rt2, sa);
    tempB = mipsdsp_rnd16_rashift(rt1, sa);
    tempA = mipsdsp_rnd16_rashift(rt0, sa);

    result = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
             ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return result;
}

target_ulong helper_shrav_r_qh(target_ulong rt, target_ulong sa)
{
    int32_t rt3, rt2, rt1, rt0;
    uint32_t tempD, tempC, tempB, tempA;
    uint64_t result;

    result = 0;
    sa = sa & 0xF;

    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_rnd16_rashift(rt3, sa);
    tempC = mipsdsp_rnd16_rashift(rt2, sa);
    tempB = mipsdsp_rnd16_rashift(rt1, sa);
    tempA = mipsdsp_rnd16_rashift(rt0, sa);

    result = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
             ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return result;
}

target_ulong helper_shra_pw(target_ulong rt, uint32_t sa)
{
    int32_t rt1, rt0;
    uint32_t tempB, tempA;

    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB = rt1 >> sa;
    tempA = rt0 >> sa;

    return ((uint64_t)tempB << 32) | (uint64_t)tempA;
}

target_ulong helper_shrav_pw(target_ulong rt, target_ulong sa)
{
    int32_t rt1, rt0;
    uint32_t tempB, tempA;

    sa = sa & 0x1F;

    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB = rt1 >> sa;
    tempA = rt0 >> sa;

    return ((uint64_t)tempB << 32) | (uint64_t)tempA;
}

target_ulong helper_shra_r_pw(target_ulong rt, uint32_t sa)
{
    int32_t rt1, rt0;
    uint32_t tempB, tempA;

    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB = mipsdsp_rnd32_rashift(rt1, sa);
    tempA = mipsdsp_rnd32_rashift(rt0, sa);

    return ((uint64_t)tempB << 32) | (uint64_t)tempA;
}

target_ulong helper_shrav_r_pw(target_ulong rt, target_ulong sa)
{
    int32_t rt1, rt0;
    uint32_t tempB, tempA;

    sa = sa & 0x1F;

    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB = mipsdsp_rnd32_rashift(rt1, sa);
    tempA = mipsdsp_rnd32_rashift(rt0, sa);

    return ((uint64_t)tempB << 32) | (uint64_t)tempA;
}
#endif

/** DSP Multiply Sub-class insns **/
target_ulong helper_muleu_s_ph_qbl(CPUMIPSState *env,
                                   target_ulong rs, target_ulong rt)
{
    uint8_t rs3, rs2;
    uint16_t tempB, tempA, rth, rtl;

    rs3 = (rs & MIPSDSP_Q3) >> 24;
    rs2 = (rs & MIPSDSP_Q2) >> 16;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;
    tempB = mipsdsp_mul_u8_u16(env, rs3, rth);
    tempA = mipsdsp_mul_u8_u16(env, rs2, rtl);
    return (target_long)(int32_t)(((uint32_t)tempB << 16) | \
                                  ((uint32_t)tempA & MIPSDSP_LO));
}

target_ulong helper_muleu_s_ph_qbr(CPUMIPSState *env,
                                   target_ulong rs, target_ulong rt)
{
    uint8_t  rs1, rs0;
    uint16_t tempB, tempA;
    uint16_t rth,   rtl;

    rs1 = (rs & MIPSDSP_Q1) >>  8;
    rs0 =  rs & MIPSDSP_Q0;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB = mipsdsp_mul_u8_u16(env, rs1, rth);
    tempA = mipsdsp_mul_u8_u16(env, rs0, rtl);
    return (target_long)(int32_t)(((uint32_t)tempB << 16) | \
                                  ((uint32_t)tempA & MIPSDSP_LO));
}

#if defined(TARGET_MIPS64)
target_ulong helper_muleu_s_qh_obl(CPUMIPSState *env,
                                   target_ulong rs, target_ulong rt)
{
    uint8_t rs3, rs2, rs1, rs0;
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    rs3 = (rs >> 56) & MIPSDSP_Q0;
    rs2 = (rs >> 48) & MIPSDSP_Q0;
    rs1 = (rs >> 40) & MIPSDSP_Q0;
    rs0 = (rs >> 32) & MIPSDSP_Q0;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_mul_u8_u16(env, rs3, rt3);
    tempC = mipsdsp_mul_u8_u16(env, rs2, rt2);
    tempB = mipsdsp_mul_u8_u16(env, rs1, rt1);
    tempA = mipsdsp_mul_u8_u16(env, rs0, rt0);

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
           ((uint64_t)tempB << 16) | (uint64_t)tempA;
    return temp;
}

target_ulong helper_muleu_s_qh_obr(CPUMIPSState *env,
                                   target_ulong rs, target_ulong rt)
{
    uint8_t rs3, rs2, rs1, rs0;
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    rs3 = (rs >> 24) & MIPSDSP_Q0;
    rs2 = (rs >> 16) & MIPSDSP_Q0;
    rs1 = (rs >> 8) & MIPSDSP_Q0;
    rs0 = rs & MIPSDSP_Q0;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_mul_u8_u16(env, rs3, rt3);
    tempC = mipsdsp_mul_u8_u16(env, rs2, rt2);
    tempB = mipsdsp_mul_u8_u16(env, rs1, rt1);
    tempA = mipsdsp_mul_u8_u16(env, rs0, rt0);

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
           ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return temp;
}
#endif

target_ulong helper_mulq_rs_ph(CPUMIPSState *env,
                               target_ulong rs, target_ulong rt)
{
    int16_t tempB, tempA, rsh, rsl, rth, rtl;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB = mipsdsp_rndq15_mul_q15_q15(env, rsh, rth);
    tempA = mipsdsp_rndq15_mul_q15_q15(env, rsl, rtl);

    return (target_long)(int32_t)(((uint32_t)tempB << 16) \
                                  | ((uint32_t)tempA & MIPSDSP_LO));
}

#if defined(TARGET_MIPS64)
target_ulong helper_mulq_rs_qh(CPUMIPSState *env,
                               target_ulong rs, target_ulong rt)
{
    uint16_t rs3, rs2, rs1, rs0;
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp;

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_rndq15_mul_q15_q15(env, rs3, rt3);
    tempC = mipsdsp_rndq15_mul_q15_q15(env, rs2, rt2);
    tempB = mipsdsp_rndq15_mul_q15_q15(env, rs1, rt1);
    tempA = mipsdsp_rndq15_mul_q15_q15(env, rs0, rt0);

    temp = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
           ((uint64_t)tempB << 16) | (uint64_t)tempA;
    return temp;
}
#endif

target_ulong helper_muleq_s_w_phl(CPUMIPSState *env,
                                  target_ulong rs, target_ulong rt)
{
    int16_t rsh, rth;
    int32_t temp;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rth = (rt & MIPSDSP_HI) >> 16;
    temp = mipsdsp_mul_q15_q15_overflowflag21(env, rsh, rth);

    return (target_long)(int32_t)temp;
}

target_ulong helper_muleq_s_w_phr(CPUMIPSState *env,
                                  target_ulong rs, target_ulong rt)
{
    int16_t rsl, rtl;
    int32_t temp;

    rsl = rs & MIPSDSP_LO;
    rtl = rt & MIPSDSP_LO;
    temp = mipsdsp_mul_q15_q15_overflowflag21(env, rsl, rtl);

    return (target_long)(int32_t)temp;
}

#if defined(TARGET_MIPS64)
target_ulong helper_muleq_s_pw_qhl(CPUMIPSState *env,
                                   target_ulong rs, target_ulong rt)
{
    uint16_t rsB, rsA;
    uint16_t rtB, rtA;
    uint32_t tempB, tempA;

    rsB = (rs >> 48) & MIPSDSP_LO;
    rsA = (rs >> 32) & MIPSDSP_LO;
    rtB = (rt >> 48) & MIPSDSP_LO;
    rtA = (rt >> 32) & MIPSDSP_LO;

    tempB = mipsdsp_mul_q15_q15(env, 5, rsB, rtB);
    tempA = mipsdsp_mul_q15_q15(env, 5, rsA, rtA);

    return ((uint64_t)tempB << 32) | (uint64_t)tempA;
}

target_ulong helper_muleq_s_pw_qhr(CPUMIPSState *env,
                                   target_ulong rs, target_ulong rt)
{
    uint16_t rsB, rsA;
    uint16_t rtB, rtA;
    uint32_t tempB, tempA;

    rsB = (rs >> 16) & MIPSDSP_LO;
    rsA = rs & MIPSDSP_LO;
    rtB = (rt >> 16) & MIPSDSP_LO;
    rtA = rt & MIPSDSP_LO;

    tempB = mipsdsp_mul_q15_q15(env, 5, rsB, rtB);
    tempA = mipsdsp_mul_q15_q15(env, 5, rsA, rtA);

    return ((uint64_t)tempB << 32) | (uint64_t)tempA;
}
#endif

void helper_dpau_h_qbl(CPUMIPSState *env,
                       uint32_t ac, target_ulong rs, target_ulong rt)
{
    uint8_t rs3, rs2;
    uint8_t rt3, rt2;
    uint16_t tempB, tempA;
    uint64_t tempC, dotp;

    rs3 = (rs & MIPSDSP_Q3) >> 24;
    rt3 = (rt & MIPSDSP_Q3) >> 24;
    rs2 = (rs & MIPSDSP_Q2) >> 16;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    tempB = mipsdsp_mul_u8_u8(rs3, rt3);
    tempA = mipsdsp_mul_u8_u8(rs2, rt2);
    dotp = (int64_t)tempB + (int64_t)tempA;
    tempC = (((uint64_t)env->active_tc.HI[ac] << 32) |
             ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO)) + dotp;

    env->active_tc.HI[ac] = (target_long)(int32_t)((tempC & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(tempC & MIPSDSP_LLO);
}

void helper_dpau_h_qbr(CPUMIPSState *env,
                       uint32_t ac, target_ulong rs, target_ulong rt)
{
    uint8_t rs1, rs0;
    uint8_t rt1, rt0;
    uint16_t tempB, tempA;
    uint64_t tempC, dotp;

    rs1 = (rs & MIPSDSP_Q1) >> 8;
    rt1 = (rt & MIPSDSP_Q1) >> 8;
    rs0 = (rs & MIPSDSP_Q0);
    rt0 = (rt & MIPSDSP_Q0);
    tempB = mipsdsp_mul_u8_u8(rs1, rt1);
    tempA = mipsdsp_mul_u8_u8(rs0, rt0);
    dotp = (int64_t)tempB + (int64_t)tempA;
    tempC = (((uint64_t)env->active_tc.HI[ac] << 32) |
             ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO)) + dotp;

    env->active_tc.HI[ac] = (target_long)(int32_t)((tempC & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(tempC & MIPSDSP_LLO);
}

#if defined(TARGET_MIPS64)
void helper_dpau_h_obl(CPUMIPSState *env,
                       target_ulong rs, target_ulong rt, uint32_t ac)
{
    uint8_t rs7, rs6, rs5, rs4;
    uint8_t rt7, rt6, rt5, rt4;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp[2];
    uint64_t acc[2];
    uint64_t temp_sum;

    temp[0] = 0;
    temp[1] = 0;

    rs7 = (rs >> 56) & MIPSDSP_Q0;
    rs6 = (rs >> 48) & MIPSDSP_Q0;
    rs5 = (rs >> 40) & MIPSDSP_Q0;
    rs4 = (rs >> 32) & MIPSDSP_Q0;
    rt7 = (rt >> 56) & MIPSDSP_Q0;
    rt6 = (rt >> 48) & MIPSDSP_Q0;
    rt5 = (rt >> 40) & MIPSDSP_Q0;
    rt4 = (rt >> 32) & MIPSDSP_Q0;

    tempD = mipsdsp_mul_u8_u8(rs7, rt7);
    tempC = mipsdsp_mul_u8_u8(rs6, rt6);
    tempB = mipsdsp_mul_u8_u8(rs5, rt5);
    tempA = mipsdsp_mul_u8_u8(rs4, rt4);

    temp[0] = (uint64_t)tempD + (uint64_t)tempC +
      (uint64_t)tempB + (uint64_t)tempA;

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];

    temp_sum = acc[0] + temp[0];

    if (temp_sum < acc[0] && temp_sum < temp[0]) {
        temp[1] += 1;
    }
    temp[0] = temp_sum;
    temp[1] += acc[1];

    env->active_tc.HI[ac] = temp[1];
    env->active_tc.LO[ac] = temp[0];
}

void helper_dpau_h_obr(CPUMIPSState *env,
                       target_ulong rs, target_ulong rt, uint32_t ac)
{
    uint8_t rs3, rs2, rs1, rs0;
    uint8_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t temp[2];
    uint64_t acc[2];
    uint64_t temp_sum;

    temp[0] = 0;
    temp[1] = 0;

    rs3 = (rs >> 24) & MIPSDSP_Q0;
    rs2 = (rs >> 16) & MIPSDSP_Q0;
    rs1 = (rs >> 8) & MIPSDSP_Q0;
    rs0 = rs & MIPSDSP_Q0;
    rt3 = (rt >> 24) & MIPSDSP_Q0;
    rt2 = (rt >> 16) & MIPSDSP_Q0;
    rt1 = (rt >> 8) & MIPSDSP_Q0;
    rt0 = rt & MIPSDSP_Q0;

    tempD = mipsdsp_mul_u8_u8(rs3, rt3);
    tempC = mipsdsp_mul_u8_u8(rs2, rt2);
    tempB = mipsdsp_mul_u8_u8(rs1, rt1);
    tempA = mipsdsp_mul_u8_u8(rs0, rt0);

    temp[0] = (uint64_t)tempD + (uint64_t)tempC +
              (uint64_t)tempB + (uint64_t)tempA;

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];

    temp_sum = acc[0] + temp[0];

    if (temp_sum < acc[0] && temp_sum < temp[0]) {
        temp[1] += 1;
    }
    temp[0] = temp_sum;
    temp[1] += acc[1];

    env->active_tc.HI[ac] = temp[1];
    env->active_tc.LO[ac] = temp[0];
}
#endif

void helper_dpsu_h_qbl(CPUMIPSState *env,
                       uint32_t ac, target_ulong rs, target_ulong rt)
{
    uint8_t  rs3, rs2, rt3, rt2;
    uint16_t tempB,  tempA;
    uint64_t dotp, tempC;

    rs3 = (rs & MIPSDSP_Q3) >> 24;
    rs2 = (rs & MIPSDSP_Q2) >> 16;
    rt3 = (rt & MIPSDSP_Q3) >> 24;
    rt2 = (rt & MIPSDSP_Q2) >> 16;

    tempB = mipsdsp_mul_u8_u8(rs3, rt3);
    tempA = mipsdsp_mul_u8_u8(rs2, rt2);

    dotp   = (tempB & 0xFFFF) + (tempA & 0xFFFF);
    tempC  = ((uint64_t)env->active_tc.HI[ac] << 32) |
             ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    tempC -= dotp;

    env->active_tc.HI[ac] = (target_long)(int32_t)((tempC & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(tempC & MIPSDSP_LLO);
}

void helper_dpsu_h_qbr(CPUMIPSState *env,
                       uint32_t ac, target_ulong rs, target_ulong rt)
{
    uint8_t  rs1, rs0, rt1, rt0;
    uint16_t tempB,  tempA;
    uint64_t dotp, tempC;

    rs1 = (rs & MIPSDSP_Q1) >> 8;
    rs0 = (rs & MIPSDSP_Q0);
    rt1 = (rt & MIPSDSP_Q1) >> 8;
    rt0 = (rt & MIPSDSP_Q0);

    tempB = mipsdsp_mul_u8_u8(rs1, rt1);
    tempA = mipsdsp_mul_u8_u8(rs0, rt0);

    dotp   = (tempB & 0xFFFF) + (tempA & 0xFFFF);
    tempC  = ((uint64_t)env->active_tc.HI[ac] << 32) |
             ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    tempC -= dotp;

    env->active_tc.HI[ac] = (target_long)(int32_t)((tempC & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(tempC & MIPSDSP_LLO);
}

#if defined(TARGET_MIPS64)
void helper_dpsu_h_obl(CPUMIPSState *env,
                       target_ulong rs, target_ulong rt, uint32_t ac)
{
    uint8_t rs7, rs6, rs5, rs4;
    uint8_t rt7, rt6, rt5, rt4;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t acc[2];
    uint64_t temp[2];
    uint64_t temp_sum;

    temp[0] = 0;
    temp[1] = 0;

    rs7 = (rs >> 56) & MIPSDSP_Q0;
    rs6 = (rs >> 48) & MIPSDSP_Q0;
    rs5 = (rs >> 40) & MIPSDSP_Q0;
    rs4 = (rs >> 32) & MIPSDSP_Q0;
    rt7 = (rt >> 56) & MIPSDSP_Q0;
    rt6 = (rt >> 48) & MIPSDSP_Q0;
    rt5 = (rt >> 40) & MIPSDSP_Q0;
    rt4 = (rt >> 32) & MIPSDSP_Q0;

    tempD = mipsdsp_mul_u8_u8(rs7, rt7);
    tempC = mipsdsp_mul_u8_u8(rs6, rt6);
    tempB = mipsdsp_mul_u8_u8(rs5, rt5);
    tempA = mipsdsp_mul_u8_u8(rs4, rt4);

    temp[0] = (uint64_t)tempD + (uint64_t)tempC +
              (uint64_t)tempB + (uint64_t)tempA;

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];

    temp_sum = acc[0] - temp[0];
    if (temp_sum > acc[0]) {
        acc[1] -= 1;
    }
    acc[0] = temp_sum;
    acc[1] -= temp[1];

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}

void helper_dpsu_h_obr(CPUMIPSState *env,
                       target_ulong rs, target_ulong rt, uint32_t ac)
{
    uint8_t rs3, rs2, rs1, rs0;
    uint8_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t acc[2];
    uint64_t temp[2];
    uint64_t temp_sum;

    temp[0] = 0;
    temp[1] = 0;

    rs3 = (rs >> 24) & MIPSDSP_Q0;
    rs2 = (rs >> 16) & MIPSDSP_Q0;
    rs1 = (rs >> 8) & MIPSDSP_Q0;
    rs0 = rs & MIPSDSP_Q0;
    rt3 = (rt >> 24) & MIPSDSP_Q0;
    rt2 = (rt >> 16) & MIPSDSP_Q0;
    rt1 = (rt >> 8) & MIPSDSP_Q0;
    rt0 = rt & MIPSDSP_Q0;

    tempD = mipsdsp_mul_u8_u8(rs3, rt3);
    tempC = mipsdsp_mul_u8_u8(rs2, rt2);
    tempB = mipsdsp_mul_u8_u8(rs1, rt1);
    tempA = mipsdsp_mul_u8_u8(rs0, rt0);

    temp[0] = (uint64_t)tempD + (uint64_t)tempC +
              (uint64_t)tempB + (uint64_t)tempA;

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];

    temp_sum = acc[0] - temp[0];
    if (temp_sum > acc[0]) {
        acc[1] -= 1;
    }
    acc[0] = temp_sum;
    acc[1] -= temp[1];

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}
#endif

void helper_dpa_w_ph(CPUMIPSState *env,
                     uint32_t ac, target_ulong rs, target_ulong rt)
{
    uint16_t rsh, rsl, rth, rtl;
    int32_t  tempA, tempB;
    int64_t  acc;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB = (int32_t)rsh * (int32_t)rth;
    tempA = (int32_t)rsl * (int32_t)rtl;

    acc = ((uint64_t)env->active_tc.HI[ac] << 32) |
          ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    acc += (int64_t)tempB + (int64_t)tempA;

    env->active_tc.HI[ac] = (target_long)(int32_t)((acc & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(acc & MIPSDSP_LLO);
}

#if defined(TARGET_MIPS64)
void helper_dpa_w_qh(CPUMIPSState *env,
                     target_ulong rs, target_ulong rt, uint32_t ac)
{
    int32_t rs3, rs2, rs1, rs0;
    int32_t rt3, rt2, rt1, rt0;
    int32_t tempD, tempC, tempB, tempA;
    int64_t acc[2];
    int64_t temp[2];
    int64_t temp_sum;

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;

    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = rs3 * rt3;
    tempC = rs2 * rt2;
    tempB = rs1 * rt1;
    tempA = rs0 * rt0;

    temp[0] = (int64_t)tempD + (int64_t)tempC +
              (int64_t)tempB + (int64_t)tempA;
    temp[0] = (int64_t)(temp[0] << 31) >> 31;

    if (((temp[0] >> 63) & 0x01) == 0) {
        temp[1] = 0;
    } else {
        temp[1] = ~0ull;
    }

    acc[1] = env->active_tc.HI[ac];
    acc[0] = env->active_tc.LO[ac];

    temp_sum = acc[0] + temp[0];
    if (((uint64_t)temp_sum < (uint64_t)acc[0]) &&
       ((uint64_t)temp_sum < (uint64_t)temp[0])) {
        temp[1] += 1;
    }
    temp[0] = temp_sum;
    temp[1] += acc[1];

    env->active_tc.HI[ac] = temp[1];
    env->active_tc.LO[ac] = temp[0];
}
#endif

void helper_dpax_w_ph(CPUMIPSState *env,
                      uint32_t ac, target_ulong rs, target_ulong rt)
{
    uint16_t rsh, rsl, rth, rtl;
    int32_t tempB, tempA;
    int64_t acc, dotp;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB  = (uint32_t)rsh * (uint32_t)rth;
    tempA  = (uint32_t)rsl * (uint32_t)rtl;
    dotp =  (int64_t)tempB + (int64_t)tempA;
    acc  =  ((uint64_t)env->active_tc.HI[ac] << 32) |
            ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    acc  += dotp;

    env->active_tc.HI[ac] = (target_long)(int32_t)((acc & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(acc & MIPSDSP_LLO);
}

void helper_dpaq_s_w_ph(CPUMIPSState *env,
                        uint32_t ac, target_ulong rs, target_ulong rt)
{
    int16_t rsh, rsl, rth, rtl;
    int32_t tempB, tempA;
    int64_t acc, dotp;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB = mipsdsp_mul_q15_q15(env, ac, rsh, rth);
    tempA = mipsdsp_mul_q15_q15(env, ac, rsl, rtl);
    dotp = (int64_t)tempB + (int64_t)tempA;
    acc = ((uint64_t)env->active_tc.HI[ac] << 32) |
          ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    acc += dotp;

    env->active_tc.HI[ac] = (target_long)(int32_t)((acc & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(acc & MIPSDSP_LLO);
}

#if defined(TARGET_MIPS64)
void helper_dpaq_s_w_qh(CPUMIPSState *env,
                        target_ulong rs, target_ulong rt, uint32_t ac)
{
    int32_t rs3, rs2, rs1, rs0;
    int32_t rt3, rt2, rt1, rt0;
    int32_t tempD, tempC, tempB, tempA;
    int64_t temp[2];
    int64_t acc[2];
    int64_t temp_sum;

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_mul_q15_q15(env, ac, rs3, rt3);
    tempC = mipsdsp_mul_q15_q15(env, ac, rs2, rt2);
    tempB = mipsdsp_mul_q15_q15(env, ac, rs1, rt1);
    tempA = mipsdsp_mul_q15_q15(env, ac, rs0, rt0);

    temp[0] = (int64_t)tempD + (int64_t)tempC +
              (int64_t)tempB + (int64_t)tempA;
    if (((temp[0] >> 63) & 0x01) == 0) {
        temp[1] = 0x00;
    } else {
        temp[1] = ~0ull;
    }

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];

    temp_sum = temp[0] + acc[0];
    if ((temp_sum < temp[0]) && (temp_sum < acc[0])) {
        temp[1] += 1;
    }
    temp[0] = temp_sum;
    temp[1] += acc[1];

    env->active_tc.HI[ac] = temp[1];
    env->active_tc.LO[ac] = temp[0];
}
#endif

void helper_dpaqx_s_w_ph(CPUMIPSState *env,
                         uint32_t ac, target_ulong rs, target_ulong rt)
{
    uint16_t rsh, rsl, rth, rtl;
    int32_t tempB, tempA;
    int64_t acc, dotp;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB = mipsdsp_mul_q15_q15(env, ac, rsh, rtl);
    tempA = mipsdsp_mul_q15_q15(env, ac, rsl, rth);
    dotp = (int64_t)tempB + (int64_t)tempA;
    acc = ((uint64_t)env->active_tc.HI[ac] << 32) |
          ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    acc += dotp;

    env->active_tc.HI[ac] = (target_long)(int32_t)((acc & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(acc & MIPSDSP_LLO);
}

void helper_dpaqx_sa_w_ph(CPUMIPSState *env,
                          uint32_t ac, target_ulong rs, target_ulong rt)
{
    int16_t rsh, rsl, rth, rtl;
    int32_t tempB, tempA, tempC62_31, tempC63;
    int64_t acc, dotp, tempC;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB = mipsdsp_mul_q15_q15(env, ac, rsh, rtl);
    tempA = mipsdsp_mul_q15_q15(env, ac, rsl, rth);
    dotp = (int64_t)tempB + (int64_t)tempA;
    acc = ((uint64_t)env->active_tc.HI[ac] << 32) |
          ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    tempC = acc + dotp;
    tempC63 = (tempC >> 63) & 0x01;
    tempC62_31 = (tempC >> 31) & 0xFFFFFFFF;

    if ((tempC63 == 0) && (tempC62_31 != 0x00000000)) {
        tempC = 0x7FFFFFFF;
        set_DSPControl_overflow_flag(env, 1, 16 + ac);
    }

    if ((tempC63 == 1) && (tempC62_31 != 0xFFFFFFFF)) {
        tempC = 0xFFFFFFFF80000000ull;
        set_DSPControl_overflow_flag(env, 1, 16 + ac);
    }

    env->active_tc.HI[ac] = (target_long)(int32_t)((tempC & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(tempC & MIPSDSP_LLO);
}

void helper_dps_w_ph(CPUMIPSState *env,
                     uint32_t ac, target_ulong rs, target_ulong rt)
{
    uint16_t rsh, rsl, rth, rtl;
    int32_t tempB, tempA;
    int64_t acc, dotp;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB  = (int32_t)rsh * (int32_t)rth;
    tempA  = (int32_t)rsl * (int32_t)rtl;
    dotp =  (int64_t)tempB + (int64_t)tempA;
    acc  =  ((uint64_t)env->active_tc.HI[ac] << 32) |
            ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    acc  -= dotp;

    env->active_tc.HI[ac] = (target_long)(int32_t)((acc & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(acc & MIPSDSP_LLO);
}

#if defined(TARGET_MIPS64)
void helper_dps_w_qh(CPUMIPSState *env,
                     target_ulong rs, target_ulong rt, uint32_t ac)
{
    int16_t rs3, rs2, rs1, rs0;
    int16_t rt3, rt2, rt1, rt0;
    int32_t tempD, tempC, tempB, tempA;
    int64_t acc[2];
    int64_t temp[2];
    int64_t temp_sum;

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = (int32_t)rs3 * (int32_t)rt3;
    tempC = (int32_t)rs2 * (int32_t)rt2;
    tempB = (int32_t)rs1 * (int32_t)rt1;
    tempA = (int32_t)rs0 * (int32_t)rt0;

    temp[0] = (int64_t)tempD + (int64_t)tempC +
              (int64_t)tempB + (int64_t)tempA;

    temp[0] = (int64_t)(temp[0] << 31) >> 31;
    if (((temp[0] >> 32) & 0x01) == 0) {
        temp[1] = 0x00;
    } else {
        temp[1] = ~0ull;
    }

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];

    temp_sum = acc[0] - temp[0];
    if ((uint64_t)temp_sum > (uint64_t)acc[0]) {
        acc[1] -= 1;
    }
    acc[0] = temp_sum;
    acc[1] -= temp[1];

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}
#endif

void helper_dpsx_w_ph(CPUMIPSState *env,
                      uint32_t ac, target_ulong rs, target_ulong rt)
{
    uint16_t rsh, rsl, rth, rtl;
    int32_t  tempB,  tempA;
    int64_t acc, dotp;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB = (int32_t)rsh * (int32_t)rtl;
    tempA = (int32_t)rsl * (int32_t)rth;
    dotp = (int64_t)tempB + (int64_t)tempA;

    acc  = ((uint64_t)env->active_tc.HI[ac] << 32) |
           ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    acc -= dotp;
    env->active_tc.HI[ac] = (target_long)(int32_t)((acc & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(acc & MIPSDSP_LLO);
}

void helper_dpsq_s_w_ph(CPUMIPSState *env,
                        uint32_t ac, target_ulong rs, target_ulong rt)
{
    int16_t rsh, rsl, rth, rtl;
    int32_t tempB, tempA;
    int64_t acc, dotp;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB = mipsdsp_mul_q15_q15(env, ac, rsh, rth);
    tempA = mipsdsp_mul_q15_q15(env, ac, rsl, rtl);
    dotp = (int64_t)tempB + (int64_t)tempA;
    acc = ((uint64_t)env->active_tc.HI[ac] << 32) |
           ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    acc -= dotp;

    env->active_tc.HI[ac] = (target_long)(int32_t)((acc & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(acc & MIPSDSP_LLO);
}

#if defined(TARGET_MIPS64)
void helper_dpsq_s_w_qh(CPUMIPSState *env,
                        target_ulong rs, target_ulong rt, uint32_t ac)
{
    int16_t rs3, rs2, rs1, rs0;
    int16_t rt3, rt2, rt1, rt0;
    int32_t tempD, tempC, tempB, tempA;
    int64_t acc[2];
    int64_t temp[2];
    int64_t temp_sum;

    temp[0] = 0;
    temp[1] = 0;

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_mul_q15_q15(env, ac, rs3, rt3);
    tempC = mipsdsp_mul_q15_q15(env, ac, rs2, rt2);
    tempB = mipsdsp_mul_q15_q15(env, ac, rs1, rt1);
    tempA = mipsdsp_mul_q15_q15(env, ac, rs0, rt0);

    temp[0] = (int64_t)tempD + (int64_t)tempC +
              (int64_t)tempB + (int64_t)tempA;
    if (((temp[0] >> 63) & 0x01) == 0) {
        temp[1] = 0x00;
    } else {
        temp[1] = ~0ull;
    }

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];

    temp_sum = acc[0] - temp[0];
    if ((uint64_t)temp_sum > (uint64_t)acc[0]) {
        acc[1] -= 1;
    }
    acc[0] = temp_sum;
    acc[1] -= temp[1];

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}
#endif

void helper_dpsqx_s_w_ph(CPUMIPSState *env,
                         uint32_t ac, target_ulong rs, target_ulong rt)
{
    int16_t rsh, rsl, rth, rtl;
    int32_t tempB, tempA;
    int64_t dotp, tempC;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB = mipsdsp_mul_q15_q15(env, ac, rsh, rtl);
    tempA = mipsdsp_mul_q15_q15(env, ac, rsl, rth);
    dotp = (int64_t)tempB + (int64_t)tempA;
    tempC = (((uint64_t)env->active_tc.HI[ac] << 32) |
            ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO)) - dotp;

    env->active_tc.HI[ac] = (target_long)(int32_t)((tempC & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(tempC & MIPSDSP_LLO);
}

void helper_dpsqx_sa_w_ph(CPUMIPSState *env,
                          uint32_t ac, target_ulong rs, target_ulong rt)
{
    int16_t rsh, rsl, rth, rtl;
    int32_t tempB, tempA, tempC63, tempC62_31;
    int64_t dotp, tempC;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;
    tempB = mipsdsp_mul_q15_q15(env, ac, rsh, rtl);
    tempA = mipsdsp_mul_q15_q15(env, ac, rsl, rth);

    dotp   = (int64_t)tempB + (int64_t)tempA;
    tempC  = ((uint64_t)env->active_tc.HI[ac] << 32) |
             ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    tempC -= dotp;

    tempC63 = (tempC >> 63) & 0x01;
    tempC62_31 = (tempC >> 31) & 0xFFFFFFFF;

    if ((tempC63 == 0) && (tempC62_31 != 0)) {
        tempC = 0x7FFFFFFF;
        set_DSPControl_overflow_flag(env, 1, 16 + ac);
    }

    if ((tempC63 == 1) && (tempC62_31 != 0xFFFFFFFF)) {
        tempC = 0xFFFFFFFF80000000ull;
        set_DSPControl_overflow_flag(env, 1, 16 + ac);
    }

    env->active_tc.HI[ac] = (target_long)(int32_t)((tempC & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(tempC & MIPSDSP_LLO);
}

void helper_mulsaq_s_w_ph(CPUMIPSState *env,
                          uint32_t ac, target_ulong rs, target_ulong rt)
{
    int16_t rsh, rsl, rth, rtl;
    int32_t tempB, tempA;
    int64_t acc, dotp;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB = mipsdsp_mul_q15_q15(env, ac, rsh, rth);
    tempA = mipsdsp_mul_q15_q15(env, ac, rsl, rtl);
    dotp = (int64_t)tempB - (int64_t)tempA;
    acc = ((uint64_t)env->active_tc.HI[ac] << 32) |
          ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    dotp = dotp + acc;
    env->active_tc.HI[ac] = (target_long)(int32_t)((dotp & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(dotp & MIPSDSP_LLO);
}

#if defined(TARGET_MIPS64)
void helper_mulsaq_s_w_qh(CPUMIPSState *env,
                          target_ulong rs, target_ulong rt, uint32_t ac)
{
    int16_t rs3, rs2, rs1, rs0;
    int16_t rt3, rt2, rt1, rt0;
    int32_t tempD, tempC, tempB, tempA;
    int64_t acc[2];
    int64_t temp[2];
    int64_t temp_sum;

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempD = mipsdsp_mul_q15_q15(env, ac, rs3, rt3);
    tempC = mipsdsp_mul_q15_q15(env, ac, rs2, rt2);
    tempB = mipsdsp_mul_q15_q15(env, ac, rs1, rt1);
    tempA = mipsdsp_mul_q15_q15(env, ac, rs0, rt0);

    temp[0] = ((int32_t)tempD - (int32_t)tempC) +
              ((int32_t)tempB - (int32_t)tempA);
    temp[0] = (int64_t)(temp[0] << 30) >> 30;
    if (((temp[0] >> 33) & 0x01) == 0) {
        temp[1] = 0x00;
    } else {
        temp[1] = ~0ull;
    }

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];

    temp_sum = acc[0] + temp[0];
    if (((uint64_t)temp_sum < (uint64_t)acc[0]) &&
       ((uint64_t)temp_sum < (uint64_t)temp[0])) {
        acc[1] += 1;
    }
    acc[0] = temp_sum;
    acc[1] += temp[1];

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}
#endif

void helper_dpaq_sa_l_w(CPUMIPSState *env,
                        uint32_t ac, target_ulong rs, target_ulong rt)
{
    int32_t temp64, temp63, tempacc63, tempdotp63, tempDL63;
    int64_t dotp, acc;
    int64_t tempDL[2];
    uint64_t temp;

    dotp = mipsdsp_mul_q31_q31(env, ac, rs, rt);
    acc = ((uint64_t)env->active_tc.HI[ac] << 32) |
          ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    tempDL[0] = acc + dotp;

    tempacc63  = (acc >> 63) & 0x01;
    tempdotp63 = (dotp >> 63) & 0x01;
    tempDL63   = (tempDL[0] >> 63) & 0x01;

    if (((tempacc63 == 1) && (tempdotp63 == 1)) |
        (((tempacc63 == 1) || (tempdotp63 == 1)) && tempDL63 == 0)) {
        tempDL[1] = 1;
    } else {
        tempDL[1] = 0;
    }

    temp = tempDL[0];
    temp64 = tempDL[1] & 0x01;
    temp63 = (tempDL[0] >> 63) & 0x01;

    if (temp64 != temp63) {
        if (temp64 == 1) {
            temp = 0x8000000000000000ull;
        } else {
            temp = 0x7FFFFFFFFFFFFFFFull;
        }

        set_DSPControl_overflow_flag(env, 1, 16 + ac);
    }

    env->active_tc.HI[ac] = (target_long)(int32_t)((temp & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(temp & MIPSDSP_LLO);
}

#if defined(TARGET_MIPS64)
void helper_dpaq_sa_l_pw(CPUMIPSState *env,
                         target_ulong rs, target_ulong rt, uint32_t ac)
{
    int32_t rs1, rs0;
    int32_t rt1, rt0;
    int64_t tempB[2], tempA[2];
    int64_t temp[2];
    int64_t acc[2];
    int64_t temp_sum;

    temp[0] = 0;
    temp[1] = 0;

    rs1 = (rs >> 32) & MIPSDSP_LLO;
    rs0 = rs & MIPSDSP_LLO;
    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB[0] = mipsdsp_mul_q31_q31(env, ac, rs1, rt1);
    tempA[0] = mipsdsp_mul_q31_q31(env, ac, rs0, rt0);

    if (((tempB[0] >> 63) & 0x01) == 0) {
        tempB[1] = 0x00;
    } else {
        tempB[1] = ~0ull;
    }

    if (((tempA[0] >> 63) & 0x01) == 0) {
        tempA[1] = 0x00;
    } else {
        tempA[1] = ~0ull;
    }

    temp_sum = tempB[0] + tempA[0];
    if (((uint64_t)temp_sum < (uint64_t)tempB[0]) &&
       ((uint64_t)temp_sum < (uint64_t)tempA[0])) {
        temp[1] += 1;
    }
    temp[0] = temp_sum;
    temp[1] += tempB[1] + tempA[1];

    mipsdsp_sat64_acc_add_q63(env, acc, ac, temp);

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}
#endif

void helper_dpsq_sa_l_w(CPUMIPSState *env,
                        uint32_t ac, target_ulong rs, target_ulong rt)
{
    int32_t temp64, temp63, tempacc63, tempdotp63, tempDL63;
    int64_t dotp, acc;
    int64_t tempDL[2];
    uint64_t temp;

    dotp = mipsdsp_mul_q31_q31(env, ac, rs, rt);
    acc = ((uint64_t)env->active_tc.HI[ac] << 32) |
          ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    tempDL[0] = acc - dotp;

    tempacc63  = (acc >> 63) & 0x01;
    tempdotp63 = (dotp >> 63) & 0x01;
    tempDL63   = (tempDL[0] >> 63) & 0x01;

    if (((tempacc63 == 1) && (tempdotp63 == 0)) |
        (((tempacc63 == 1) || (tempdotp63 == 0)) && tempDL63 == 0)) {
        tempDL[1] = 1;
    } else {
        tempDL[1] = 0;
    }

    temp = tempDL[0];
    temp64 = tempDL[1] & 0x01;
    temp63 = (tempDL[0] >> 63) & 0x01;
    if (temp64 != temp63) {
        if (temp64 == 1) {
            temp = 0x8000000000000000ull;
        } else {
            temp = 0x7FFFFFFFFFFFFFFFull;
        }
        set_DSPControl_overflow_flag(env, 1, ac + 16);
    }

    env->active_tc.HI[ac] = (target_long)(int32_t)((temp & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(temp & MIPSDSP_LLO);
}

#if defined(TARGET_MIPS64)
void helper_dpsq_sa_l_pw(CPUMIPSState *env,
                         target_ulong rs, target_ulong rt, uint32_t ac)
{
    int32_t rs1, rs0;
    int32_t rt1, rt0;
    int64_t tempB[2], tempA[2];
    int64_t temp[2];
    int64_t acc[2];
    int64_t temp_sum;

    temp[0] = 0x00;
    temp[1] = 0x00;

    rs1 = (rs >> 32) & MIPSDSP_LLO;
    rs0 = rs & MIPSDSP_LLO;
    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB[0] = mipsdsp_mul_q31_q31(env, ac, rs1, rt1);
    tempA[0] = mipsdsp_mul_q31_q31(env, ac, rs0, rt0);

    if (((tempB[0] >> 31) & 0x01) == 0) {
        tempB[1] = 0x00;
    } else {
        tempB[1] = ~0ull;
    }

    if (((tempA[0] >> 31) & 0x01) == 0) {
        tempA[1] = 0x00;
    } else {
        tempA[1] = ~0ull;
    }

    temp_sum = tempB[0] + tempA[0];
    if (((uint64_t)temp_sum < (uint64_t)tempB[0]) &&
       ((uint64_t)temp_sum < (uint64_t)tempA[0])) {
        temp[1] += 1;
    }
    temp[0] = temp_sum;
    temp[1] += tempA[1] + tempB[1];

    mipsdsp_sat64_acc_sub_q63(env, acc, ac, temp);

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}

void helper_mulsaq_s_l_pw(CPUMIPSState *env,
                          target_ulong rs, target_ulong rt, uint32_t ac)
{
    int32_t rs1, rs0;
    int32_t rt1, rt0;
    int64_t tempB[2], tempA[2];
    int64_t temp[2];
    int64_t acc[2];
    int64_t temp_sum;

    rs1 = (rs >> 32) & MIPSDSP_LLO;
    rs0 = rs & MIPSDSP_LLO;
    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB[0] = mipsdsp_mul_q31_q31(env, ac, rs1, rt1);
    tempA[0] = mipsdsp_mul_q31_q31(env, ac, rs0, rt0);

    if (((tempB[0] >> 63) & 0x01) == 0) {
        tempB[1] = 0x00;
    } else {
        tempB[1] = ~0ull;
    }

    if (((tempA[0] >> 63) & 0x01) == 0) {
        tempA[1] = 0x00;
    } else {
        tempA[1] = ~0ull;
    }

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];

    temp_sum = tempB[0] - tempA[0];
    if ((uint64_t)temp_sum > (uint64_t)tempB[0]) {
        tempB[1] -= 1;
    }
    temp[0] = temp_sum;
    temp[1] = tempB[1] - tempA[1];

    if ((temp[1] & 0x01) == 0) {
        temp[1] = 0x00;
    } else {
        temp[1] = ~0ull;
    }

    temp_sum = acc[0] + temp[0];
    if (((uint64_t)temp_sum < (uint64_t)acc[0]) &&
       ((uint64_t)temp_sum < (uint64_t)temp[0])) {
        acc[1] += 1;
    }
    acc[0] = temp_sum;
    acc[1] += temp[1];

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}
#endif

void helper_maq_s_w_phl(CPUMIPSState *env,
                        uint32_t ac, target_ulong rs, target_ulong rt)
{
    int16_t rsh, rth;
    int32_t  tempA;
    int64_t tempL, acc;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rth = (rt & MIPSDSP_HI) >> 16;
    tempA  = mipsdsp_mul_q15_q15(env, ac, rsh, rth);
    acc = ((uint64_t)env->active_tc.HI[ac] << 32) |
          ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    tempL  = (int64_t)tempA + acc;
    env->active_tc.HI[ac] = (target_long)(int32_t)((tempL & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(tempL & MIPSDSP_LLO);
}

void helper_maq_s_w_phr(CPUMIPSState *env,
                        uint32_t ac, target_ulong rs, target_ulong rt)
{
    int16_t rsl, rtl;
    int32_t tempA;
    int64_t tempL, acc;

    rsl = rs & MIPSDSP_LO;
    rtl = rt & MIPSDSP_LO;
    tempA  = mipsdsp_mul_q15_q15(env, ac, rsl, rtl);
    acc = ((uint64_t)env->active_tc.HI[ac] << 32) |
          ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    tempL = (int64_t)tempA + acc;

    env->active_tc.HI[ac] = (target_long)(int32_t)((tempL & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(tempL & MIPSDSP_LLO);
}

void helper_maq_sa_w_phl(CPUMIPSState *env, uint32_t ac,
                         target_ulong rs, target_ulong rt)
{
    int16_t rsh, rth;
    int32_t tempA;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rth = (rt & MIPSDSP_HI) >> 16;
    tempA = mipsdsp_mul_q15_q15(env, ac, rsh, rth);
    tempA = mipsdsp_sat32_acc_q31(env, ac, tempA);

    env->active_tc.HI[ac] = (target_long)(int32_t)(((int64_t)tempA & \
                                                    MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)((int64_t)tempA & \
                                                   MIPSDSP_LLO);
}

void helper_maq_sa_w_phr(CPUMIPSState *env, uint32_t ac,
                         target_ulong rs, target_ulong rt)
{
    int16_t rsl, rtl;
    int32_t tempA;

    rsl = rs & MIPSDSP_LO;
    rtl = rt & MIPSDSP_LO;

    tempA = mipsdsp_mul_q15_q15(env, ac, rsl, rtl);
    tempA = mipsdsp_sat32_acc_q31(env, ac, tempA);

    env->active_tc.HI[ac] = (target_long)(int32_t)(((int64_t)tempA & \
                                                    MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)((int64_t)tempA & \
                                                   MIPSDSP_LLO);
}

/***************************************************************
 * In manual, GPR[rd](..0) <- tempB(15..0) || tempA(15..0),
 * I'm not sure its means zero extend or sign extend.
 * Now treat it as zero extend.
 ***************************************************************/
target_ulong helper_mul_ph(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    int16_t rsh, rsl, rth, rtl;
    int32_t tempB, tempA;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;
    tempB = mipsdsp_mul_i16_i16(env, rsh, rth);
    tempA = mipsdsp_mul_i16_i16(env, rsl, rtl);

    return (target_ulong)(uint32_t)(((tempB & MIPSDSP_LO) << 16) | \
                                    (tempA & MIPSDSP_LO));
}

/***************************************************************
 * In manual, GPR[rd](..0) <- tempB(15..0) || tempA(15..0),
 * I'm not sure its means zero extend or sign extend.
 * Now treat it as zero extend.
 ***************************************************************/
target_ulong helper_mul_s_ph(CPUMIPSState *env,
                             target_ulong rs, target_ulong rt)
{
    int16_t  rsh, rsl, rth, rtl;
    int32_t  tempB, tempA;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;
    tempB = mipsdsp_sat16_mul_i16_i16(env, rsh, rth);
    tempA = mipsdsp_sat16_mul_i16_i16(env, rsl, rtl);

    return (target_ulong)(((tempB & MIPSDSP_LO) << 16) | (tempA & MIPSDSP_LO));
}

target_ulong helper_mulq_s_ph(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    int16_t rsh, rsl, rth, rtl;
    int32_t temp, tempB, tempA;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB = mipsdsp_sat16_mul_q15_q15(env, rsh, rth);
    tempA = mipsdsp_sat16_mul_q15_q15(env, rsl, rtl);
    temp = ((tempB & MIPSDSP_LO) << 16) | (tempA & MIPSDSP_LO);

    return (target_long)temp;
}

target_ulong helper_mulq_s_w(CPUMIPSState *env,
                             target_ulong rs, target_ulong rt)
{
    int32_t tempI;
    int64_t tempL;

    if ((rs == 0x80000000) && (rt == 0x80000000)) {
        tempL = 0x7FFFFFFF00000000ull;
        set_DSPControl_overflow_flag(env, 1, 21);
    } else {
        tempL  = ((int64_t)rs * (int64_t)rt) << 1;
    }
    tempI = (tempL & MIPSDSP_LHI) >> 32;

    return (target_long)(int32_t)tempI;
}

target_ulong helper_mulq_rs_w(CPUMIPSState *env,
                              target_ulong rs, target_ulong rt)
{
    uint32_t rs_t, rt_t;
    uint32_t tempI;
    int64_t tempL;

    rs_t = rs & MIPSDSP_LLO;
    rt_t = rt & MIPSDSP_LLO;

    if ((rs_t == 0x80000000) && (rt_t == 0x80000000)) {
        tempL = 0x7FFFFFFF00000000ull;
        set_DSPControl_overflow_flag(env, 1, 21);
    } else {
        tempL  = ((int64_t)rs_t * (int64_t)rt_t) << 1;
        tempL += 0x80000000ull;
    }
    tempI = (tempL & MIPSDSP_LHI) >> 32;

    return (target_long)(int32_t)tempI;
}

void helper_mulsa_w_ph(CPUMIPSState *env,
                       uint32_t ac, target_ulong rs, target_ulong rt)
{
    uint16_t rsh, rsl, rth, rtl;
    int32_t tempB, tempA;
    int64_t dotp, acc;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    tempB = (int32_t)rsh * (int32_t)rth;
    tempA = (int32_t)rsl * (int32_t)rtl;

    dotp = (int64_t)tempB - (int64_t)tempA;
    acc  = ((int64_t)env->active_tc.HI[ac] << 32) |
           ((int64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    acc = acc + dotp;

    env->active_tc.HI[ac] = (target_long)(int32_t)((acc & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(acc & MIPSDSP_LLO);
}

#if defined(TARGET_MIPS64)
void helper_maq_s_w_qhll(CPUMIPSState *env,
                         target_ulong rs, target_ulong rt, uint32_t ac)
{
    int16_t rs_t, rt_t;
    int32_t temp_mul;
    int64_t temp[2];
    int64_t acc[2];
    int64_t temp_sum;

    temp[0] = 0;
    temp[1] = 0;

    rs_t = (rs >> 48) & MIPSDSP_LO;
    rt_t = (rt >> 48) & MIPSDSP_LO;
    temp_mul = mipsdsp_mul_q15_q15(env, ac, rs_t, rt_t);

    temp[0] = (int64_t)temp_mul;
    if (((temp[0] >> 63) & 0x01) == 0) {
        temp[1] = 0x00;
    } else {
        temp[1] = ~0ull;
    }

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];

    temp_sum = acc[0] + temp[0];
    if (((uint64_t)temp_sum < (uint64_t)acc[0]) &&
       ((uint64_t)temp_sum < (uint64_t)temp[0])) {
        acc[1] += 1;
    }
    acc[0] = temp_sum;
    acc[1] += temp[1];

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}

void helper_maq_s_w_qhlr(CPUMIPSState *env,
                         target_ulong rs, target_ulong rt, uint32_t ac)
{
    int16_t rs_t, rt_t;
    int32_t temp_mul;
    int64_t temp[2];
    int64_t acc[2];
    int64_t temp_sum;

    temp[0] = 0;
    temp[1] = 0;

    rs_t = (rs >> 32) & MIPSDSP_LO;
    rt_t = (rt >> 32) & MIPSDSP_LO;
    temp_mul = mipsdsp_mul_q15_q15(env, ac, rs_t, rt_t);

    temp[0] = (int64_t)temp_mul;
    if (((temp[0] >> 63) & 0x01) == 0) {
        temp[1] = 0x00;
    } else {
        temp[1] = ~0ull;
    }

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];

    temp_sum = acc[0] + temp[0];
    if (((uint64_t)temp_sum < (uint64_t)acc[0]) &&
       ((uint64_t)temp_sum < (uint64_t)temp[0])) {
        acc[1] += 1;
    }
    acc[0] = temp_sum;
    acc[1] += temp[1];

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}

void helper_maq_s_w_qhrl(CPUMIPSState *env,
                         target_ulong rs, target_ulong rt, uint32_t ac)
{
    int16_t rs_t, rt_t;
    int32_t temp_mul;
    int64_t temp[2];
    int64_t acc[2];
    int64_t temp_sum;

    temp[0] = 0;
    temp[1] = 0;

    rs_t = (rs >> 16) & MIPSDSP_LO;
    rt_t = (rt >> 16) & MIPSDSP_LO;
    temp_mul = mipsdsp_mul_q15_q15(env, ac, rs_t, rt_t);

    temp[0] = (int64_t)temp_mul;
    if (((temp[0] >> 63) & 0x01) == 0) {
        temp[1] = 0x00;
    } else {
        temp[1] = ~0ull;
    }

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];

    temp_sum = acc[0] + temp[0];
    if (((uint64_t)temp_sum < (uint64_t)acc[0]) &&
       ((uint64_t)temp_sum < (uint64_t)temp[0])) {
        acc[1] += 1;
    }
    acc[0] = temp_sum;
    acc[1] += temp[1];

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}

void helper_maq_s_w_qhrr(CPUMIPSState *env,
                         target_ulong rs, target_ulong rt, uint32_t ac)
{
    int16_t rs_t, rt_t;
    int32_t temp_mul;
    int64_t temp[2];
    int64_t acc[2];
    int64_t temp_sum;

    temp[0] = 0;
    temp[1] = 0;

    rs_t = rs & MIPSDSP_LO;
    rt_t = rt & MIPSDSP_LO;
    temp_mul = mipsdsp_mul_q15_q15(env, ac, rs_t, rt_t);

    temp[0] = (int64_t)temp_mul;
    if (((temp[0] >> 63) & 0x01) == 0) {
        temp[1] = 0x00;
    } else {
        temp[1] = ~0ull;
    }

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];

    temp_sum = acc[0] + temp[0];
    if (((uint64_t)temp_sum < (uint64_t)acc[0]) &&
       ((uint64_t)temp_sum < (uint64_t)temp[0])) {
        acc[1] += 1;
    }
    acc[0] = temp_sum;
    acc[1] += temp[1];

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}

void helper_maq_sa_w_qhll(CPUMIPSState *env,
                          target_ulong rs, target_ulong rt, uint32_t ac)
{
    int16_t rs_t, rt_t;
    int32_t temp;
    int64_t acc[2];

    rs_t = (rs >> 48) & MIPSDSP_LO;
    rt_t = (rt >> 48) & MIPSDSP_LO;
    temp = mipsdsp_mul_q15_q15(env, ac, rs_t, rt_t);
    temp = mipsdsp_sat32_acc_q31(env, ac, temp);

    acc[0] = (int64_t)(int32_t)temp;
    if (((acc[0] >> 63) & 0x01) == 0) {
        acc[1] = 0x00;
    } else {
        acc[1] = ~0ull;
    }

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}

void helper_maq_sa_w_qhlr(CPUMIPSState *env,
                          target_ulong rs, target_ulong rt, uint32_t ac)
{
    int16_t rs_t, rt_t;
    int32_t temp;
    int64_t acc[2];

    rs_t = (rs >> 32) & MIPSDSP_LO;
    rt_t = (rt >> 32) & MIPSDSP_LO;
    temp = mipsdsp_mul_q15_q15(env, ac, rs_t, rt_t);
    temp = mipsdsp_sat32_acc_q31(env, ac, temp);

    acc[0] = (int64_t)temp;
    if (((acc[0] >> 63) & 0x01) == 0) {
        acc[1] = 0x00;
    } else {
        acc[1] = ~0ull;
    }

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}

void helper_maq_sa_w_qhrl(CPUMIPSState *env,
                          target_ulong rs, target_ulong rt, uint32_t ac)
{
    int16_t rs_t, rt_t;
    int32_t temp;
    int64_t acc[2];

    rs_t = (rs >> 16) & MIPSDSP_LO;
    rt_t = (rt >> 16) & MIPSDSP_LO;
    temp = mipsdsp_mul_q15_q15(env, ac, rs_t, rt_t);
    temp = mipsdsp_sat32_acc_q31(env, ac, temp);

    acc[0] = (int64_t)temp;
    if (((acc[0] >> 63) & 0x01) == 0) {
        acc[1] = 0x00;
    } else {
        acc[1] = ~0ull;
    }

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}

void helper_maq_sa_w_qhrr(CPUMIPSState *env,
                          target_ulong rs, target_ulong rt, uint32_t ac)
{
    int16_t rs_t, rt_t;
    int32_t temp;
    int64_t acc[2];

    rs_t = rs & MIPSDSP_LO;
    rt_t = rt & MIPSDSP_LO;
    temp = mipsdsp_mul_q15_q15(env, ac, rs_t, rt_t);
    temp = mipsdsp_sat32_acc_q31(env, ac, temp);

    acc[0] = (int64_t)temp;
    if (((acc[0] >> 63) & 0x01) == 0) {
        acc[1] = 0x00;
    } else {
        acc[1] = ~0ull;
    }

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}

void helper_maq_s_l_pwl(CPUMIPSState *env,
                        target_ulong rs, target_ulong rt, uint32_t ac)
{
    int32_t rs_t, rt_t;
    int64_t temp[2];
    int64_t acc[2];
    int64_t temp_sum;

    temp[0] = 0;
    temp[1] = 0;

    rs_t = (rs >> 32) & MIPSDSP_LLO;
    rt_t = (rt >> 32) & MIPSDSP_LLO;

    temp[0] = mipsdsp_mul_q31_q31(env, ac, rs_t, rt_t);
    if (((temp[0] >> 63) & 0x01) == 0) {
        temp[1] = 0x00;
    } else {
        temp[1] = ~0ull;
    }

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];

    temp_sum = acc[0] + temp[0];
    if (((uint64_t)temp_sum < (uint64_t)temp[0]) &&
       ((uint64_t)temp_sum < (uint64_t)acc[0])) {
        acc[1] += 1;
    }
    acc[0] = temp_sum;
    acc[1] += temp[1];

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}

void helper_maq_s_l_pwr(CPUMIPSState *env,
                        target_ulong rs, target_ulong rt, uint32_t ac)
{
    int32_t rs_t, rt_t;
    int64_t temp[2];
    int64_t acc[2];
    int64_t temp_sum;

    temp[0] = 0;
    temp[1] = 0;

    rs_t = rs & MIPSDSP_LLO;
    rt_t = rt & MIPSDSP_LLO;

    temp[0] = mipsdsp_mul_q31_q31(env, ac, rs_t, rt_t);
    if (((temp[0] >> 63) & 0x01) == 0) {
        temp[1] = 0x00;
    } else {
        temp[1] = ~0ull;
    }

    acc[0] = env->active_tc.LO[ac];
    acc[1] = env->active_tc.HI[ac];

    temp_sum = acc[0] + temp[0];
    if (((uint64_t)temp_sum < (uint64_t)temp[0]) &&
       ((uint64_t)temp_sum < (uint64_t)acc[0])) {
        acc[1] += 1;
    }
    acc[0] = temp_sum;
    acc[1] += temp[1];

    env->active_tc.HI[ac] = acc[1];
    env->active_tc.LO[ac] = acc[0];
}

void helper_dmadd(CPUMIPSState *env,
                  target_ulong rs, target_ulong rt, uint32_t ac)
{
    int32_t rs1, rs0;
    int32_t rt1, rt0;
    int32_t tempB, tempA;
    int64_t tempBL[2], tempAL[2];
    int64_t acc[2];
    int64_t temp[2];
    int64_t temp_sum;

    temp[0] = 0x00;
    temp[1] = 0x00;

    rs1 = (rs >> 32) & MIPSDSP_LLO;
    rs0 = rs & MIPSDSP_LLO;
    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB = rs1 * rt1;
    tempA = rs0 * rt0;

    tempBL[0] = (int64_t)tempB;
    tempAL[0] = (int64_t)tempA;

    if (((tempBL[0] >> 63) & 0x01) == 0) {
        tempBL[1] = 0x0;
    } else {
        tempBL[1] = ~0ull;
    }

    if (((tempAL[0] >> 63) & 0x01) == 0) {
        tempAL[1] = 0x0;
    } else {
        tempAL[1] = ~0ull;
    }

    acc[1] = env->active_tc.HI[ac];
    acc[0] = env->active_tc.LO[ac];

    temp_sum = tempBL[0] + tempAL[0];
    if (((uint64_t)temp_sum < (uint64_t)tempBL[0]) &&
       ((uint64_t)temp_sum < (uint64_t)tempAL[0])) {
        temp[1] += 1;
    }
    temp[0] = temp_sum;
    temp[1] += tempBL[1] + tempAL[1];

    temp_sum = temp[0] + acc[0];
    if (((uint64_t)temp_sum < (uint64_t)temp[0]) &&
       ((uint64_t)temp_sum < (uint64_t)acc[0])) {
        temp[1] += 1;
    }
    temp[0] = temp_sum;
    temp[1] += acc[1];

    env->active_tc.HI[ac] = temp[1];
    env->active_tc.LO[ac] = temp[0];
}

void helper_dmaddu(CPUMIPSState *env,
                   target_ulong rs, target_ulong rt, uint32_t ac)
{
    uint32_t rs1, rs0;
    uint32_t rt1, rt0;
    uint64_t tempBL[2], tempAL[2];
    uint64_t acc[2];
    uint64_t temp[2];
    uint64_t temp_sum;

    temp[0] = 0x00;
    temp[1] = 0x00;

    rs1 = (rs >> 32) & MIPSDSP_LLO;
    rs0 = rs & MIPSDSP_LLO;
    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempBL[0] = mipsdsp_mul_u32_u32(env, rs1, rt1);
    tempAL[0] = mipsdsp_mul_u32_u32(env, rs0, rt0);
    tempBL[1] = 0;
    tempAL[1] = 0;

    acc[1] = env->active_tc.HI[ac];
    acc[0] = env->active_tc.LO[ac];

    temp_sum = tempBL[0] + tempAL[0];
    if ((temp_sum < tempBL[0]) && (temp_sum < tempAL[0])) {
        temp[1] += 1;
    }
    temp[0] = temp_sum;
    temp[1] += tempBL[1] + tempAL[1];

    temp_sum = temp[0] + acc[0];
    if ((temp_sum < temp[0]) && (temp_sum < acc[0])) {
        temp[1] += 1;
    }
    temp[0] = temp_sum;
    temp[1] += acc[1];

    env->active_tc.HI[ac] = temp[1];
    env->active_tc.LO[ac] = temp[0];
}

void helper_dmsub(CPUMIPSState *env,
                  target_ulong rs, target_ulong rt, uint32_t ac)
{
    int32_t rs1, rs0;
    int32_t rt1, rt0;
    int32_t tempB, tempA;
    int64_t tempBL[2], tempAL[2];
    int64_t acc[2];
    int64_t temp[2];
    int64_t temp_sum;

    temp[0] = 0x00;
    temp[1] = 0x00;

    rs1 = (rs >> 32) & MIPSDSP_LLO;
    rs0 = rs & MIPSDSP_LLO;
    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempB = rs1 * rt1;
    tempA = rs0 * rt0;

    tempBL[0] = (int64_t)tempB;
    tempAL[0] = (int64_t)tempA;

    if (((tempBL[0] >> 63) & 0x01) == 0) {
        tempBL[1] = 0x0;
    } else {
        tempBL[1] = ~0ull;
    }

    if (((tempAL[0] >> 63) & 0x01) == 0) {
        tempAL[1] = 0x0;
    } else {
        tempAL[1] = ~0ull;
    }

    acc[1] = env->active_tc.HI[ac];
    acc[0] = env->active_tc.LO[ac];

    temp_sum = acc[0] - tempBL[0];
    if ((uint64_t)temp_sum > (uint64_t)acc[0]) {
        temp[1] -= 1;
    }
    temp[0] = temp_sum;
    temp[1] += acc[1] - tempBL[1];

    temp_sum = temp[0] - tempAL[0];
    if ((uint64_t)temp_sum > (uint64_t)temp[0]) {
        temp[1] -= 1;
    }
    temp[0] = temp_sum;
    temp[1] -= tempAL[1];

    env->active_tc.HI[ac] = temp[1];
    env->active_tc.LO[ac] = temp[0];
}

void helper_dmsubu(CPUMIPSState *env,
                   target_ulong rs, target_ulong rt, uint32_t ac)
{
    uint32_t rs1, rs0;
    uint32_t rt1, rt0;
    uint64_t tempBL[2], tempAL[2];
    uint64_t acc[2];
    uint64_t temp[2];
    uint64_t temp_sum;

    temp[0] = 0x00;
    temp[1] = 0x00;

    rs1 = (rs >> 32) & MIPSDSP_LLO;
    rs0 = rs & MIPSDSP_LLO;
    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempBL[0] = mipsdsp_mul_u32_u32(env, rs1, rt1);
    tempAL[0] = mipsdsp_mul_u32_u32(env, rs0, rt0);
    tempBL[1] = 0;
    tempAL[1] = 0;

    acc[1] = env->active_tc.HI[ac];
    acc[0] = env->active_tc.LO[ac];

    temp_sum = acc[0] - tempBL[0];
    if (temp_sum > acc[0]) {
        temp[1] -= 1;
    }
    temp[0] = temp_sum;
    temp[1] += acc[1] - tempBL[1];

    temp_sum = temp[0] - tempAL[0];
    if ((uint64_t)temp_sum > (uint64_t)temp[0]) {
        temp[1] -= 1;
    }
    temp[0] = temp_sum;
    temp[1] -= tempAL[1];

    env->active_tc.HI[ac] = temp[1];
    env->active_tc.LO[ac] = temp[0];
}
#endif

/** DSP Bit/Manipulation Sub-class insns **/
target_ulong helper_bitrev(target_ulong rt)
{
    int32_t temp;
    uint32_t rd;
    int i, last;

    temp = rt & MIPSDSP_LO;
    rd = 0;
    for (i = 0; i < 16; i++) {
        last = temp % 2;
        temp = temp >> 1;
        rd = rd | (last << (15 - i));
    }

    return (target_ulong)rd;
}

target_ulong helper_insv(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint32_t pos, size, msb, lsb, filter;
    uint32_t temp, temprs, temprt;
    target_ulong dspc;

    dspc = env->active_tc.DSPControl;
    pos  = dspc & 0x1F;
    size = (dspc >> 7) & 0x1F;
    msb  = pos + size - 1;
    lsb  = pos;

    if (lsb > msb) {
        return rt;
    }

    filter = ((int32_t)0x01 << size) - 1;
    filter = filter << pos;
    temprs = rs & filter;
    temprt = rt & ~filter;
    temp = temprs | temprt;

    return (target_long)(int32_t)temp;
}

#if defined(TARGET_MIPS64)
target_ulong helper_dinsv(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    target_ulong dspctrl;
    target_ulong filter;
    uint8_t pos, size;
    uint8_t msb, lsb;
    uint64_t temp;

    temp = rt;
    dspctrl = env->active_tc.DSPControl;
    pos = dspctrl & 0x7F;
    size = (dspctrl >> 7) & 0x3F;

    msb = pos + size - 1;
    lsb = pos;

    if ((lsb > msb) || (msb > 63)) {
        return temp;
    }

    temp = 0;
    filter = ((target_ulong)0x01 << size) - 1;
    filter = filter << pos;

    temp |= rs & filter;
    temp |= rt & (~filter);

    return temp;
}
#endif

/** DSP Compare-Pick Sub-class insns **/
#define CMPU_QB(name) \
void helper_cmpu_ ## name ## _qb(CPUMIPSState *env, target_ulong rs, \
                                 target_ulong rt)     \
{                                                     \
    uint8_t rs3, rs2, rs1, rs0;                       \
    uint8_t rt3, rt2, rt1, rt0;                       \
    uint32_t cc3 = 0, cc2 = 0, cc1 = 0, cc0 = 0;      \
    uint32_t flag;                                    \
                                                      \
    rs3 = (rs & MIPSDSP_Q3) >> 24;                    \
    rs2 = (rs & MIPSDSP_Q2) >> 16;                    \
    rs1 = (rs & MIPSDSP_Q1) >>  8;                    \
    rs0 =  rs & MIPSDSP_Q0;                           \
                                                      \
    rt3 = (rt & MIPSDSP_Q3) >> 24;                    \
    rt2 = (rt & MIPSDSP_Q2) >> 16;                    \
    rt1 = (rt & MIPSDSP_Q1) >>  8;                    \
    rt0 =  rt & MIPSDSP_Q0;                           \
                                                      \
    cc3 = mipsdsp_cmp_##name(rs3, rt3);                \
    cc2 = mipsdsp_cmp_##name(rs2, rt2);                \
    cc1 = mipsdsp_cmp_##name(rs1, rt1);                \
    cc0 = mipsdsp_cmp_##name(rs0, rt0);                \
                                                      \
    flag = (cc3 << 3) | (cc2 << 2) | (cc1 << 1) | cc0;\
    set_DSPControl_24(env, flag, 4);                  \
}

CMPU_QB(eq)
CMPU_QB(lt)
CMPU_QB(le)

#define CMPGU_QB(name) \
target_ulong helper_cmpgu_ ## name ## _qb(target_ulong rs, target_ulong rt) \
{                                                       \
    uint8_t rs3, rs2, rs1, rs0;                         \
    uint8_t rt3, rt2, rt1, rt0;                         \
    uint8_t cc3 = 0, cc2 = 0, cc1 = 0, cc0 = 0;         \
    uint32_t temp;                                      \
                                                        \
    rs3 = (rs & MIPSDSP_Q3) >> 24;                      \
    rs2 = (rs & MIPSDSP_Q2) >> 16;                      \
    rs1 = (rs & MIPSDSP_Q1) >>  8;                      \
    rs0 =  rs & MIPSDSP_Q0;                             \
                                                        \
    rt3 = (rt & MIPSDSP_Q3) >> 24;                      \
    rt2 = (rt & MIPSDSP_Q2) >> 16;                      \
    rt1 = (rt & MIPSDSP_Q1) >>  8;                      \
    rt0 =  rt & MIPSDSP_Q0;                             \
                                                        \
    cc3 = mipsdsp_cmp_##name(rs3, rt3);                  \
    cc2 = mipsdsp_cmp_##name(rs2, rt2);                  \
    cc1 = mipsdsp_cmp_##name(rs1, rt1);                  \
    cc0 = mipsdsp_cmp_##name(rs0, rt0);                  \
                                                        \
    temp = (cc3 << 3) | (cc2 << 2) | (cc1 << 1) | cc0;  \
                                                        \
    return (target_ulong)temp;                          \
}

CMPGU_QB(eq)
CMPGU_QB(lt)
CMPGU_QB(le)

#define CMP_PH(name) \
void helper_cmp_##name##_ph(CPUMIPSState *env, target_ulong rs, \
                            target_ulong rt)       \
{                                                  \
    int16_t rsh, rsl, rth, rtl;                    \
    int32_t flag;                                  \
    int32_t ccA = 0, ccB = 0;                      \
                                                   \
    rsh = (rs & MIPSDSP_HI) >> 16;                 \
    rsl =  rs & MIPSDSP_LO;                        \
    rth = (rt & MIPSDSP_HI) >> 16;                 \
    rtl =  rt & MIPSDSP_LO;                        \
                                                   \
    ccB = mipsdsp_cmp_##name(rsh, rth);            \
    ccA = mipsdsp_cmp_##name(rsl, rtl);            \
                                                   \
    flag = (ccB << 1) | ccA;                       \
    set_DSPControl_24(env, flag, 2);               \
}

CMP_PH(eq)
CMP_PH(lt)
CMP_PH(le)

#if defined(TARGET_MIPS64)
#define CMPU_OB(name) \
void helper_cmpu_ ## name ##_ob(CPUMIPSState *env, target_ulong rs, \
                                target_ulong rt)        \
{                                                       \
    int i;                                              \
    uint8_t rs_t[8], rt_t[8];                           \
    uint32_t cond;                                      \
                                                        \
    cond = 0;                                           \
                                                        \
    for (i = 0; i < 8; i++) {                           \
        rs_t[i] = (rs >> (8 * i)) & MIPSDSP_Q0;         \
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;         \
                                                        \
        if (mipsdsp_cmp_##name(rs_t[i], rt_t[i])) {                     \
            cond |= 0x01 << i;                          \
        }                                               \
    }                                                   \
                                                        \
    set_DSPControl_24(env, cond, 8);                    \
}

CMPU_OB(eq)
CMPU_OB(lt)
CMPU_OB(le)

#define CMPGDU_OB(name) \
target_ulong helper_cmpgdu_##name##_ob(CPUMIPSState *env,           \
                                 target_ulong rs, target_ulong rt)  \
{                                                     \
    int i;                                            \
    uint8_t rs_t[8], rt_t[8];                         \
    uint32_t cond;                                    \
                                                      \
    cond = 0;                                         \
                                                      \
    for (i = 0; i < 8; i++) {                         \
        rs_t[i] = (rs >> (8 * i)) & MIPSDSP_Q0;       \
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;       \
                                                      \
        if (mipsdsp_cmp_##name(rs_t[i], rt_t[i])) {   \
            cond |= 0x01 << i;                        \
        }                                             \
    }                                                 \
                                                      \
    set_DSPControl_24(env, cond, 8);                  \
                                                      \
    return (uint64_t)cond;                            \
}

CMPGDU_OB(eq)
CMPGDU_OB(lt)
CMPGDU_OB(le)

#define CMPGU_OB(name) \
target_ulong helper_cmpgu_##name##_ob(target_ulong rs, target_ulong rt) \
{                                                                       \
    int i;                                                              \
    uint8_t rs_t[8], rt_t[8];                                           \
    uint32_t cond;                                                      \
                                                                        \
    cond = 0;                                                           \
                                                                        \
    for (i = 0; i < 8; i++) {                                           \
        rs_t[i] = (rs >> (8 * i)) & MIPSDSP_Q0;                         \
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;                         \
                                                                        \
        if (mipsdsp_cmp_##name(rs_t[i], rt_t[i])) {                     \
            cond |= 0x01 << i;                                          \
        }                                                               \
    }                                                                   \
                                                                        \
    return (uint64_t)cond;                                              \
}

CMPGU_OB(eq)
CMPGU_OB(lt)
CMPGU_OB(le)


#define CMP_QH(name) \
void helper_cmp_##name##_qh(CPUMIPSState *env, target_ulong rs, \
                            target_ulong rt)                    \
{                                                               \
    uint16_t rs3, rs2, rs1, rs0;                                \
    uint16_t rt3, rt2, rt1, rt0;                                \
    uint32_t cond;                                              \
                                                                \
    cond = 0;                                                   \
                                                                \
    rs3 = (rs >> 48) & MIPSDSP_LO;                              \
    rs2 = (rs >> 32) & MIPSDSP_LO;                              \
    rs1 = (rs >> 16) & MIPSDSP_LO;                              \
    rs0 = rs & MIPSDSP_LO;                                      \
    rt3 = (rt >> 48) & MIPSDSP_LO;                              \
    rt2 = (rt >> 32) & MIPSDSP_LO;                              \
    rt1 = (rt >> 16) & MIPSDSP_LO;                              \
    rt0 = rt & MIPSDSP_LO;                                      \
                                                                \
    if (mipsdsp_cmp_##name(rs3, rt3)) {                         \
        cond |= 0x08;                                           \
    }                                                           \
                                                                \
    if (mipsdsp_cmp_##name(rs2, rt2)) {                         \
        cond |= 0x04;                                           \
    }                                                           \
                                                                \
    if (mipsdsp_cmp_##name(rs1, rt1)) {                         \
        cond |= 0x02;                                           \
    }                                                           \
                                                                \
    if (mipsdsp_cmp_##name(rs0, rt0)) {                         \
        cond |= 0x01;                                           \
    }                                                           \
                                                                \
    set_DSPControl_24(env, cond, 4);                            \
}

CMP_QH(eq)
CMP_QH(lt)
CMP_QH(le)

#define CMP_PW(name) \
void helper_cmp_## name ##_pw(CPUMIPSState *env, target_ulong rs, \
                              target_ulong rt)                    \
{                                                                 \
    uint32_t rs1, rs0;                                            \
    uint32_t rt1, rt0;                                            \
    uint32_t cond;                                                \
                                                                  \
    cond = 0;                                                     \
                                                                  \
    rs1 = (rs >> 32) & MIPSDSP_LLO;                               \
    rs0 = rs & MIPSDSP_LLO;                                       \
    rt1 = (rt >> 32) & MIPSDSP_LLO;                               \
    rt0 = rt & MIPSDSP_LLO;                                       \
                                                                  \
    if (mipsdsp_cmp_##name(rs1, rt1)) {                           \
        cond |= 0x2;                                              \
    }                                                             \
                                                                  \
    if (mipsdsp_cmp_##name(rs0, rt0)) {                           \
        cond |= 0x1;                                              \
    }                                                             \
                                                                  \
    set_DSPControl_24(env, cond, 2);                              \
}

CMP_PW(eq)
CMP_PW(lt)
CMP_PW(le)
#endif

target_ulong helper_pick_qb(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint8_t rs3, rs2, rs1, rs0;
    uint8_t rt3, rt2, rt1, rt0;
    uint8_t tp3, tp2, tp1, tp0;

    uint32_t dsp27, dsp26, dsp25, dsp24, rd;
    target_ulong dsp;

    rs3 = (rs & MIPSDSP_Q3) >> 24;
    rs2 = (rs & MIPSDSP_Q2) >> 16;
    rs1 = (rs & MIPSDSP_Q1) >>  8;
    rs0 =  rs & MIPSDSP_Q0;
    rt3 = (rt & MIPSDSP_Q3) >> 24;
    rt2 = (rt & MIPSDSP_Q2) >> 16;
    rt1 = (rt & MIPSDSP_Q1) >>  8;
    rt0 =  rt & MIPSDSP_Q0;

    dsp = env->active_tc.DSPControl;
    dsp27 = (dsp >> 27) & 0x01;
    dsp26 = (dsp >> 26) & 0x01;
    dsp25 = (dsp >> 25) & 0x01;
    dsp24 = (dsp >> 24) & 0x01;

    tp3 = dsp27 == 1 ? rs3 : rt3;
    tp2 = dsp26 == 1 ? rs2 : rt2;
    tp1 = dsp25 == 1 ? rs1 : rt1;
    tp0 = dsp24 == 1 ? rs0 : rt0;

    rd = ((uint32_t)tp3 << 24) |
         ((uint32_t)tp2 << 16) |
         ((uint32_t)tp1 <<  8) |
         (uint32_t)tp0;

    return (target_long)(int32_t)rd;
}

target_ulong helper_pick_ph(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint16_t rsh, rsl, rth, rtl;
    uint16_t tempB, tempA;
    uint32_t dsp25, dsp24;
    uint32_t rd;
    target_ulong dsp;

    rsh = (rs & MIPSDSP_HI) >> 16;
    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;
    rtl =  rt & MIPSDSP_LO;

    dsp = env->active_tc.DSPControl;
    dsp25 = (dsp >> 25) & 0x01;
    dsp24 = (dsp >> 24) & 0x01;

    tempB = (dsp25 == 1) ? rsh : rth;
    tempA = (dsp24 == 1) ? rsl : rtl;
    rd = (((uint32_t)tempB << 16) & MIPSDSP_HI) | (uint32_t)tempA;

    return (target_long)(int32_t)rd;
}

#if defined(TARGET_MIPS64)
target_ulong helper_pick_ob(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    int i;
    uint32_t cond;
    uint8_t rs_t[8], rt_t[8];
    uint8_t temp[8];
    uint64_t result;

    result = 0;
    cond = get_DSPControl_24(env, 8);

    for (i = 0; i < 8; i++) {
        rs_t[i] = (rs >> (8 * i)) & MIPSDSP_Q0;
        rt_t[i] = (rt >> (8 * i)) & MIPSDSP_Q0;

        temp[i] = (cond % 2) == 1 ? rs_t[i] : rt_t[i];
        cond = cond / 2;
    }

    for (i = 0; i < 8; i++) {
        result |= (uint64_t)temp[i] << (8 * i);
    }

    return result;
}

target_ulong helper_pick_qh(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint32_t cond;
    uint16_t rs3, rs2, rs1, rs0;
    uint16_t rt3, rt2, rt1, rt0;
    uint16_t tempD, tempC, tempB, tempA;
    uint64_t result;

    cond = get_DSPControl_24(env, 4);

    rs3 = (rs >> 48) & MIPSDSP_LO;
    rs2 = (rs >> 32) & MIPSDSP_LO;
    rs1 = (rs >> 16) & MIPSDSP_LO;
    rs0 = rs & MIPSDSP_LO;
    rt3 = (rt >> 48) & MIPSDSP_LO;
    rt2 = (rt >> 32) & MIPSDSP_LO;
    rt1 = (rt >> 16) & MIPSDSP_LO;
    rt0 = rt & MIPSDSP_LO;

    tempA = ((cond % 2) == 1) ? rs0 : rt0;
    cond = cond / 2;
    tempB = ((cond % 2) == 1) ? rs1 : rt1;
    cond = cond / 2;
    tempC = ((cond % 2) == 1) ? rs2 : rt2;
    cond = cond / 2;
    tempD = ((cond % 2) == 1) ? rs3 : rt3;

    result = ((uint64_t)tempD << 48) | ((uint64_t)tempC << 32) |
             ((uint64_t)tempB << 16) | (uint64_t)tempA;

    return result;
}

target_ulong helper_pick_pw(CPUMIPSState *env, target_ulong rs, target_ulong rt)
{
    uint32_t cond;
    uint32_t rs1, rs0;
    uint32_t rt1, rt0;
    uint32_t tempB, tempA;

    cond = get_DSPControl_24(env, 2);
    rs1 = (rs >> 32) & MIPSDSP_LLO;
    rs0 = rs & MIPSDSP_LLO;
    rt1 = (rt >> 32) & MIPSDSP_LLO;
    rt0 = rt & MIPSDSP_LLO;

    tempA = ((cond % 2) == 1) ? rs0 : rt0;
    cond = cond / 2;
    tempB = ((cond % 2) == 1) ? rs1 : rt1;

    return ((uint64_t)tempB << 32) | (uint64_t)tempA;
}
#endif

target_ulong helper_append(target_ulong rt, target_ulong rs, uint32_t sa)
{
    int len;
    uint32_t temp;

    len = sa & 0x1F;

    if (len == 0) {
        temp = rt;
    } else {
        temp = (rt << len) | (rs & (((uint32_t)0x01 << len) - 1));
    }

    return (target_long)(int32_t)temp;
}

#if defined(TARGET_MIPS64)
target_ulong helper_dappend(target_ulong rs, target_ulong rt, uint32_t sa)
{
    uint32_t filter;
    uint64_t temp;

    filter = 0;
    temp = 0;

    if (sa == 0) {
        temp = rt;
    } else {
        filter = (0x01 << sa) - 1;
        temp = rt << sa;
        temp |= rs & filter;
    }

    return temp;
}

#endif

target_ulong helper_prepend(uint32_t sa, target_ulong rs, target_ulong rt)
{
    return (target_long)(int32_t)(uint32_t)(((rs & MIPSDSP_LLO) << (32 - sa)) |\
                                            ((rt & MIPSDSP_LLO) >> sa));
}

#if defined(TARGET_MIPS64)
target_ulong helper_prependd(target_ulong rs, target_ulong rt, uint32_t sa)
{
    uint32_t shift;

    shift = sa & 0x10;
    shift |= 0x20;

    return (rs << (64 - shift)) | (rt >> shift);
}

target_ulong helper_prependw(target_ulong rs, target_ulong rt, uint32_t sa)
{
    uint32_t shift;

    shift = sa;

    return (rs << (64 - shift)) | (rt >> shift);
}
#endif

target_ulong helper_balign(uint32_t bp, target_ulong rt, target_ulong rs)
{
    bp = bp & 0x03;

    if (bp == 0 || bp == 2) {
        return rt;
    } else {
        return (target_long)(int32_t)((rt << (8 * bp)) | \
                                      (rs >> (8 * (4 - bp))));
    }
}

#if defined(TARGET_MIPS64)
target_ulong helper_dbalign(target_ulong rs, target_ulong rt, uint32_t sa)
{
    uint8_t bp;

    bp = sa & 0x07;

    if ((bp == 0) || (bp == 2) || (bp == 4)) {
        return rt;
    } else {
        return (rt << (8 * bp)) | (rs >> (8 * (8 - bp)));
    }
}

#endif

target_ulong helper_packrl_ph(target_ulong rs, target_ulong rt)
{
    uint32_t rsl, rth;

    rsl =  rs & MIPSDSP_LO;
    rth = (rt & MIPSDSP_HI) >> 16;

    return (target_long)(int32_t)((rsl << 16) | rth);
}

#if defined(TARGET_MIPS64)
target_ulong helper_packrl_pw(target_ulong rs, target_ulong rt)
{
    uint32_t rs0, rt1;

    rs0 = rs & MIPSDSP_LLO;
    rt1 = (rt >> 32) & MIPSDSP_LLO;

    return ((uint64_t)rs0 << 32) | (uint64_t)rt1;
}
#endif

/** DSP Accumulator and DSPControl Access Sub-class insns **/
target_ulong helper_extr_w(CPUMIPSState *env, uint32_t ac, uint32_t shift)
{
    int32_t tempI;
    int64_t tempDL[2];

    mipsdsp_rndrashift_short_acc(env, tempDL, ac, shift);
    if ((tempDL[1] != 0 || (tempDL[0] & MIPSDSP_LHI) != 0) &&
        (tempDL[1] != 1 || (tempDL[0] & MIPSDSP_LHI) != MIPSDSP_LHI)) {
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    tempI = (tempDL[0] >> 1) & MIPSDSP_LLO;

    tempDL[0] += 1;
    if (tempDL[0] == 0) {
        tempDL[1] += 1;
    }

    if ((!(tempDL[1] == 0 && (tempDL[0] & MIPSDSP_LHI) == 0x00)) &&
        (!(tempDL[1] == 1 && (tempDL[0] & MIPSDSP_LHI) == MIPSDSP_LHI))) {
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    return (target_long)tempI;
}

target_ulong helper_extr_r_w(CPUMIPSState *env, uint32_t ac, uint32_t shift)
{
    int64_t tempDL[2];

    mipsdsp_rndrashift_short_acc(env, tempDL, ac, shift);
    if ((tempDL[1] != 0 || (tempDL[0] & MIPSDSP_LHI) != 0) &&
        (tempDL[1] != 1 || (tempDL[0] & MIPSDSP_LHI) != MIPSDSP_LHI)) {
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    tempDL[0] += 1;
    if (tempDL[0] == 0) {
        tempDL[1] += 1;
    }

    if ((tempDL[1] != 0 || (tempDL[0] & MIPSDSP_LHI) != 0) &&
        (tempDL[1] != 1 && (tempDL[0] & MIPSDSP_LHI) != MIPSDSP_LHI)) {
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    return (target_long)(int32_t)(tempDL[0] >> 1);
}

target_ulong helper_extr_rs_w(CPUMIPSState *env, uint32_t ac, uint32_t shift)
{
    int32_t tempI, temp64;
    int64_t tempDL[2];

    mipsdsp_rndrashift_short_acc(env, tempDL, ac, shift);
    if ((tempDL[1] != 0 || (tempDL[0] & MIPSDSP_LHI) != 0) &&
        (tempDL[1] != 1 || (tempDL[0] & MIPSDSP_LHI) != MIPSDSP_LHI)) {
        set_DSPControl_overflow_flag(env, 1, 23);
    }
    tempDL[0] += 1;
    if (tempDL[0] == 0) {
        tempDL[1] += 1;
    }
    tempI = tempDL[0] >> 1;

    if ((tempDL[1] != 0 || (tempDL[0] & MIPSDSP_LHI) != 0) &&
        (tempDL[1] != 1 || (tempDL[0] & MIPSDSP_LHI) != MIPSDSP_LHI)) {
        temp64 = tempDL[1];
        if (temp64 == 0) {
            tempI = 0x7FFFFFFF;
        } else {
            tempI = 0x80000000;
        }
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    return (target_long)tempI;
}

#if defined(TARGET_MIPS64)
target_ulong helper_dextr_w(CPUMIPSState *env, uint32_t ac, uint32_t shift)
{
    uint64_t temp[3];

    mipsdsp_rndrashift_acc(env, temp, ac, shift);

    return (int64_t)(int32_t)(temp[0] >> 1);
}

target_ulong helper_dextr_r_w(CPUMIPSState *env, uint32_t ac, uint32_t shift)
{
    uint64_t temp[3];
    uint32_t temp128;

    mipsdsp_rndrashift_acc(env, temp, ac, shift);

    temp[0] += 1;
    if (temp[0] == 0) {
        temp[1] += 1;
        if (temp[1] == 0) {
            temp[2] += 1;
        }
    }

    temp128 = temp[2] & 0x01;

    if ((temp128 != 0 || temp[1] != 0) &&
       (temp128 != 1 || temp[1] != ~0ull)) {
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    return (int64_t)(int32_t)(temp[0] >> 1);
}

target_ulong helper_dextr_rs_w(CPUMIPSState *env, uint32_t ac, uint32_t shift)
{
    uint64_t temp[3];
    uint32_t temp128;

    mipsdsp_rndrashift_acc(env, temp, ac, shift);

    temp[0] += 1;
    if (temp[0] == 0) {
        temp[1] += 1;
        if (temp[1] == 0) {
            temp[2] += 1;
        }
    }

    temp128 = temp[2] & 0x01;

    if ((temp128 != 0 || temp[1] != 0) &&
       (temp128 != 1 || temp[1] != ~0ull)) {
        if (temp128 == 0) {
            temp[0] = 0x0FFFFFFFF;
        } else {
            temp[0] = 0x0100000000;
        }
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    return (int64_t)(int32_t)(temp[0] >> 1);
}

target_ulong helper_dextr_l(CPUMIPSState *env, uint32_t ac, uint32_t shift)
{
    uint64_t temp[3];
    target_ulong result;

    mipsdsp_rndrashift_acc(env, temp, ac, shift);
    result = (temp[1] << 63) | (temp[0] >> 1);

    return result;
}

target_ulong helper_dextr_r_l(CPUMIPSState *env, uint32_t ac, uint32_t shift)
{
    uint64_t temp[3];
    uint32_t temp128;
    target_ulong result;

    mipsdsp_rndrashift_acc(env, temp, ac, shift);

    temp[0] += 1;
    if (temp[0] == 0) {
        temp[1] += 1;
        if (temp[1] == 0) {
            temp[2] += 1;
        }
    }

    temp128 = temp[2] & 0x01;

    if ((temp128 != 0 || temp[1] != 0) &&
       (temp128 != 1 || temp[1] != ~0ull)) {
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    result = (temp[1] << 63) | (temp[0] >> 1);

    return result;
}

target_ulong helper_dextr_rs_l(CPUMIPSState *env, uint32_t ac, uint32_t shift)
{
    uint64_t temp[3];
    uint32_t temp128;
    target_ulong result;

    mipsdsp_rndrashift_acc(env, temp, ac, shift);

    temp[0] += 1;
    if (temp[0] == 0) {
        temp[1] += 1;
        if (temp[1] == 0) {
            temp[2] += 1;
        }
    }

    temp128 = temp[2] & 0x01;

    if ((temp128 != 0 || temp[1] != 0) &&
       (temp128 != 1 || temp[1] != ~0ull)) {
        if (temp128 == 0) {
            temp[1] &= 0xFFFFFFFFFFFFFFFEull;
            temp[0] |= 0xFFFFFFFFFFFFFFFEull;
        } else {
            temp[1] |= 0x01;
            temp[0] &= 0x01;
        }
        set_DSPControl_overflow_flag(env, 1, 23);
    }
    result = (temp[1] << 63) | (temp[0] >> 1);

    return result;
}
#endif

target_ulong helper_extr_s_h(CPUMIPSState *env, uint32_t ac, uint32_t shift)
{
    int64_t temp;

    temp = mipsdsp_rashift_short_acc(env, ac, shift);
    if (temp > 0x0000000000007FFFull) {
        temp = 0x00007FFF;
        set_DSPControl_overflow_flag(env, 1, 23);
    } else if (temp < 0xFFFFFFFFFFFF8000ull) {
        temp = 0xFFFF8000;
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    return (target_long)(int32_t)(temp & 0xFFFFFFFF);
}

target_ulong helper_extrv_s_h(CPUMIPSState *env, uint32_t ac, target_ulong rs)
{
    int32_t shift, tempI;
    int64_t tempL;

    shift = rs & 0x0F;
    tempL = mipsdsp_rashift_short_acc(env, ac, shift);
    if (tempL > 0x000000000007FFFull) {
        tempI = 0x00007FFF;
        set_DSPControl_overflow_flag(env, 1, 23);
    } else if (tempL < 0xFFFFFFFFFFF8000ull) {
        tempI = 0xFFFF8000;
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    return (target_long)(int32_t)tempI;
}

#if defined(TARGET_MIPS64)
target_ulong helper_dextr_s_h(CPUMIPSState *env, uint32_t ac, uint32_t shift)
{
    int64_t temp[2];
    uint32_t temp127;

    mipsdsp_rashift_acc(env, (uint64_t *)temp, ac, shift);

    temp127 = (temp[1] >> 63) & 0x01;

    if ((temp127 == 0) && (temp[1] > 0 || temp[0] > 32767)) {
        temp[0] &= 0xFFFF0000;
        temp[0] |= 0x00007FFF;
        set_DSPControl_overflow_flag(env, 1, 23);
    } else if ((temp127 == 1) &&
            (temp[1] < 0xFFFFFFFFFFFFFFFFll
             || temp[0] < 0xFFFFFFFFFFFF1000ll)) {
        temp[0] &= 0xFFFF0000;
        temp[0] |= 0x00008000;
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    return (int64_t)(int16_t)(temp[0] & MIPSDSP_LO);
}

target_ulong helper_dextrv_s_h(CPUMIPSState *env,
                               uint32_t ac, target_ulong shift)
{
    int64_t temp[2];
    uint32_t temp127;

    shift = shift & 0x1F;
    mipsdsp_rashift_acc(env, (uint64_t *)temp, ac, shift);

    temp127 = (temp[1] >> 63) & 0x01;

    if ((temp127 == 0) && (temp[1] > 0 || temp[0] > 32767)) {
        temp[0] &= 0xFFFF0000;
        temp[0] |= 0x00007FFF;
        set_DSPControl_overflow_flag(env, 1, 23);
    } else if ((temp127 == 1) &&
            (temp[1] < ~0ull || temp[0] < 0xFFFFFFFFFFFF1000ull)) {
        temp[0] &= 0xFFFF0000;
        temp[0] |= 0x00008000;
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    return (int64_t)(int16_t)(temp[0] & MIPSDSP_LO);
}
#endif

target_ulong helper_extrv_w(CPUMIPSState *env, uint32_t ac, target_ulong rs)
{
    int32_t shift, tempI;
    int64_t tempDL[2];

    shift = rs & 0x0F;
    mipsdsp_rndrashift_short_acc(env, tempDL, ac, shift);
    if ((tempDL[1] != 0 || (tempDL[0] & MIPSDSP_LHI) != 0) &&
        (tempDL[1] != 1 || (tempDL[0] & MIPSDSP_LHI) != MIPSDSP_LHI)) {
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    tempI = tempDL[0] >> 1;

    tempDL[0] += 1;
    if (tempDL[0] == 0) {
        tempDL[1] += 1;
    }
    if ((tempDL[1] != 0 || (tempDL[0] & MIPSDSP_LHI) != 0) &&
        (tempDL[1] != 1 || (tempDL[0] & MIPSDSP_LHI) != MIPSDSP_LHI)) {
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    return (target_long)tempI;
}

target_ulong helper_extrv_r_w(CPUMIPSState *env, uint32_t ac, target_ulong rs)
{
    int32_t shift;
    int64_t tempDL[2];

    shift = rs & 0x0F;
    mipsdsp_rndrashift_short_acc(env, tempDL, ac, shift);
    if ((tempDL[1] != 0 || (tempDL[0] & MIPSDSP_LHI) != 0) &&
        (tempDL[1] != 1 || (tempDL[0] & MIPSDSP_LHI) != MIPSDSP_LHI)) {
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    tempDL[0] += 1;
    if (tempDL[0] == 0) {
        tempDL[1] += 1;
    }

    if ((tempDL[1] != 0 || (tempDL[0] & MIPSDSP_LHI) != 0) &&
        (tempDL[1] != 1 || (tempDL[0] & MIPSDSP_LHI) != MIPSDSP_LHI)) {
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    return (target_long)(int32_t)(tempDL[0] >> 1);
}

target_ulong helper_extrv_rs_w(CPUMIPSState *env, uint32_t ac, target_ulong rs)
{
    int32_t shift, tempI;
    int64_t tempDL[2];

    shift = rs & 0x0F;
    mipsdsp_rndrashift_short_acc(env, tempDL, ac, shift);
    if ((tempDL[1] != 0 || (tempDL[0] & MIPSDSP_LHI) != 0) &&
        (tempDL[1] != 1 || (tempDL[0] & MIPSDSP_LHI) != MIPSDSP_LHI)) {
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    tempDL[0] += 1;
    if (tempDL[0] == 0) {
        tempDL[1] += 1;
    }
    tempI = tempDL[0] >> 1;

    if ((tempDL[1] != 0 || (tempDL[0] & MIPSDSP_LHI) != 0) &&
        (tempDL[1] != 1 || (tempDL[0] & MIPSDSP_LHI) != MIPSDSP_LHI)) {
        if (tempDL[1] == 0) {
            tempI = 0x7FFFFFFF;
        } else {
            tempI = 0x80000000;
        }
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    return (target_long)tempI;
}

#if defined(TARGET_MIPS64)
target_ulong helper_dextrv_w(CPUMIPSState *env, uint32_t ac, target_ulong shift)
{
    uint64_t temp[3];

    shift = shift & 0x1F;

    mipsdsp_rndrashift_acc(env, temp, ac, shift);

    return (int64_t)(int32_t)(temp[0] >> 1);
}

target_ulong helper_dextrv_r_w(CPUMIPSState *env,
                               uint32_t ac, target_ulong shift)
{
    uint64_t temp[3];
    uint32_t temp128;

    shift = shift & 0x1F;

    mipsdsp_rndrashift_acc(env, temp, ac, shift);

    temp[0] += 1;
    if (temp[0] == 0) {
        temp[1] += 1;
        if (temp[1] == 0) {
            temp[2] += 1;
        }
    }

    temp128 = temp[2] & 0x01;

    if ((temp128 != 0 || temp[1] != 0) &&
       (temp128 != 1 || temp[1] != ~0ull)) {
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    return (int64_t)(int32_t)(temp[0] >> 1);
}

target_ulong helper_dextrv_rs_w(CPUMIPSState *env,
                                uint32_t ac, target_ulong shift)
{
    uint64_t temp[3];
    uint32_t temp128;

    shift = shift & 0x1F;

    mipsdsp_rndrashift_acc(env, temp, ac, shift);

    temp[0] += 1;
    if (temp[0] == 0) {
        temp[1] += 1;
        if (temp[1] == 0) {
            temp[2] += 1;
        }
    }

    temp128 = temp[2] & 0x01;

    if ((temp128 != 0 || temp[1] != 0) &&
       (temp128 != 1 || temp[1] != ~0ull)) {
        if (temp128 == 0) {
            temp[0] |= 0x0FFFFFFFF;
        } else {
            temp[0] = 0x0100000000;
        }
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    return (int64_t)(int32_t)(temp[0] >> 1);
}

target_ulong helper_dextrv_l(CPUMIPSState *env, uint32_t ac, target_ulong shift)
{
    uint64_t temp[3];
    target_ulong result;

    shift = shift & 0x1F;

    mipsdsp_rndrashift_acc(env, temp, ac, shift);

    result = (temp[1] << 63) | (temp[0] >> 1);

    return result;
}

target_ulong helper_dextrv_r_l(CPUMIPSState *env,
                               uint32_t ac, target_ulong shift)
{
    uint64_t temp[3];
    uint32_t temp128;
    target_ulong result;

    shift = shift & 0x1F;

    mipsdsp_rndrashift_acc(env, temp, ac, shift);

    temp[0] += 1;
    if (temp[0] == 0) {
        temp[1] += 1;
        if (temp[1] == 0) {
            temp[2] += 1;
        }
    }

    temp128 = temp[2] & 0x01;

    if ((temp128 != 0 || temp[1] != 0) &&
       (temp128 != 1 || temp[1] != ~0ull)) {
        set_DSPControl_overflow_flag(env, 1, 23);
    }
    result = (temp[1] << 63) | (temp[0] >> 1);

    return result;
}

target_ulong helper_dextrv_rs_l(CPUMIPSState *env,
                                uint32_t ac, target_ulong shift)
{
    uint64_t temp[3];
    uint32_t temp128;
    target_ulong result;

    shift = shift & 0x1F;

    mipsdsp_rndrashift_acc(env, temp, ac, shift);

    temp[0] += 1;
    if (temp[0] == 0) {
        temp[1] += 1;
        if (temp[1] == 0) {
            temp[2] += 1;
        }
    }

    temp128 = temp[2] & 0x01;

    if ((temp128 != 0 || temp[1] != 0) &&
       (temp128 != 1 || temp[1] != ~0ull)) {
        if (temp128 == 0) {
            temp[1] &= 0xFFFFFFFFFFFFFFFEull;
            temp[0] |= 0xFFFFFFFFFFFFFFFEull;
        } else {
            temp[1] |= 0x01;
            temp[0] &= 0x01;
        }
        set_DSPControl_overflow_flag(env, 1, 23);
    }

    result = (temp[1] << 63) | (temp[0] >> 1);

    return result;
}
#endif

target_ulong helper_extp(CPUMIPSState *env, uint32_t ac, uint32_t size)
{
    int32_t start_pos;
    int sub;
    uint32_t temp;
    uint64_t acc;

    temp = 0;
    start_pos = get_DSPControl_pos(env);
    sub = start_pos - (size + 1);
    if (sub >= -1) {
        acc = ((uint64_t)env->active_tc.HI[ac] << 32) |
              ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
        temp = (acc >> (start_pos - size)) &
               (((uint32_t)0x01 << (size + 1)) - 1);
        set_DSPControl_efi(env, 0);
    } else {
        set_DSPControl_efi(env, 1);
    }

    return (target_ulong)temp;
}

target_ulong helper_extpv(CPUMIPSState *env, uint32_t ac, target_ulong rs)
{
    int32_t start_pos, size;
    uint32_t temp;
    uint64_t acc;

    temp = 0;
    start_pos = get_DSPControl_pos(env);
    size = rs & 0x1F;

    if (start_pos - (size + 1) >= -1) {
        acc = ((uint64_t)env->active_tc.HI[ac] << 32) |
              ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
        temp = (acc >> (start_pos - size)) &
               (((uint32_t)0x01 << (size + 1)) - 1);
        set_DSPControl_efi(env, 0);
    } else {
        set_DSPControl_efi(env, 1);
    }

    return (target_ulong)temp;
}

target_ulong helper_extpdp(CPUMIPSState *env, uint32_t ac, uint32_t size)
{
    int32_t start_pos;
    int sub;
    uint32_t temp;
    uint64_t acc;

    temp = 0;
    start_pos = get_DSPControl_pos(env);
    sub = start_pos - (size + 1);
    if (sub >= -1) {
        acc  = ((uint64_t)env->active_tc.HI[ac] << 32) |
               ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
        temp = (acc >> (start_pos - size)) &
               (((uint32_t)0x01 << (size + 1)) - 1);

        set_DSPControl_pos(env, start_pos - (size + 1));
        set_DSPControl_efi(env, 0);
    } else {
        set_DSPControl_efi(env, 1);
    }

    return (target_ulong)temp;
}

target_ulong helper_extpdpv(CPUMIPSState *env, uint32_t ac, target_ulong rs)
{
    int32_t start_pos, size;
    uint32_t temp;
    uint64_t acc;

    temp = 0;
    start_pos = get_DSPControl_pos(env);
    size = rs & 0x1F;

    if (start_pos - (size + 1) >= -1) {
        acc  = ((uint64_t)env->active_tc.HI[ac] << 32) |
               ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
        temp = (acc >> (start_pos - size)) & (((int)0x01 << (size + 1)) - 1);
        set_DSPControl_pos(env, start_pos - (size + 1));
        set_DSPControl_efi(env, 0);
    } else {
        set_DSPControl_efi(env, 1);
    }

    return (target_ulong)temp;
}

#if defined(TARGET_MIPS64)
target_ulong helper_dextp(CPUMIPSState *env, uint32_t ac, uint32_t size)
{
    int start_pos;
    int len;
    int sub;
    uint64_t tempB, tempA;
    uint64_t temp;

    temp = 0;

    size = size & 0x3F;
    start_pos = get_DSPControl_pos(env);
    len = start_pos - size;
    tempB = env->active_tc.HI[ac];
    tempA = env->active_tc.LO[ac];

    sub = start_pos - (size + 1);

    if (sub >= -1) {
        temp = (tempB << (64 - len)) | (tempA >> len);
        temp = temp & ((0x01 << (size + 1)) - 1);
        set_DSPControl_efi(env, 0);
    } else {
        set_DSPControl_efi(env, 1);
    }

    return temp;
}

target_ulong helper_dextpv(CPUMIPSState *env, uint32_t ac, target_ulong size)
{
    uint8_t start_pos;
    uint8_t len;
    int sub;
    uint64_t tempB, tempA;
    uint64_t temp;

    temp = 0;

    size = size & 0x3F;
    start_pos = get_DSPControl_pos(env);
    len = start_pos - size;
    tempB = env->active_tc.HI[ac];
    tempA = env->active_tc.LO[ac];

    sub = start_pos - (size + 1);

    if (sub >= -1) {
        temp = (tempB << (64 - len)) | (tempA >> len);
        temp = temp & ((0x01 << (size + 1)) - 1);
        set_DSPControl_efi(env, 0);
    } else {
        set_DSPControl_efi(env, 1);
    }

    return temp;
}

target_ulong helper_dextpdp(CPUMIPSState *env, uint32_t ac, uint32_t size)
{
    int start_pos;
    int len;
    int sub;
    uint64_t tempB, tempA;
    uint64_t temp;

    temp = 0;
    start_pos = get_DSPControl_pos(env);
    len = start_pos - size;
    tempB = env->active_tc.HI[ac];
    tempA = env->active_tc.LO[ac];

    sub = start_pos - (size + 1);

    if (sub >= -1) {
        temp = (tempB << (64 - len)) | (tempA >> len);
        temp = temp & ((0x01 << (size + 1)) - 1);
        set_DSPControl_pos(env, sub);
        set_DSPControl_efi(env, 0);
    } else {
        set_DSPControl_efi(env, 1);
    }

    return temp;
}

target_ulong helper_dextpdpv(CPUMIPSState *env, uint32_t ac, target_ulong size)
{
    int start_pos;
    int len;
    int sub;
    uint64_t tempB, tempA;
    uint64_t temp;

    temp = 0;
    start_pos = get_DSPControl_pos(env);
    size = size & 0x3F;
    len = start_pos - size;
    tempB = env->active_tc.HI[ac];
    tempA = env->active_tc.LO[ac];

    sub = start_pos - (size + 1);

    if (sub >= -1) {
        temp = (tempB << (64 - len)) | (tempA >> len);
        temp = temp & ((0x01 << (size + 1)) - 1);
        set_DSPControl_pos(env, sub);
        set_DSPControl_efi(env, 0);
    } else {
        set_DSPControl_efi(env, 1);
    }

    return temp;
}
#endif

void helper_shilo(CPUMIPSState *env, uint32_t ac, uint32_t shift)
{
    uint8_t  sign;
    uint64_t temp, acc;

    shift = (int32_t)((int32_t)shift << 26) >> 26;
    sign  = (shift >>  5) & 0x01;
    shift = (sign == 0) ? shift : -shift;
    acc = (((uint64_t)env->active_tc.HI[ac] << 32) & MIPSDSP_LHI) |
          ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);

    if (shift == 0) {
        temp = acc;
    } else {
        if (sign == 0) {
            temp = acc >> shift;
        } else {
            temp = acc << shift;
        }
    }

    env->active_tc.HI[ac] = (target_long)(int32_t)((temp & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_long)(int32_t)(temp & MIPSDSP_LLO);
}

void helper_shilov(CPUMIPSState *env, uint32_t ac, target_ulong rs)
{
    uint8_t sign;
    int8_t  rs5_0;
    uint64_t temp, acc;

    rs5_0 = rs & 0x3F;
    rs    = (int8_t)(rs5_0 << 2) >> 2;
    sign  = (rs5_0 >> 5) & 0x01;
    rs5_0 = (sign == 0) ? rs : -rs;
    acc   = (((uint64_t)env->active_tc.HI[ac] << 32) & MIPSDSP_LHI) |
            ((uint64_t)env->active_tc.LO[ac] & MIPSDSP_LLO);
    if (rs5_0 == 0) {
        temp = acc;
    } else {
        if (sign == 0) {
            temp = acc >> rs5_0;
        } else {
            temp = acc << rs5_0;
        }
    }

    env->active_tc.HI[ac] = (target_ulong)(int32_t)((temp & MIPSDSP_LHI) >> 32);
    env->active_tc.LO[ac] = (target_ulong)(int32_t)(temp & MIPSDSP_LLO);
}

#if defined(TARGET_MIPS64)
void helper_dshilo(CPUMIPSState *env, uint32_t shift, uint32_t ac)
{
    uint8_t shift6;
    int8_t shift_t;
    uint64_t tempB, tempA;

    shift6 = (shift >> 6) & 0x01;
    shift_t = (shift6 << 7) | (uint8_t)shift;
    shift_t = shift6 == 0 ? shift_t : -shift_t;

    tempB = env->active_tc.HI[ac];
    tempA = env->active_tc.LO[ac];

    if (shift_t != 0) {
        if (shift6 == 0) {
            tempA = (tempB << (64 - shift)) | (tempA >> shift);
            tempB = tempB >> shift;
        } else {
            tempB = (tempB << shift) | (tempA >> (64 - shift));
            tempA = tempA << shift;
        }
    }

    env->active_tc.HI[ac] = tempB;
    env->active_tc.LO[ac] = tempA;
}

void helper_dshilov(CPUMIPSState *env, target_ulong shift, uint32_t ac)
{
    uint8_t shift6;
    int8_t shift_t;
    uint64_t tempB, tempA;

    shift6 = (shift >> 6) & 0x01;
    shift_t = (shift6 << 7) | ((uint8_t)shift & 0x7F);
    shift_t = shift6 == 0 ? shift_t : -shift_t;

    tempB = env->active_tc.HI[ac];
    tempA = env->active_tc.LO[ac];

    if (shift_t != 0) {
        if (shift6 == 0) {
            tempA = (tempB << (64 - shift)) | (tempA >> shift);
            tempB = tempB >> shift;
        } else {
            tempB = (tempB << shift) | (tempA >> (64 - shift));
            tempA = tempA << shift;
        }
    }

    env->active_tc.HI[ac] = tempB;
    env->active_tc.LO[ac] = tempA;
}
#endif
void helper_mthlip(CPUMIPSState *env, uint32_t ac, target_ulong rs)
{
    int32_t tempA, tempB, pos;

    tempA = rs;
    tempB = env->active_tc.LO[ac];
    env->active_tc.HI[ac] = (target_long)tempB;
    env->active_tc.LO[ac] = (target_long)tempA;
    pos = get_DSPControl_pos(env);

    if (pos > 32) {
        return;
    } else {
        set_DSPControl_pos(env, pos + 32);
    }
}

#if defined(TARGET_MIPS64)
void helper_dmthlip(CPUMIPSState *env, target_ulong rs, uint32_t ac)
{
    uint8_t ac_t;
    uint8_t pos;
    uint64_t tempB, tempA;

    ac_t = ac & 0x3;

    tempA = rs;
    tempB = env->active_tc.LO[ac_t];

    env->active_tc.HI[ac_t] = tempB;
    env->active_tc.LO[ac_t] = tempA;

    pos = get_DSPControl_pos(env);

    if (pos <= 64) {
        pos = pos + 64;
        set_DSPControl_pos(env, pos);
    }
}
#endif

void helper_wrdsp(CPUMIPSState *env, target_ulong rs, uint32_t mask_num)
{
    uint8_t  mask[6];
    uint8_t  i;
    uint32_t newbits, overwrite;
    target_ulong dsp;

    newbits   = 0x00;
    overwrite = 0xFFFFFFFF;
    dsp = env->active_tc.DSPControl;

    for (i = 0; i < 6; i++) {
        mask[i] = (mask_num >> i) & 0x01;
    }

    if (mask[0] == 1) {
#if defined(TARGET_MIPS64)
        overwrite &= 0xFFFFFF80;
        newbits   &= 0xFFFFFF80;
        newbits   |= 0x0000007F & rs;
#else
        overwrite &= 0xFFFFFFC0;
        newbits   &= 0xFFFFFFC0;
        newbits   |= 0x0000003F & rs;
#endif
    }

    if (mask[1] == 1) {
        overwrite &= 0xFFFFE07F;
        newbits   &= 0xFFFFE07F;
        newbits   |= 0x00001F80 & rs;
    }

    if (mask[2] == 1) {
        overwrite &= 0xFFFFDFFF;
        newbits   &= 0xFFFFDFFF;
        newbits   |= 0x00002000 & rs;
    }

    if (mask[3] == 1) {
        overwrite &= 0xFF00FFFF;
        newbits   &= 0xFF00FFFF;
        newbits   |= 0x00FF0000 & rs;
    }

    if (mask[4] == 1) {
        overwrite &= 0x00FFFFFF;
        newbits   &= 0x00FFFFFF;
        newbits   |= 0xFF000000 & rs;
    }

    if (mask[5] == 1) {
        overwrite &= 0xFFFFBFFF;
        newbits   &= 0xFFFFBFFF;
        newbits   |= 0x00004000 & rs;
    }

    dsp = dsp & overwrite;
    dsp = dsp | newbits;
    env->active_tc.DSPControl = dsp;
}

target_ulong helper_rddsp(CPUMIPSState *env, uint32_t masknum)
{
    uint8_t  mask[6];
    uint32_t ruler, i;
    target_ulong temp;
    target_ulong dsp;

    ruler = 0x01;
    for (i = 0; i < 6; i++) {
        mask[i] = (masknum & ruler) >> i ;
        ruler = ruler << 1;
    }

    temp  = 0x00;
    dsp = env->active_tc.DSPControl;

    if (mask[0] == 1) {
#if defined(TARGET_MIPS64)
        temp |= dsp & 0x7F;
#else
        temp |= dsp & 0x3F;
#endif
    }

    if (mask[1] == 1) {
        temp |= dsp & 0x1F80;
    }

    if (mask[2] == 1) {
        temp |= dsp & 0x2000;
    }

    if (mask[3] == 1) {
        temp |= dsp & 0x00FF0000;
    }

    if (mask[4] == 1) {
        temp |= dsp & 0xFF000000;
    }

    if (mask[5] == 1) {
        temp |= dsp & 0x4000;
    }

    return temp;
}

#undef MIPSDSP_LHI
#undef MIPSDSP_LLO
#undef MIPSDSP_HI
#undef MIPSDSP_LO
#undef MIPSDSP_Q3
#undef MIPSDSP_Q2
#undef MIPSDSP_Q1
#undef MIPSDSP_Q0
