// #include "common/include/inttypes.h"
// #include "loader/include/lib.h"
//
//
// /*
//  * String
//  */
// char *strcpy(char *dest, const char *src)
// {
//     char *ori = dest;
//
//     for (char ch = *src; ch; ch = *++src, dest++) {
//         *dest = ch;
//     }
//     *dest = '\0';
//
//     return ori;
// }
//
// char *strncpy(char *dest, const char *src, size_t n)
// {
//     char *ori = dest;
//
//     size_t i = 0;
//     for (char ch = *src; ch && i < n - 1; ch = *++src, dest++, i++) {
//         *dest = ch;
//     }
//     *dest = '\0';
//
//     return ori;
// }
//
// int strcmp(const char *s1, const char *s2)
// {
//     if (!s1 && !s2) {
//         return 0;
//     }
//
//     while (*s1 || *s2) {
//         if (*s1 != *s2) {
//             return *s1 > *s2 ? 1 : -1;
//         }
//
//         s1++;
//         s2++;
//     }
//
//     return 0;
// }
//
// int strncmp(const char *s1, const char *s2, size_t n)
// {
//     if (!s1 && !s2) {
//         return 0;
//     }
//
//     for (; n && (*s1 || *s2); n--, s1++, s2++) {
//         if (*s1 != *s2) {
//             return *s1 > *s2 ? 1 : -1;
//         }
//     }
//
//     return 0;
// }
//
// size_t strlen(const char *s)
// {
//     int len = 0;
//
//     while (*s++) {
//         len++;
//     }
//
//     return len;
// }
//
// char *strpbrk(const char *str1, const char *str2)
// {
//     if (!str1 || !str2) {
//         return NULL;
//     }
//
//     for (; *str1; str1++) {
//         for (const char *cmp = str2; *cmp; cmp++) {
//             if (*cmp == *str1) {
//                 return (char *)str1;
//             }
//         }
//     }
//
//     return NULL;
// }
//
// char *strchr(const char *str, int c)
// {
//     if (!str) {
//         return NULL;
//     }
//
//     for (; *str; str++) {
//         if (*str == c) {
//             return (char *)str;
//         }
//     }
//
//     return NULL;
// }
//
// char *strstr(const char *str, const char *needle)
// {
//     if (!str || !needle || !*needle) {
//         return NULL;
//     }
//
//     size_t len = strlen(needle);
//     for (char first = needle[0]; *str; str++) {
//         size_t remain_len = strlen(str);
//         if (remain_len < len) {
//             return NULL;
//         }
//
//         if (*str == first && !memcmp(str, needle, len)) {
//             return (char *)str;
//         }
//     }
//
//     return NULL;
// }
//
//
// /*
//  * Memory
//  */
// void *memcpy(void *dest, const void *src, size_t count)
// {
//     ulong i;
//
//     u8 *s = (u8 *)src;
//     u8 *d = (u8 *)dest;
//
//     for (i = 0; i < count; i++) {
//         d[i] = s[i];
//     }
//
//     return dest;
// }
//
// void *memset(void *src, int value, size_t size)
// {
//     ulong i;
//     u8 *ptr = (u8 *)src;
//     u8 val8 = (u8)value;
//
//     for (i = 0; i < size; i++) {
//         ptr[i] = val8;
//     }
//
//     return src;
// }
//
// void *memzero(void *src, size_t size)
// {
//     return memset(src, 0, size);
// }
//
// int memcmp(const void *src1, const void *src2, size_t len)
// {
//     u8 *cmp1 = (u8 *)src1;
//     u8 *cmp2 = (u8 *)src2;
//
//     ulong i;
//     int result = 0;
//
//     for (i = 0; i < len; i++) {
//         if (cmp1[i] > cmp2[i]) {
//             return 1;
//         } else if (cmp1[i] < cmp2[i]) {
//             return -1;
//         }
//     }
//
//     return result;
// }
//
// void *memchr(const void *blk, int c, size_t size)
// {
//     if (!blk) {
//         return NULL;
//     }
//
//     const char *s = blk;
//     for (size_t i = 0; i < size; i++, s++) {
//         if (*s == c) {
//             return (void *)s;
//         }
//     }
//
//     return NULL;
// }
//
//
// void *memstr(const void *blk, const char *needle, size_t size)
// {
//     if (!blk || !needle || !*needle) {
//         return NULL;
//     }
//
//     size_t l = strlen(needle);
//     if (size < l) {
//         return NULL;
//     }
//
//     char first = needle[0];
//     const char *s = blk;
//
//     size -= l - 1;
//     for (size_t i = 0; i < size; i++, s++) {
//         if (*s == first && !memcmp(s, needle, l)) {
//             return (void *)s;
//         }
//     }
//
//     return NULL;
// }
//
//
// /*
//  * Digit and alpha
//  */
// int isdigit(int ch)
// {
//     return ch && (ch >= '0' && ch <= '9');
// }
//
// int isxdigit(int ch)
// {
//     return ch && (
//         (ch >= '0' && ch <= '9') ||
//         (ch >= 'a' && ch <= 'f') ||
//         (ch >= 'A' && ch <= 'F')
//     );
// }
//
//
// /*
//  * String to number
//  */
// u64 stou64(const char *str, int base)
// {
//     if (!str || !*str) {
//         return 0;
//     }
//
//     bool neg = false;
//     if (*str == '-') {
//         str++;
//         neg = true;
//     }
//
//     if ((base == 0 || base == 16) &&
//         (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
//     ) {
//         str += 2;
//         base = 16;
//     }
//
//     u64 num = 0;
//     for (char ch = *str; isxdigit(ch); ch = *++str) {
//         if (base == 16) {
//             num *= 16ull;
//         } else {
//             num *= 10ull;
//         }
//
//         if (ch >= '0' && ch <= '9') {
//             num += ch - '0';
//         } else if (base == 16 && ch >= 'a' && ch <= 'f') {
//             num += ch - 'a' + 10;
//         } else if (base == 16 && ch >= 'A' && ch <= 'F') {
//             num += ch - 'A' + 10;
//         }
//     }
//
//     return neg ? -num : num;
// }
//
// u32 stou32(const char *str, int base)
// {
//     return (u32)stou64(str, base);
// }
//
// ulong stoul(const char *str, int base)
// {
//     return (ulong)stou64(str, base);
// }
//
// int stoi(const char *str, int base)
// {
//     return (int)stou64(str, base);
// }
//
// int atoi(const char *str)
// {
//     return stoi(str, 10);
// }
