/* Use of this source code is governed by the Apache 2.0 license; see COPYING.
 * Based on original SSE2 code by Tony Rogvall that is
 * copyright 2011 Teclo Networks AG. MIT licensed by Juho Snellman. */

/* IP checksum routine for AVX2. Sources:
   https://github.com/snabbco/snabb/blob/b9da7caa1928256a3b490867e9bc62dfbcd21ede/src/arch/avx2.c
   https://github.com/snabbco/snabb/blob/9fdcb99934e46babf24b0cef0f375aab493fcfbe/src/lib/checksum.c
*/

#include <stdint.h>
#include <arpa/inet.h>
#include <x86intrin.h>

uint16_t cksum_generic(unsigned char *p, size_t len, uint16_t initial) {
  uint32_t sum = htons(initial);
  const uint16_t *u16 = (const uint16_t *)p;

  while (len >= (sizeof(*u16) * 4)) {
    sum += u16[0];
    sum += u16[1];
    sum += u16[2];
    sum += u16[3];
    len -= sizeof(*u16) * 4;
    u16 += 4;
  }
  while (len >= sizeof(*u16)) {
    sum += *u16;
    len -= sizeof(*u16);
    u16 += 1;
  }

  /* if length is in odd bytes */
  if (len == 1)
    sum += *((const uint8_t *)u16);

  while(sum>>16)
    sum = (sum & 0xFFFF) + (sum>>16);
  return ntohs((uint16_t)~sum);
}

static inline uint32_t cksum_ua_loop(unsigned char *p, uint16_t n) {
  uint32_t s0 = 0;
  uint32_t s1 = 0;

  while (n) {
    s0 += p[0];
    s1 += p[1];
    p += 2;
    n--;
  }
  return (s0<<8)+s1;
}

static inline uint32_t cksum_avx2_loop(unsigned char *p, size_t n) {
    __m256i sum0, sum1, zero;
    uint32_t s[8] __attribute__((aligned(32))); // aligned for avx2 store
    uint32_t sum2;

    zero = _mm256_set_epi64x(0,0,0,0);
    sum0 = zero;
    sum1 = zero;

    while(n) {
        size_t k = (n >= 0xff) ? 0xff : n;
        __m256i t0,t1;
        __m256i s0 = zero;
        __m256i s1 = zero;
        n -= k;
        while (k) {
            __m256i src = _mm256_loadu_si256((__m256i const*) p);
            __m256i t;

            t = _mm256_unpacklo_epi8(src, zero);
            s0 = _mm256_adds_epu16(s0, t);
            t = _mm256_unpackhi_epi8(src, zero);
            s1 = _mm256_adds_epu16(s1, t);
            p += sizeof(src);
            k--;
        }

        // LOW - combine S0 and S1 into sum0
        t0 = _mm256_unpacklo_epi16(s0, zero);
        sum0 = _mm256_add_epi32(sum0, t0);
        t1 = _mm256_unpacklo_epi16(s1, zero);
        sum1 = _mm256_add_epi32(sum1, t1);

        // HIGH - combine S0 and S1 into sum1
        t0 = _mm256_unpackhi_epi16(s0, zero);
        sum0 = _mm256_add_epi32(sum0, t0);
        t1 = _mm256_unpackhi_epi16(s1, zero);
        sum1 = _mm256_add_epi32(sum1, t1);
    }
    // here we must sum the 4-32 bit sums into one 32 bit sum
    _mm256_store_si256((__m256i*)s, sum0);
    sum2 = (s[0]<<8) + s[1] + (s[2]<<8) + s[3] + (s[4]<<8) + s[5] + (s[6]<<8) + s[7];
    _mm256_store_si256((__m256i*)s, sum1);
    sum2 += (s[0]<<8) + s[1] + (s[2]<<8) + s[3] + (s[4]<<8) + s[5] + (s[6]<<8) + s[7];

    return sum2;
}

uint16_t cksum_avx2(unsigned char *p, size_t n, uint16_t initial) {
    uint32_t sum = initial;

    if (n < 128) { return cksum_generic(p, n, initial); }
    if (n >= 64) {
        size_t k = (n >> 5);
        sum += cksum_avx2_loop(p, k);
        n -= (32*k);
        p += (32*k);
    }
    if (n > 1) {
        size_t k = (n>>1);   // number of 16-bit words
        sum += cksum_ua_loop(p, k);
        n -= (2*k);
        p += (2*k);
    }
    if (n)       // take care of left over byte
        sum += (p[0] << 8);
    while(sum>>16)
        sum = (sum & 0xFFFF) + (sum>>16);
    return (uint16_t)~sum;
}
