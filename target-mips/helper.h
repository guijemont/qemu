#include "def-helper.h"

DEF_HELPER_2(raise_exception_err, noreturn, i32, int)
DEF_HELPER_1(raise_exception, noreturn, i32)

#ifdef TARGET_MIPS64
DEF_HELPER_3(ldl, tl, tl, tl, int)
DEF_HELPER_3(ldr, tl, tl, tl, int)
DEF_HELPER_3(sdl, void, tl, tl, int)
DEF_HELPER_3(sdr, void, tl, tl, int)
#endif
DEF_HELPER_3(lwl, tl, tl, tl, int)
DEF_HELPER_3(lwr, tl, tl, tl, int)
DEF_HELPER_3(swl, void, tl, tl, int)
DEF_HELPER_3(swr, void, tl, tl, int)

#ifndef CONFIG_USER_ONLY
DEF_HELPER_2(ll, tl, tl, int)
DEF_HELPER_3(sc, tl, tl, tl, int)
#ifdef TARGET_MIPS64
DEF_HELPER_2(lld, tl, tl, int)
DEF_HELPER_3(scd, tl, tl, tl, int)
#endif
#endif

DEF_HELPER_FLAGS_1(clo, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(clz, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
#ifdef TARGET_MIPS64
DEF_HELPER_FLAGS_1(dclo, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(dclz, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_2(dmult, void, tl, tl)
DEF_HELPER_2(dmultu, void, tl, tl)
#endif

DEF_HELPER_2(muls, tl, tl, tl)
DEF_HELPER_2(mulsu, tl, tl, tl)
DEF_HELPER_2(macc, tl, tl, tl)
DEF_HELPER_2(maccu, tl, tl, tl)
DEF_HELPER_2(msac, tl, tl, tl)
DEF_HELPER_2(msacu, tl, tl, tl)
DEF_HELPER_2(mulhi, tl, tl, tl)
DEF_HELPER_2(mulhiu, tl, tl, tl)
DEF_HELPER_2(mulshi, tl, tl, tl)
DEF_HELPER_2(mulshiu, tl, tl, tl)
DEF_HELPER_2(macchi, tl, tl, tl)
DEF_HELPER_2(macchiu, tl, tl, tl)
DEF_HELPER_2(msachi, tl, tl, tl)
DEF_HELPER_2(msachiu, tl, tl, tl)

#ifndef CONFIG_USER_ONLY
/* CP0 helpers */
DEF_HELPER_0(mfc0_mvpcontrol, tl)
DEF_HELPER_0(mfc0_mvpconf0, tl)
DEF_HELPER_0(mfc0_mvpconf1, tl)
DEF_HELPER_0(mftc0_vpecontrol, tl)
DEF_HELPER_0(mftc0_vpeconf0, tl)
DEF_HELPER_0(mfc0_random, tl)
DEF_HELPER_0(mfc0_tcstatus, tl)
DEF_HELPER_0(mftc0_tcstatus, tl)
DEF_HELPER_0(mfc0_tcbind, tl)
DEF_HELPER_0(mftc0_tcbind, tl)
DEF_HELPER_0(mfc0_tcrestart, tl)
DEF_HELPER_0(mftc0_tcrestart, tl)
DEF_HELPER_0(mfc0_tchalt, tl)
DEF_HELPER_0(mftc0_tchalt, tl)
DEF_HELPER_0(mfc0_tccontext, tl)
DEF_HELPER_0(mftc0_tccontext, tl)
DEF_HELPER_0(mfc0_tcschedule, tl)
DEF_HELPER_0(mftc0_tcschedule, tl)
DEF_HELPER_0(mfc0_tcschefback, tl)
DEF_HELPER_0(mftc0_tcschefback, tl)
DEF_HELPER_0(mfc0_count, tl)
DEF_HELPER_0(mftc0_entryhi, tl)
DEF_HELPER_0(mftc0_status, tl)
DEF_HELPER_0(mftc0_cause, tl)
DEF_HELPER_0(mftc0_epc, tl)
DEF_HELPER_0(mftc0_ebase, tl)
DEF_HELPER_1(mftc0_configx, tl, tl)
DEF_HELPER_0(mfc0_lladdr, tl)
DEF_HELPER_1(mfc0_watchlo, tl, i32)
DEF_HELPER_1(mfc0_watchhi, tl, i32)
DEF_HELPER_0(mfc0_debug, tl)
DEF_HELPER_0(mftc0_debug, tl)
#ifdef TARGET_MIPS64
DEF_HELPER_0(dmfc0_tcrestart, tl)
DEF_HELPER_0(dmfc0_tchalt, tl)
DEF_HELPER_0(dmfc0_tccontext, tl)
DEF_HELPER_0(dmfc0_tcschedule, tl)
DEF_HELPER_0(dmfc0_tcschefback, tl)
DEF_HELPER_0(dmfc0_lladdr, tl)
DEF_HELPER_1(dmfc0_watchlo, tl, i32)
#endif /* TARGET_MIPS64 */

DEF_HELPER_1(mtc0_index, void, tl)
DEF_HELPER_1(mtc0_mvpcontrol, void, tl)
DEF_HELPER_1(mtc0_vpecontrol, void, tl)
DEF_HELPER_1(mttc0_vpecontrol, void, tl)
DEF_HELPER_1(mtc0_vpeconf0, void, tl)
DEF_HELPER_1(mttc0_vpeconf0, void, tl)
DEF_HELPER_1(mtc0_vpeconf1, void, tl)
DEF_HELPER_1(mtc0_yqmask, void, tl)
DEF_HELPER_1(mtc0_vpeopt, void, tl)
DEF_HELPER_1(mtc0_entrylo0, void, tl)
DEF_HELPER_1(mtc0_tcstatus, void, tl)
DEF_HELPER_1(mttc0_tcstatus, void, tl)
DEF_HELPER_1(mtc0_tcbind, void, tl)
DEF_HELPER_1(mttc0_tcbind, void, tl)
DEF_HELPER_1(mtc0_tcrestart, void, tl)
DEF_HELPER_1(mttc0_tcrestart, void, tl)
DEF_HELPER_1(mtc0_tchalt, void, tl)
DEF_HELPER_1(mttc0_tchalt, void, tl)
DEF_HELPER_1(mtc0_tccontext, void, tl)
DEF_HELPER_1(mttc0_tccontext, void, tl)
DEF_HELPER_1(mtc0_tcschedule, void, tl)
DEF_HELPER_1(mttc0_tcschedule, void, tl)
DEF_HELPER_1(mtc0_tcschefback, void, tl)
DEF_HELPER_1(mttc0_tcschefback, void, tl)
DEF_HELPER_1(mtc0_entrylo1, void, tl)
DEF_HELPER_1(mtc0_context, void, tl)
DEF_HELPER_1(mtc0_pagemask, void, tl)
DEF_HELPER_1(mtc0_pagegrain, void, tl)
DEF_HELPER_1(mtc0_wired, void, tl)
DEF_HELPER_1(mtc0_srsconf0, void, tl)
DEF_HELPER_1(mtc0_srsconf1, void, tl)
DEF_HELPER_1(mtc0_srsconf2, void, tl)
DEF_HELPER_1(mtc0_srsconf3, void, tl)
DEF_HELPER_1(mtc0_srsconf4, void, tl)
DEF_HELPER_1(mtc0_hwrena, void, tl)
DEF_HELPER_1(mtc0_count, void, tl)
DEF_HELPER_1(mtc0_entryhi, void, tl)
DEF_HELPER_1(mttc0_entryhi, void, tl)
DEF_HELPER_1(mtc0_compare, void, tl)
DEF_HELPER_1(mtc0_status, void, tl)
DEF_HELPER_1(mttc0_status, void, tl)
DEF_HELPER_1(mtc0_intctl, void, tl)
DEF_HELPER_1(mtc0_srsctl, void, tl)
DEF_HELPER_1(mtc0_cause, void, tl)
DEF_HELPER_1(mttc0_cause, void, tl)
DEF_HELPER_1(mtc0_ebase, void, tl)
DEF_HELPER_1(mttc0_ebase, void, tl)
DEF_HELPER_1(mtc0_config0, void, tl)
DEF_HELPER_1(mtc0_config2, void, tl)
DEF_HELPER_1(mtc0_lladdr, void, tl)
DEF_HELPER_2(mtc0_watchlo, void, tl, i32)
DEF_HELPER_2(mtc0_watchhi, void, tl, i32)
DEF_HELPER_1(mtc0_xcontext, void, tl)
DEF_HELPER_1(mtc0_framemask, void, tl)
DEF_HELPER_1(mtc0_debug, void, tl)
DEF_HELPER_1(mttc0_debug, void, tl)
DEF_HELPER_1(mtc0_performance0, void, tl)
DEF_HELPER_1(mtc0_taglo, void, tl)
DEF_HELPER_1(mtc0_datalo, void, tl)
DEF_HELPER_1(mtc0_taghi, void, tl)
DEF_HELPER_1(mtc0_datahi, void, tl)

/* MIPS MT functions */
DEF_HELPER_1(mftgpr, tl, i32);
DEF_HELPER_1(mftlo, tl, i32)
DEF_HELPER_1(mfthi, tl, i32)
DEF_HELPER_1(mftacx, tl, i32)
DEF_HELPER_0(mftdsp, tl)
DEF_HELPER_2(mttgpr, void, tl, i32)
DEF_HELPER_2(mttlo, void, tl, i32)
DEF_HELPER_2(mtthi, void, tl, i32)
DEF_HELPER_2(mttacx, void, tl, i32)
DEF_HELPER_1(mttdsp, void, tl)
DEF_HELPER_0(dmt, tl)
DEF_HELPER_0(emt, tl)
DEF_HELPER_0(dvpe, tl)
DEF_HELPER_0(evpe, tl)
#endif /* !CONFIG_USER_ONLY */

/* microMIPS functions */
DEF_HELPER_3(lwm, void, tl, tl, i32);
DEF_HELPER_3(swm, void, tl, tl, i32);
#ifdef TARGET_MIPS64
DEF_HELPER_3(ldm, void, tl, tl, i32);
DEF_HELPER_3(sdm, void, tl, tl, i32);
#endif

DEF_HELPER_2(fork, void, tl, tl)
DEF_HELPER_1(yield, tl, tl)

/* CP1 functions */
DEF_HELPER_1(cfc1, tl, i32)
DEF_HELPER_2(ctc1, void, tl, i32)

DEF_HELPER_1(float_cvtd_s, i64, i32)
DEF_HELPER_1(float_cvtd_w, i64, i32)
DEF_HELPER_1(float_cvtd_l, i64, i64)
DEF_HELPER_1(float_cvtl_d, i64, i64)
DEF_HELPER_1(float_cvtl_s, i64, i32)
DEF_HELPER_1(float_cvtps_pw, i64, i64)
DEF_HELPER_1(float_cvtpw_ps, i64, i64)
DEF_HELPER_1(float_cvts_d, i32, i64)
DEF_HELPER_1(float_cvts_w, i32, i32)
DEF_HELPER_1(float_cvts_l, i32, i64)
DEF_HELPER_1(float_cvts_pl, i32, i32)
DEF_HELPER_1(float_cvts_pu, i32, i32)
DEF_HELPER_1(float_cvtw_s, i32, i32)
DEF_HELPER_1(float_cvtw_d, i32, i64)

DEF_HELPER_2(float_addr_ps, i64, i64, i64)
DEF_HELPER_2(float_mulr_ps, i64, i64, i64)

#define FOP_PROTO(op)                       \
DEF_HELPER_1(float_ ## op ## l_s, i64, i32) \
DEF_HELPER_1(float_ ## op ## l_d, i64, i64) \
DEF_HELPER_1(float_ ## op ## w_s, i32, i32) \
DEF_HELPER_1(float_ ## op ## w_d, i32, i64)
FOP_PROTO(round)
FOP_PROTO(trunc)
FOP_PROTO(ceil)
FOP_PROTO(floor)
#undef FOP_PROTO

#define FOP_PROTO(op)                       \
DEF_HELPER_1(float_ ## op ## _s, i32, i32)  \
DEF_HELPER_1(float_ ## op ## _d, i64, i64)
FOP_PROTO(sqrt)
FOP_PROTO(rsqrt)
FOP_PROTO(recip)
#undef FOP_PROTO

#define FOP_PROTO(op)                       \
DEF_HELPER_1(float_ ## op ## _s, i32, i32)  \
DEF_HELPER_1(float_ ## op ## _d, i64, i64)  \
DEF_HELPER_1(float_ ## op ## _ps, i64, i64)
FOP_PROTO(abs)
FOP_PROTO(chs)
FOP_PROTO(recip1)
FOP_PROTO(rsqrt1)
#undef FOP_PROTO

#define FOP_PROTO(op)                             \
DEF_HELPER_2(float_ ## op ## _s, i32, i32, i32)   \
DEF_HELPER_2(float_ ## op ## _d, i64, i64, i64)   \
DEF_HELPER_2(float_ ## op ## _ps, i64, i64, i64)
FOP_PROTO(add)
FOP_PROTO(sub)
FOP_PROTO(mul)
FOP_PROTO(div)
FOP_PROTO(recip2)
FOP_PROTO(rsqrt2)
#undef FOP_PROTO

#define FOP_PROTO(op)                                 \
DEF_HELPER_3(float_ ## op ## _s, i32, i32, i32, i32)  \
DEF_HELPER_3(float_ ## op ## _d, i64, i64, i64, i64)  \
DEF_HELPER_3(float_ ## op ## _ps, i64, i64, i64, i64)
FOP_PROTO(muladd)
FOP_PROTO(mulsub)
FOP_PROTO(nmuladd)
FOP_PROTO(nmulsub)
#undef FOP_PROTO

#define FOP_PROTO(op)                               \
DEF_HELPER_3(cmp_d_ ## op, void, i64, i64, int)     \
DEF_HELPER_3(cmpabs_d_ ## op, void, i64, i64, int)  \
DEF_HELPER_3(cmp_s_ ## op, void, i32, i32, int)     \
DEF_HELPER_3(cmpabs_s_ ## op, void, i32, i32, int)  \
DEF_HELPER_3(cmp_ps_ ## op, void, i64, i64, int)    \
DEF_HELPER_3(cmpabs_ps_ ## op, void, i64, i64, int)
FOP_PROTO(f)
FOP_PROTO(un)
FOP_PROTO(eq)
FOP_PROTO(ueq)
FOP_PROTO(olt)
FOP_PROTO(ult)
FOP_PROTO(ole)
FOP_PROTO(ule)
FOP_PROTO(sf)
FOP_PROTO(ngle)
FOP_PROTO(seq)
FOP_PROTO(ngl)
FOP_PROTO(lt)
FOP_PROTO(nge)
FOP_PROTO(le)
FOP_PROTO(ngt)
#undef FOP_PROTO

/* Special functions */
#ifndef CONFIG_USER_ONLY
DEF_HELPER_0(tlbwi, void)
DEF_HELPER_0(tlbwr, void)
DEF_HELPER_0(tlbp, void)
DEF_HELPER_0(tlbr, void)
DEF_HELPER_0(di, tl)
DEF_HELPER_0(ei, tl)
DEF_HELPER_0(eret, void)
DEF_HELPER_0(deret, void)
#endif /* !CONFIG_USER_ONLY */
DEF_HELPER_0(rdhwr_cpunum, tl)
DEF_HELPER_0(rdhwr_synci_step, tl)
DEF_HELPER_0(rdhwr_cc, tl)
DEF_HELPER_0(rdhwr_ccres, tl)
DEF_HELPER_1(pmon, void, int)
DEF_HELPER_0(wait, void)

/*** MIPS DSP ***/
/* DSP Arithmetic Sub-class insns */
DEF_HELPER_FLAGS_3(addq_ph, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(addq_s_ph, 0, tl, env, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(addq_qh, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(addq_s_qh, 0, tl, env, tl, tl)
#endif
DEF_HELPER_FLAGS_3(addq_s_w, 0, tl, env, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(addq_pw, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(addq_s_pw, 0, tl, env, tl, tl)
#endif
DEF_HELPER_FLAGS_3(addu_qb, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(addu_s_qb, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_2(adduh_qb, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(adduh_r_qb, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_3(addu_ph, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(addu_s_ph, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_2(addqh_ph, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(addqh_r_ph, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(addqh_w, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(addqh_r_w, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(addu_ob, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(addu_s_ob, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_2(adduh_ob, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(adduh_r_ob, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_3(addu_qh, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(addu_s_qh, 0, tl, env, tl, tl)
#endif
DEF_HELPER_FLAGS_3(subq_ph, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(subq_s_ph, 0, tl, env, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(subq_qh, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(subq_s_qh, 0, tl, env, tl, tl)
#endif
DEF_HELPER_FLAGS_3(subq_s_w, 0, tl, env, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(subq_pw, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(subq_s_pw, 0, tl, env, tl, tl)
#endif
DEF_HELPER_FLAGS_3(subu_qb, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(subu_s_qb, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_2(subuh_qb, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(subuh_r_qb, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_3(subu_ph, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(subu_s_ph, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_2(subqh_ph, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(subqh_r_ph, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(subqh_w, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(subqh_r_w, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(subu_ob, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(subu_s_ob, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_2(subuh_ob, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(subuh_r_ob, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_3(subu_qh, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(subu_s_qh, 0, tl, env, tl, tl)
#endif
DEF_HELPER_FLAGS_3(addsc, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(addwc, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_2(modsub, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_1(raddu_w_qb, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_1(raddu_l_ob, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
#endif
DEF_HELPER_FLAGS_2(absq_s_qb, 0, tl, env, tl)
DEF_HELPER_FLAGS_2(absq_s_ph, 0, tl, env, tl)
DEF_HELPER_FLAGS_2(absq_s_w, 0, tl, env, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_2(absq_s_ob, 0, tl, env, tl)
DEF_HELPER_FLAGS_2(absq_s_qh, 0, tl, env, tl)
DEF_HELPER_FLAGS_2(absq_s_pw, 0, tl, env, tl)
#endif
DEF_HELPER_FLAGS_2(precr_qb_ph, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(precrq_qb_ph, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_3(precr_sra_ph_w, TCG_CALL_CONST | TCG_CALL_PURE,
                   tl, i32, tl, tl)
DEF_HELPER_FLAGS_3(precr_sra_r_ph_w, TCG_CALL_CONST | TCG_CALL_PURE,
                   tl, i32, tl, tl)
DEF_HELPER_FLAGS_2(precrq_ph_w, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_3(precrq_rs_ph_w, 0, tl, env, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_2(precr_ob_qh, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_3(precr_sra_qh_pw,
                   TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl, i32)
DEF_HELPER_FLAGS_3(precr_sra_r_qh_pw,
                   TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl, i32)
DEF_HELPER_FLAGS_2(precrq_ob_qh, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(precrq_qh_pw, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_3(precrq_rs_qh_pw,
                   TCG_CALL_CONST | TCG_CALL_PURE, tl, env, tl, tl)
DEF_HELPER_FLAGS_2(precrq_pw_l, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
#endif
DEF_HELPER_FLAGS_3(precrqu_s_qb_ph, 0, tl, env, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(precrqu_s_ob_qh,
                   TCG_CALL_CONST | TCG_CALL_PURE, tl, env, tl, tl)

DEF_HELPER_FLAGS_1(preceq_pw_qhl, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(preceq_pw_qhr, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(preceq_pw_qhla, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(preceq_pw_qhra, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
#endif
DEF_HELPER_FLAGS_1(precequ_ph_qbl, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(precequ_ph_qbr, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(precequ_ph_qbla, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(precequ_ph_qbra, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_1(precequ_qh_obl, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(precequ_qh_obr, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(precequ_qh_obla, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(precequ_qh_obra, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
#endif
DEF_HELPER_FLAGS_1(preceu_ph_qbl, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(preceu_ph_qbr, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(preceu_ph_qbla, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(preceu_ph_qbra, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_1(preceu_qh_obl, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(preceu_qh_obr, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(preceu_qh_obla, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_1(preceu_qh_obra, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
#endif

/* DSP GPR-Based Shift Sub-class insns */
DEF_HELPER_FLAGS_3(shll_qb, 0, tl, env, i32, tl)
DEF_HELPER_FLAGS_3(shllv_qb, 0, tl, env, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(shll_ob, 0, tl, env, tl, i32)
DEF_HELPER_FLAGS_3(shllv_ob, 0, tl, env, tl, tl)
#endif
DEF_HELPER_FLAGS_3(shll_ph, 0, tl, env, i32, tl)
DEF_HELPER_FLAGS_3(shllv_ph, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(shll_s_ph, 0, tl, env, i32, tl)
DEF_HELPER_FLAGS_3(shllv_s_ph, 0, tl, env, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(shll_qh, 0, tl, env, tl, i32)
DEF_HELPER_FLAGS_3(shllv_qh, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(shll_s_qh, 0, tl, env, tl, i32)
DEF_HELPER_FLAGS_3(shllv_s_qh, 0, tl, env, tl, tl)
#endif
DEF_HELPER_FLAGS_3(shll_s_w, 0, tl, env, i32, tl)
DEF_HELPER_FLAGS_3(shllv_s_w, 0, tl, env, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(shll_pw, 0, tl, env, tl, i32)
DEF_HELPER_FLAGS_3(shllv_pw, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(shll_s_pw, 0, tl, env, tl, i32)
DEF_HELPER_FLAGS_3(shllv_s_pw, 0, tl, env, tl, tl)
#endif
DEF_HELPER_FLAGS_2(shrl_qb, TCG_CALL_CONST | TCG_CALL_PURE, tl, i32, tl)
DEF_HELPER_FLAGS_2(shrlv_qb, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(shrl_ph, TCG_CALL_CONST | TCG_CALL_PURE, tl, i32, tl)
DEF_HELPER_FLAGS_2(shrlv_ph, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_2(shrl_ob, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, i32)
DEF_HELPER_FLAGS_2(shrlv_ob, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(shrl_qh, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, i32)
DEF_HELPER_FLAGS_2(shrlv_qh, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
#endif
DEF_HELPER_FLAGS_2(shra_qb, TCG_CALL_CONST | TCG_CALL_PURE, tl, i32, tl)
DEF_HELPER_FLAGS_2(shra_r_qb, TCG_CALL_CONST | TCG_CALL_PURE, tl, i32, tl)
DEF_HELPER_FLAGS_2(shrav_qb, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(shrav_r_qb, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_2(shra_ob, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, i32)
DEF_HELPER_FLAGS_2(shrav_ob, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(shra_r_ob, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, i32)
DEF_HELPER_FLAGS_2(shrav_r_ob, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
#endif
DEF_HELPER_FLAGS_2(shra_ph, TCG_CALL_CONST | TCG_CALL_PURE, tl, i32, tl)
DEF_HELPER_FLAGS_2(shrav_ph, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(shra_r_ph, TCG_CALL_CONST | TCG_CALL_PURE, tl, i32, tl)
DEF_HELPER_FLAGS_2(shrav_r_ph, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(shra_r_w, TCG_CALL_CONST | TCG_CALL_PURE, tl, i32, tl)
DEF_HELPER_FLAGS_2(shrav_r_w, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_2(shra_qh, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, i32)
DEF_HELPER_FLAGS_2(shrav_qh, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(shra_r_qh, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, i32)
DEF_HELPER_FLAGS_2(shrav_r_qh, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(shra_pw, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, i32)
DEF_HELPER_FLAGS_2(shrav_pw, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(shra_r_pw, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, i32)
DEF_HELPER_FLAGS_2(shrav_r_pw, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
#endif

/* DSP Multiply Sub-class insns */
DEF_HELPER_FLAGS_3(muleu_s_ph_qbl, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(muleu_s_ph_qbr, 0, tl, env, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(muleu_s_qh_obl, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(muleu_s_qh_obr, 0, tl, env, tl, tl)
#endif
DEF_HELPER_FLAGS_3(mulq_rs_ph, 0, tl, env, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(mulq_rs_qh, 0, tl, env, tl, tl)
#endif
DEF_HELPER_FLAGS_3(muleq_s_w_phl, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(muleq_s_w_phr, 0, tl, env, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(muleq_s_pw_qhl, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(muleq_s_pw_qhr, 0, tl, env, tl, tl)
#endif
DEF_HELPER_FLAGS_4(dpau_h_qbl, 0, void, env, i32, tl, tl)
DEF_HELPER_FLAGS_4(dpau_h_qbr, 0, void, env, i32, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_4(dpau_h_obl, 0, void, env, tl, tl, i32)
DEF_HELPER_FLAGS_4(dpau_h_obr, 0, void, env, tl, tl, i32)
#endif
DEF_HELPER_FLAGS_4(dpsu_h_qbl, 0, void, env, i32, tl, tl)
DEF_HELPER_FLAGS_4(dpsu_h_qbr, 0, void, env, i32, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_4(dpsu_h_obl, 0, void, env, tl, tl, i32)
DEF_HELPER_FLAGS_4(dpsu_h_obr, 0, void, env, tl, tl, i32)
#endif
DEF_HELPER_FLAGS_4(dpa_w_ph, 0, void, env, i32, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_4(dpa_w_qh, 0, void, env, tl, tl, i32)
#endif
DEF_HELPER_FLAGS_4(dpax_w_ph, 0, void, env, i32, tl, tl)
DEF_HELPER_FLAGS_4(dpaq_s_w_ph, 0, void, env, i32, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_4(dpaq_s_w_qh, 0, void, env, tl, tl, i32)
#endif
DEF_HELPER_FLAGS_4(dpaqx_s_w_ph, 0, void, env, i32, tl, tl)
DEF_HELPER_FLAGS_4(dpaqx_sa_w_ph, 0, void, env, i32, tl, tl)
DEF_HELPER_FLAGS_4(dps_w_ph, 0, void, env, i32, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_4(dps_w_qh, 0, void, env, tl, tl, i32)
#endif
DEF_HELPER_FLAGS_4(dpsx_w_ph, 0, void, env, i32, tl, tl)
DEF_HELPER_FLAGS_4(dpsq_s_w_ph, 0, void, env, i32, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_4(dpsq_s_w_qh, 0, void, env, tl, tl, i32)
#endif
DEF_HELPER_FLAGS_4(dpsqx_s_w_ph, 0, void, env, i32, tl, tl)
DEF_HELPER_FLAGS_4(dpsqx_sa_w_ph, 0, void, env, i32, tl, tl)
DEF_HELPER_FLAGS_4(mulsaq_s_w_ph, 0, void, env, i32, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_4(mulsaq_s_w_qh, 0, void, env, tl, tl, i32)
#endif
DEF_HELPER_FLAGS_4(dpaq_sa_l_w, 0, void, env, i32, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_4(dpaq_sa_l_pw, 0, void, env, tl, tl, i32)
#endif
DEF_HELPER_FLAGS_4(dpsq_sa_l_w, 0, void, env, i32, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_4(dpsq_sa_l_pw, 0, void, env, tl, tl, i32)
DEF_HELPER_FLAGS_4(mulsaq_s_l_pw, 0, void, env, tl, tl, i32)
#endif
DEF_HELPER_FLAGS_4(maq_s_w_phl, 0, void, env, i32, tl, tl)
DEF_HELPER_FLAGS_4(maq_s_w_phr, 0, void, env, i32, tl, tl)
DEF_HELPER_FLAGS_4(maq_sa_w_phl, 0, void, env, i32, tl, tl)
DEF_HELPER_FLAGS_4(maq_sa_w_phr, 0, void, env, i32, tl, tl)
DEF_HELPER_FLAGS_3(mul_ph, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(mul_s_ph, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(mulq_s_ph, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(mulq_s_w, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(mulq_rs_w, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_4(mulsa_w_ph, 0, void, env, i32, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_4(maq_s_w_qhll, 0, void, env, tl, tl, i32)
DEF_HELPER_FLAGS_4(maq_s_w_qhlr, 0, void, env, tl, tl, i32)
DEF_HELPER_FLAGS_4(maq_s_w_qhrl, 0, void, env, tl, tl, i32)
DEF_HELPER_FLAGS_4(maq_s_w_qhrr, 0, void, env, tl, tl, i32)
DEF_HELPER_FLAGS_4(maq_sa_w_qhll, 0, void, env, tl, tl, i32)
DEF_HELPER_FLAGS_4(maq_sa_w_qhlr, 0, void, env, tl, tl, i32)
DEF_HELPER_FLAGS_4(maq_sa_w_qhrl, 0, void, env, tl, tl, i32)
DEF_HELPER_FLAGS_4(maq_sa_w_qhrr, 0, void, env, tl, tl, i32)
DEF_HELPER_FLAGS_4(maq_s_l_pwl, 0, void, env, tl, tl, i32)
DEF_HELPER_FLAGS_4(maq_s_l_pwr, 0, void, env, tl, tl, i32)
DEF_HELPER_FLAGS_4(dmadd, 0, void, env, tl, tl, i32)
DEF_HELPER_FLAGS_4(dmaddu, 0, void, env, tl, tl, i32)
DEF_HELPER_FLAGS_4(dmsub, 0, void, env, tl, tl, i32)
DEF_HELPER_FLAGS_4(dmsubu, 0, void, env, tl, tl, i32)
#endif

/* DSP Bit/Manipulation Sub-class insns */
DEF_HELPER_FLAGS_1(bitrev, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl)
DEF_HELPER_FLAGS_3(insv, 0, tl, env, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(dinsv, 0, tl, env, tl, tl);
#endif

/* DSP Compare-Pick Sub-class insns */
DEF_HELPER_FLAGS_3(cmpu_eq_qb, 0, void, env, tl, tl)
DEF_HELPER_FLAGS_3(cmpu_lt_qb, 0, void, env, tl, tl)
DEF_HELPER_FLAGS_3(cmpu_le_qb, 0, void, env, tl, tl)
DEF_HELPER_FLAGS_2(cmpgu_eq_qb, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(cmpgu_lt_qb, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(cmpgu_le_qb, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_3(cmp_eq_ph, 0, void, env, tl, tl)
DEF_HELPER_FLAGS_3(cmp_lt_ph, 0, void, env, tl, tl)
DEF_HELPER_FLAGS_3(cmp_le_ph, 0, void, env, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(cmpu_eq_ob, 0, void, env, tl, tl)
DEF_HELPER_FLAGS_3(cmpu_lt_ob, 0, void, env, tl, tl)
DEF_HELPER_FLAGS_3(cmpu_le_ob, 0, void, env, tl, tl)
DEF_HELPER_FLAGS_3(cmpgdu_eq_ob, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(cmpgdu_lt_ob, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(cmpgdu_le_ob, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_2(cmpgu_eq_ob, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(cmpgu_lt_ob, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_2(cmpgu_le_ob, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
DEF_HELPER_FLAGS_3(cmp_eq_qh, 0, void, env, tl, tl)
DEF_HELPER_FLAGS_3(cmp_lt_qh, 0, void, env, tl, tl)
DEF_HELPER_FLAGS_3(cmp_le_qh, 0, void, env, tl, tl)
DEF_HELPER_FLAGS_3(cmp_eq_pw, 0, void, env, tl, tl)
DEF_HELPER_FLAGS_3(cmp_lt_pw, 0, void, env, tl, tl)
DEF_HELPER_FLAGS_3(cmp_le_pw, 0, void, env, tl, tl)
#endif
DEF_HELPER_FLAGS_3(pick_qb, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(pick_ph, 0, tl, env, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(pick_ob, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(pick_qh, 0, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(pick_pw, 0, tl, env, tl, tl)
#endif
DEF_HELPER_FLAGS_3(append, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl, i32)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(dappend, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl, i32)
#endif
DEF_HELPER_FLAGS_3(prepend, TCG_CALL_CONST | TCG_CALL_PURE, tl, i32, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(prependd, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl, i32)
DEF_HELPER_FLAGS_3(prependw, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl, i32)
#endif
DEF_HELPER_FLAGS_3(balign, TCG_CALL_CONST | TCG_CALL_PURE, tl, i32, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(dbalign, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl, i32)
#endif
DEF_HELPER_FLAGS_2(packrl_ph, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_2(packrl_pw, TCG_CALL_CONST | TCG_CALL_PURE, tl, tl, tl)
#endif

/* DSP Accumulator and DSPControl Access Sub-class insns */
DEF_HELPER_FLAGS_3(extr_w, 0, tl, env, i32, i32)
DEF_HELPER_FLAGS_3(extr_r_w, 0, tl, env, i32, i32)
DEF_HELPER_FLAGS_3(extr_rs_w, 0, tl, env, i32, i32)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(dextr_w, 0, tl, env, i32, i32)
DEF_HELPER_FLAGS_3(dextr_r_w, 0, tl, env, i32, i32)
DEF_HELPER_FLAGS_3(dextr_rs_w, 0, tl, env, i32, i32)
DEF_HELPER_FLAGS_3(dextr_l, 0, tl, env, i32, i32)
DEF_HELPER_FLAGS_3(dextr_r_l, 0, tl, env, i32, i32)
DEF_HELPER_FLAGS_3(dextr_rs_l, 0, tl, env, i32, i32)
#endif
DEF_HELPER_FLAGS_3(extr_s_h, 0, tl, env, i32, i32)
DEF_HELPER_FLAGS_3(extrv_s_h, 0, tl, env, i32, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(dextr_s_h, 0, tl, env, i32, i32)
DEF_HELPER_FLAGS_3(dextrv_s_h, 0, tl, env, i32, tl)
#endif
DEF_HELPER_FLAGS_3(extrv_w, 0, tl, env, i32, tl)
DEF_HELPER_FLAGS_3(extrv_r_w, 0, tl, env, i32, tl)
DEF_HELPER_FLAGS_3(extrv_rs_w, 0, tl, env, i32, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(dextrv_w, 0, tl, env, i32, tl)
DEF_HELPER_FLAGS_3(dextrv_r_w, 0, tl, env, i32, tl)
DEF_HELPER_FLAGS_3(dextrv_rs_w, 0, tl, env, i32, tl)
DEF_HELPER_FLAGS_3(dextrv_l, 0, tl, env, i32, tl)
DEF_HELPER_FLAGS_3(dextrv_r_l, 0, tl, env, i32, tl)
DEF_HELPER_FLAGS_3(dextrv_rs_l, 0, tl, env, i32, tl)
#endif
DEF_HELPER_FLAGS_3(extp, 0, tl, env, i32, i32)
DEF_HELPER_FLAGS_3(extpv, 0, tl, env, i32, tl)
DEF_HELPER_FLAGS_3(extpdp, 0, tl, env, i32, i32)
DEF_HELPER_FLAGS_3(extpdpv, 0, tl, env, i32, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(dextp, 0, tl, env, i32, i32)
DEF_HELPER_FLAGS_3(dextpv, 0, tl, env, i32, tl)
DEF_HELPER_FLAGS_3(dextpdp, 0, tl, env, i32, i32)
DEF_HELPER_FLAGS_3(dextpdpv, 0, tl, env, i32, tl)
#endif
DEF_HELPER_FLAGS_3(shilo, 0, void, env, i32, i32)
DEF_HELPER_FLAGS_3(shilov, 0, void, env, i32, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(dshilo, 0, void, env, i32, i32)
DEF_HELPER_FLAGS_3(dshilov, 0, void, env, tl, i32)
#endif
DEF_HELPER_FLAGS_3(mthlip, 0, void, env, i32, tl)
#if defined(TARGET_MIPS64)
DEF_HELPER_FLAGS_3(dmthlip, 0, void, env, tl, i32)
#endif
DEF_HELPER_FLAGS_3(wrdsp, 0, void, env, tl, i32)
DEF_HELPER_FLAGS_2(rddsp, 0, tl, env, i32)

#include "def-helper.h"
