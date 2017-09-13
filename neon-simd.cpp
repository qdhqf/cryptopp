// crc-simd.cpp - written and placed in the public domain by
//                Jeffrey Walton, Uri Blumenthal and Marcel Raad.
//
//    This source file uses intrinsics to gain access to ARMv7a and
//    ARMv8a NEON instructions. A separate source file is needed
//    because additional CXXFLAGS are required to enable the
//    appropriate instructions sets in some build configurations.

#include "pch.h"
#include "config.h"
#include "stdcpp.h"

#if (CRYPTOPP_ARM_NEON_AVAILABLE)
# include <arm_neon.h>
#endif

#if defined(CRYPTOPP_ARM_ACLE_AVAILABLE)
# include <arm_acle.h>
#endif

#ifdef CRYPTOPP_GNU_STYLE_INLINE_ASSEMBLY
# include <signal.h>
# include <setjmp.h>
#endif

#ifndef EXCEPTION_EXECUTE_HANDLER
# define EXCEPTION_EXECUTE_HANDLER 1
#endif

NAMESPACE_BEGIN(CryptoPP)

#ifdef CRYPTOPP_GNU_STYLE_INLINE_ASSEMBLY
extern "C" {
    typedef void (*SigHandler)(int);

	static jmp_buf s_jmpSIGILL;
	static void SigIllHandler(int)
	{
		longjmp(s_jmpSIGILL, 1);
	}
};
#endif  // Not CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY

bool CPU_ProbeNEON()
{
#if (CRYPTOPP_ARM_NEON_AVAILABLE)
# if defined(CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY)
	volatile bool result = true;
	__try
	{
		uint32_t v1[4] = {1,1,1,1};
		uint32x4_t x1 = vld1q_u32(v1);
		uint64_t v2[2] = {1,1};
		uint64x2_t x2 = vld1q_u64(v2);

		uint32x4_t x3 = vdupq_n_u32(2);
		x3 = vsetq_lane_u32(vgetq_lane_u32(x1,0),x3,0);
		x3 = vsetq_lane_u32(vgetq_lane_u32(x1,3),x3,3);
		uint64x2_t x4 = vdupq_n_u64(2);
		x4 = vsetq_lane_u64(vgetq_lane_u64(x2,0),x4,0);
		x4 = vsetq_lane_u64(vgetq_lane_u64(x2,1),x4,1);

		result = !!(vgetq_lane_u32(x3,0) | vgetq_lane_u64(x4,1));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
	return result;
# else

	// longjmp and clobber warnings. Volatile is required.
	// http://github.com/weidai11/cryptopp/issues/24 and http://stackoverflow.com/q/7721854
	volatile bool result = true;

	volatile SigHandler oldHandler = signal(SIGILL, SigIllHandler);
	if (oldHandler == SIG_ERR)
		return false;

	volatile sigset_t oldMask;
	if (sigprocmask(0, NULLPTR, (sigset_t*)&oldMask))
		return false;

	if (setjmp(s_jmpSIGILL))
		result = false;
	else
	{
		uint32_t v1[4] = {1,1,1,1};
		uint32x4_t x1 = vld1q_u32(v1);
		uint64_t v2[2] = {1,1};
		uint64x2_t x2 = vld1q_u64(v2);

		uint32x4_t x3 = {0,0,0,0};
		x3 = vsetq_lane_u32(vgetq_lane_u32(x1,0),x3,0);
		x3 = vsetq_lane_u32(vgetq_lane_u32(x1,3),x3,3);
		uint64x2_t x4 = {0,0};
		x4 = vsetq_lane_u64(vgetq_lane_u64(x2,0),x4,0);
		x4 = vsetq_lane_u64(vgetq_lane_u64(x2,1),x4,1);

		// Hack... GCC optimizes away the code and returns true
		result = !!(vgetq_lane_u32(x3,0) | vgetq_lane_u64(x4,1));
	}

	sigprocmask(SIG_SETMASK, (sigset_t*)&oldMask, NULLPTR);
	signal(SIGILL, oldHandler);
	return result;
# endif
#else
	return false;
#endif  // CRYPTOPP_ARM_NEON_AVAILABLE
}

NAMESPACE_END
