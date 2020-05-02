/*
 * Substitues __movmem_i4_even from libgcc
 */
void __movmem_i4_even(void *src, void *dst, unsigned long len)
{
    unsigned char *sc = src, *dc = dst;
    for (unsigned long i = 0; i < len; i++) {
        dc[i] = sc[i];
    }
}
