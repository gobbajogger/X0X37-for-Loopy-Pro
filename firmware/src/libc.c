/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* The few libc routines the compiler may call implicitly (struct copies,
 * zeroing). Freestanding: no other libc. */
/* Word at a time when both ends allow it: the audio jobs zero and copy whole float
 * buffers and voice states (a byte loop cost ~6 cycles a byte on the AC79). The bytes
 * written are the same, front to back, one word read before it is written (as the byte
 * loop, also for an overlapping copy). */
void *memset(void *d, int c, unsigned n)
{
    unsigned char *p = d;
    if (n >= 8u && !((unsigned)p & 3u)) {
        unsigned w = (unsigned char)c;
        unsigned *q = (unsigned *)(void *)p;
        w |= w << 8;
        w |= w << 16;
        for (; n >= 16u; n -= 16u) {   /* 4 words a loop pass: fewer taken branches */
            q[0] = w; q[1] = w; q[2] = w; q[3] = w;
            q += 4;
        }
        for (; n >= 4u; n -= 4u)
            *q++ = w;
        p = (unsigned char *)q;
    }
    while (n--)
        *p++ = (unsigned char)c;
    return d;
}

void *memcpy(void *d, const void *s, unsigned n)
{
    unsigned char *p = d;
    const unsigned char *q = s;
    if (n >= 8u && !(((unsigned)p | (unsigned)q) & 3u)) {
        unsigned *pw = (unsigned *)(void *)p;
        const unsigned *qw = (const unsigned *)(const void *)q;
        /* 4 words a loop pass, each word still read just before it is written
         * (same access order as one word a pass) */
        for (; n >= 16u; n -= 16u) {
            pw[0] = qw[0]; pw[1] = qw[1]; pw[2] = qw[2]; pw[3] = qw[3];
            pw += 4; qw += 4;
        }
        for (; n >= 4u; n -= 4u)
            *pw++ = *qw++;
        p = (unsigned char *)pw;
        q = (const unsigned char *)qw;
    }
    while (n--)
        *p++ = *q++;
    return d;
}

int memcmp(const void *a, const void *b, unsigned n)
{
    const unsigned char *p = a, *q = b;
    for (; n; n--, p++, q++)
        if (*p != *q)
            return *p - *q;
    return 0;
}

static int str_eq(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static unsigned str_len(const char *s)
{
    unsigned n = 0;
    while (s[n])
        n++;
    return n;
}

static void str_cpy(char *d, const char *s, unsigned max)
{
    unsigned i = 0;
    while (s[i] && i + 1 < max) {
        d[i] = s[i];
        i++;
    }
    d[i] = 0;
}

/* decimal with an optional fixed point: fmt_fix(buf, -63, 1) -> "-6.3" */
static void fmt_fix(char *b, int32_t v, int decimals)
{
    char t[12];
    int n = 0, neg = v < 0, i;
    uint32_t u = (uint32_t)(neg ? -v : v);
    do {
        t[n++] = (char)('0' + u % 10u);
        u /= 10u;
        if (n == decimals)
            t[n++] = '.';
    } while (u || n <= decimals);
    if (neg)
        t[n++] = '-';
    for (i = 0; i < n; i++)
        b[i] = t[n - 1 - i];
    b[n] = 0;
}

static void fmt_int(char *b, int32_t v) { fmt_fix(b, v, 0); }

static uint32_t rng_state = 0x1234567u;
static uint32_t rng(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}
