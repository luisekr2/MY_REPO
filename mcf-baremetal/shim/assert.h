#ifndef BARE_ASSERT_H
#define BARE_ASSERT_H
void __assert_fail(const char *, const char *, unsigned int, const char *);
#define assert(x) ((x) ? (void)0 : __assert_fail(#x, __FILE__, __LINE__, __func__))
#endif
