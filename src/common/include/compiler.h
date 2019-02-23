#ifndef __COMMON_INCLUDE_COMPILER_H__
#define __COMMON_INCLUDE_COMPILER_H__


#ifndef packed_struct
#define packed_struct   __attribute__((packed))
#endif

#ifndef no_inline
#define no_inline       __attribute__((noinline))
#endif

#ifndef no_opt
#ifdef __clang__
#define no_opt          __attribute__((optnone))
#else
#define no_opt          __attribute__((optimize("-O0")))
#endif
#endif

#ifndef weak_func
#define weak_func       __attribute__((weak))
#endif

#ifndef weak_alias
#define weak_alias(f)   __attribute__((weak, alias(#f)))
#endif

#ifndef entry_func
#define entry_func      __attribute__((section("entry")))
#endif


#endif

