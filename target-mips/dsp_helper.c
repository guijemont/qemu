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
