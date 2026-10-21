#pragma once

#if defined(_M_X64) || defined(__x86_64__) || defined(_M_IX86) || defined(__i386__)
    #include <immintrin.h>
#endif

namespace strata::dsp {

/**
 * RAII Denormal Guard: Sets Flush-To-Zero (FTZ) and Denormals-Are-Zero (DAZ)
 * on entry, restoring previous state on exit.
 */
class DenormalGuard final {
public:
    DenormalGuard() noexcept {
#if defined(_M_X64) || defined(__x86_64__) || defined(_M_IX86) || defined(__i386__)
        oldMxcsr_ = _mm_getcsr();
        // 0x8000 = FTZ (Flush-to-Zero), 0x0040 = DAZ (Denormals-are-Zero)
        _mm_setcsr(oldMxcsr_ | 0x8040);
#endif
    }

    ~DenormalGuard() noexcept {
#if defined(_M_X64) || defined(__x86_64__) || defined(_M_IX86) || defined(__i386__)
        _mm_setcsr(oldMxcsr_);
#endif
    }

    DenormalGuard(const DenormalGuard&) = delete;
    DenormalGuard& operator=(const DenormalGuard&) = delete;
    DenormalGuard(DenormalGuard&&) = delete;
    DenormalGuard& operator=(DenormalGuard&&) = delete;

private:
#if defined(_M_X64) || defined(__x86_64__) || defined(_M_IX86) || defined(__i386__)
    unsigned int oldMxcsr_{0};
#endif
};

} // namespace strata::dsp
