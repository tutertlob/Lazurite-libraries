#ifndef _ASSERT_H
#define _ASSERT_H

#define __ASSERT_VOID_CAST  (void)

#ifdef NDEBUG
#define assert(expr)    (__ASSERT_VOID_CAST(0))
#else

#ifndef __assert
#ifdef __ASSERT_HALT
#define __assert    __assert_dhalt
#elif __ASSERT_STOP
#define __assert    __assert_stop
#else
#define __assert    __assert_brk
#endif
#endif /* __assert */

extern void __assert_dhalt(const char *assertion, const char *file, unsigned int line);
extern void __assert_stop(const char *assertion, const char *file, unsigned int line);
extern void __assert_brk(const char *assertion, const char *file, unsigned int line);

#define assert(expr)            \
    ((expr)                     \
    ? __ASSERT_VOID_CAST(0)     \
    : __assert (#expr, __FILE__, __LINE__))
#endif

#endif /* _ASSERT_H */
