#ifndef __COMMON_INCLUDE_COMPILER_H__
#define __COMMON_INCLUDE_COMPILER_H__


#ifndef packed_struct
#define packed_struct       __attribute__((packed))
#endif

#ifndef packed2_struct
#define packed2_struct      __attribute__((packed, aligned(2)))
#endif

#ifndef packed4_struct
#define packed4_struct      __attribute__((packed, aligned(4)))
#endif

#ifndef packed8_struct
#define packed8_struct      __attribute__((packed, aligned(8)))
#endif

#ifndef aligned_struct
#define aligned_struct(a)   __attribute__((packed, aligned(a)))
#endif

#ifndef no_inline
#define no_inline           __attribute__((noinline))
#endif

#ifndef no_opt
#ifdef __clang__
#define no_opt              __attribute__((optnone))
#else
#define no_opt              __attribute__((optimize("-O0")))
#endif
#endif

#ifndef weak_func
#define weak_func           __attribute__((weak))
#endif

#ifndef weak_alias
#define weak_alias(f)       __attribute__((weak, alias(#f)))
#endif

#ifndef entry_func
#define entry_func          __attribute__((section("entry")))
#endif

#ifndef aligned_var
#define aligned_var(a)      __attribute__((aligned(a)))
#endif


#endif

