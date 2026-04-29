// ppc_config.h — CafeRecomp configuration for Wii U (Espresso / PPC750)
//
// These values are defined at build time by CMake (see CafeUtils/CMakeLists.txt)
// and can be overridden per-project.
//
// PPC_IMAGE_BASE   : Virtual address of image base (where data[] starts)
// PPC_IMAGE_SIZE   : Size of the guest memory region
// PPC_CODE_BASE    : Start address of the .text section
//
// The defaults below match a typical Wii U game layout:
//   Code starts at 0x02000000, data heap at 0x10000000.

#ifndef PPC_CONFIG_H_INCLUDED
#define PPC_CONFIG_H_INCLUDED

// ── Wii U memory layout ───────────────────────────────────────────────────────
// Application code/data lives in the first 1GB (0x00000000–0x3FFFFFFF).
// The Espresso uses 32-bit addresses — no 64-bit ops needed.
#ifndef PPC_IMAGE_BASE
#define PPC_IMAGE_BASE 0
#endif

#ifndef PPC_IMAGE_SIZE
#define PPC_IMAGE_SIZE 0x40000000ULL   // 1 GB addressable
#endif

#ifndef PPC_CODE_BASE
#define PPC_CODE_BASE 0x02000000       // Typical Wii U game code start
#endif

// ── Espresso-specific tuning ──────────────────────────────────────────────────
// The Espresso (PPC750-derived) is a single-threaded, in-order core per
// thread. No SMT, no 64-bit operations in user mode.
// These macros control which registers are optimized as C++ locals vs
// full PPCContext struct fields.

#define PPC_CONFIG_NON_VOLATILE_AS_LOCAL
#define PPC_CONFIG_NON_ARGUMENT_AS_LOCAL
#define PPC_CONFIG_CR_AS_LOCAL
#define PPC_CONFIG_CTR_AS_LOCAL
#define PPC_CONFIG_XER_AS_LOCAL
#define PPC_CONFIG_RESERVED_AS_LOCAL
#define PPC_CONFIG_SKIP_LR
#define PPC_CONFIG_SKIP_MSR

#endif // PPC_CONFIG_H_INCLUDED
