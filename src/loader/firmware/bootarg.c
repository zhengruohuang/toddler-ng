#include "common/include/inttypes.h"
#include "loader/include/devtree.h"
#include "loader/include/kprintf.h"
#include "loader/include/lib.h"


/*
 * Bootarg format
 *  key1=val1 key2 key3=skey1:sval1;skey2;skey3:sval3 key4=val4 ...
 */

static char *bootargs = "";


static int has_subkey(const char *subkey)
{
    return subkey && subkey[0];
}

static int is_term_char(const char *subkey, char ch)
{
    return ch == '\0' || ch == ' ' || ch == '\t' ||
        (has_subkey(subkey) && ch == ';');
}

static char *find_key_end(const char *key, const char *subkey)
{
    char *key_start = strstr(bootargs, key);
    char *subkey_start, *val_end;
    if (!key_start) {
        return NULL;
    }

    key_start += strlen(key);
    if (!has_subkey(subkey)) {
        // No subkey required
        return key_start;
    }

    val_end = strpbrk(key_start, " \t");
    subkey_start = strstr(key_start, subkey);
    if (val_end && subkey_start >= val_end) {
        return NULL;
    }

    subkey_start += strlen(subkey);
    return subkey_start;
}

char *bootargs_parse_data(const char *key, const char *subkey)
{
    char *key_end = find_key_end(key, subkey);

    if (!has_subkey(subkey)) {
        // No subkey required
        return key_end[0] == '=' ? key_end + 1 : NULL;
    } else {
        return key_end[0] == ':' ? key_end + 1 : NULL;
    }

    return NULL;
}

u64 bootargs_parse_u64(const char *key, const char *subkey, const int base,
    u64 def)
{
    char ch;
    u64 val = 0;

    char *s = bootargs_parse_data(key, subkey);
    if (!s) {
        return def;
    }

    for (ch = *s; !is_term_char(subkey, ch); ch = *++s) {
        if (base == 10) {
            panic_if(ch < '0' || ch > '9',
                "Unable to parse dec: %c\n", ch);
            val *= 10;
            val += ch - '0';
        }

        else if (base == 16) {
            // Skip 0x
            if (ch == '0' && (s[1] == 'x' || s[1] == 'X')) {
                s++;
                continue;
            }

            panic_if(!(ch >= '0' && ch <= '9') ||
                      (ch >= 'A' && ch <= 'F') ||
                      (ch >= 'a' && ch <= 'f'),
                     "Unable to parse hex: %c\n", ch);
            val *= 16;
            val += ch >= 'a' ? 10 + ch - 'a' :
                (ch >= 'A' ? 10 + ch - 'A' : ch - '0');
        }

        else {
            panic("Unsupported base: %d\n", base);
        }
    }

    return val;
}

u32 bootargs_parse_u32(const char *key, const char *subkey, const int base,
    u32 def)
{
    return (u32)bootargs_parse_u64(key, subkey, base, def);
}

ulong bootargs_parse_ulong(const char *key, const char *subkey, const int base,
    ulong def)
{
    return (ulong)bootargs_parse_u64(key, subkey, base, def);
}

int bootargs_parse_int(const char *key, const char *subkey, int def)
{
    char ch;
    int val = 0, neg = 0;

    char *s = bootargs_parse_data(key, subkey);
    if (!s) {
        return def;
    }

    if (*s == '-') {
        neg = 1;
        s++;
    }

    for (ch = *s; !is_term_char(subkey, ch); ch = *++s) {
        panic_if(ch < '0' || ch > '9',
            "Unable to parse dec: %c\n", ch);
        val *= 10;
        val += ch - '0';
    }

    return neg ? -val : val;
}

int bootargs_parse_char(const char *key, const char *subkey, int def)
{
    char *s = bootargs_parse_data(key, subkey);
    return s ? *s : def;
}

int bootargs_parse_str(const char *key, const char *subkey, char *buf,
    size_t len)
{
    char ch;
    size_t i;

    char *s = bootargs_parse_data(key, subkey);
    if (!s) {
        return 0;
    }

    for (ch = *s, i = 0; i < len - 1 && !is_term_char(subkey, ch);
        ch =*++s, i++
    ) {
        buf[i] = ch;
    }

    buf[i] = '\0';
    return i;
}

int bootargs_parse_switch(const char *key, const char *subkey, int set,
    int nset, int def)
{
    char *s = bootargs_parse_data(key, subkey);

    if (!s) {
        s = find_key_end(key, subkey);
        return s ? set : nset;
    }

    if ((s[0] == 'T' || s[0] == 't') && (s[1] == 'R' || s[1] == 'r') &&
        (s[2] == 'U' || s[2] == 'u') && (s[3] == 'E' || s[3] == 'e') &&
        is_term_char(subkey, s[4])
    ) {
        return set;
    }

    if ((s[0] == 'F' || s[0] == 'f') && (s[1] == 'A' || s[1] == 'a') &&
        (s[2] == 'L' || s[2] == 'l') && (s[3] == 'S' || s[3] == 's') &&
        (s[4] == 'E' || s[4] == 'e') && is_term_char(subkey, s[5])
    ) {
        return nset;
    }

    if ((s[0] == 'Y' || s[0] == 'y') && (s[1] == 'E' || s[1] == 'e') &&
        (s[2] == 'S' || s[2] == 's') && is_term_char(subkey, s[3])
    ) {
        return set;
    }

    if ((s[0] == 'N' || s[0] == 'n') && (s[1] == 'O' || s[1] == 'o') &&
        is_term_char(subkey, s[2])
    ) {
        return nset;
    }

    if (s[0] == '1' && is_term_char(subkey, s[1])) {
        return set;
    }

    if (s[0] == '0' && is_term_char(subkey, s[1])) {
        return nset;
    }

    return def;
}

int bootargs_parse_bool(const char *key, const char *subkey)
{
    return bootargs_parse_switch(key, subkey, 1, 0, 0);
}

void init_bootargs()
{
    struct devtree_node *chosen = devtree_walk("/chosen");
    if (!chosen) {
        return;
    }

    struct devtree_prop *prop = devtree_find_prop(chosen, "bootargs");
    if (!prop) {
        return;
    }

    bootargs = devtree_get_prop_data(prop);
    kprintf("Bootargs: %s\n", bootargs);
}
