// Copyright (c) YugaByte, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.
//
// Contains the legacy Bob Jenkins Lookup2-based hashing routines. These need to
// always return the same results as their values have been recorded in various
// places and cannot easily be updated. Original author: Sanjay Ghemawat

#include "jenkins_hash.hpp"

// Detecting the *endianness* of the machine.
// Default detection implemented with reference to
// http://www.boost.org/doc/libs/1_42_0/boost/detail/endian.hpp

// Note, you might want to remove the part which defines big endian because this is not even
// supported for big endian systems. This is just supported for little endian systems.
// Or the reverse, just check it out.
#if defined (__GLIBC__)
# include <endian.h>
# if (__BYTE_ORDER == __LITTLE_ENDIAN)
#  define JENKINS_LITTLE_ENDIAN
# elif (__BYTE_ORDER == __BIG_ENDIAN)
#  define JENKINS_BIG_ENDIAN
# elif (__BYTE_ORDER == __PDP_ENDIAN)
#  define JENKINS_PDP_ENDIAN
# else
#  error Unknown machine endianness detected.
# endif
# define JENKINS_BYTE_ORDER __BYTE_ORDER
#elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
# define JENKINS_BIG_ENDIAN
# define JENKINS_BYTE_ORDER 4321
#elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
# define JENKINS_LITTLE_ENDIAN
# define JENKINS_BYTE_ORDER 1234
#elif defined(__sparc) || defined(__sparc__) \
 || defined(_POWER) || defined(__powerpc__) \
 || defined(__ppc__) || defined(__hpux) || defined(__hppa) \
 || defined(_MIPSEB) || defined(_POWER) \
 || defined(__s390__)
# define JENKINS_BIG_ENDIAN
# define JENKINS_BYTE_ORDER 4321
#elif defined(__i386__) || defined(__alpha__) \
 || defined(__ia64) || defined(__ia64__) \
 || defined(_M_IX86) || defined(_M_IA64) \
 || defined(_M_ALPHA) || defined(__amd64) \
 || defined(__amd64__) || defined(_M_AMD64) \
 || defined(__x86_64) || defined(__x86_64__) \
 || defined(_M_X64) || defined(__bfin__)

# define JENKINS_LITTLE_ENDIAN
# define JENKINS_BYTE_ORDER 1234
// From https://stackoverflow.com/questions/4239993/determining-endianness-at-compile-time/4240029
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || \
    defined(__BIG_ENDIAN__) || \
    defined(__ARMEB__) || \
    defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || \
    defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
# define JENKINS_BIG_ENDIAN
# define JENKINS_BYTE_ORDER 4321
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || \
    defined(__LITTLE_ENDIAN__) || \
    defined(__ARMEL__) || \
    defined(__THUMBEL__) || \
    defined(__AARCH64EL__) || \
    defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
# define JENKINS_LITTLE_ENDIAN
# define JENKINS_BYTE_ORDER 1234
#else
# error The file cassandra-cpp-driver/src/jenkins_hash.hpp needs to be set up for your CPU type.
#endif

#define JENKINS_ULONGLONG(x) x##ULL
#define UNALIGNED_LOAD64(_p) (*reinterpret_cast<const uint64_t *>(_p))

namespace cass {

// ----------------------------------------------------------------------
// mix()
//    The mix function is due to Bob Jenkins (see
//    http://burtleburtle.net/bob/hash/index.html).
//    Each mix takes 36 instructions, in 18 cycles if you're lucky.
//
//    On x86 architectures, this requires 45 instructions in 27 cycles,
//    if you're lucky.
// ----------------------------------------------------------------------
static inline void mix(uint64_t &a, uint64_t &b, uint64_t &c) { // 64bit version
  a -= b; a -= c; a ^= (c>>43);
  b -= c; b -= a; b ^= (a<<9);
  c -= a; c -= b; c ^= (b>>8);
  a -= b; a -= c; a ^= (c>>38);
  b -= c; b -= a; b ^= (a<<23);
  c -= a; c -= b; c ^= (b>>5);
  a -= b; a -= c; a ^= (c>>35);
  b -= c; b -= a; b ^= (a<<49);
  c -= a; c -= b; c ^= (b>>11);
  a -= b; a -= c; a ^= (c>>12);
  b -= c; b -= a; b ^= (a<<18);
  c -= a; c -= b; c ^= (b>>22);
}

#if defined(JENKINS_LITTLE_ENDIAN) && !defined(NEED_ALIGNED_LOADS)

static inline uint64_t Word64At(const char *ptr) {
  return UNALIGNED_LOAD64(ptr);
}

#else

// Note this code is unlikely to be used.
static inline uint64_t Word64At(const char *ptr) {
    return  (static_cast<uint64_t>(ptr[0]) +
            (static_cast<uint64_t>(ptr[1]) << 8) +
            (static_cast<uint64_t>(ptr[2]) << 16) +
            (static_cast<uint64_t>(ptr[3]) << 24) +
            (static_cast<uint64_t>(ptr[4]) << 32) +
            (static_cast<uint64_t>(ptr[5]) << 40) +
            (static_cast<uint64_t>(ptr[6]) << 48) +
            (static_cast<uint64_t>(ptr[7]) << 56));
}

#endif

static inline uint64_t char2unsigned64(char c) {
  return static_cast<uint64_t>(static_cast<unsigned char>(c));
}

uint64_t Hash64StringWithSeed(const char *s, uint32_t len, uint64_t c) {
  uint64_t a = JENKINS_ULONGLONG(0xe08c1d668b756f82); // Golden ratio; an arbitrary value.
  uint64_t b = a;
  uint32_t keylen = len;

  for (; keylen >= 3 * sizeof(a);
      keylen -= 3 * static_cast<uint32_t>(sizeof(a)), s += 3 * sizeof(a)) {
    a += Word64At(s);
    b += Word64At(s + sizeof(a));
    c += Word64At(s + sizeof(a) * 2);
    mix(a, b, c);
  }

  c += len;
  switch (keylen) { // Deal with rest. Cases fall through.
    case 23:
      c += char2unsigned64(s[22]) << 56;
      /* FALLTHRU */
    case 22:
      c += char2unsigned64(s[21]) << 48;
      /* FALLTHRU */
    case 21:
      c += char2unsigned64(s[20]) << 40;
      /* FALLTHRU */
    case 20:
      c += char2unsigned64(s[19]) << 32;
      /* FALLTHRU */
    case 19:
      c += char2unsigned64(s[18]) << 24;
      /* FALLTHRU */
    case 18:
      c += char2unsigned64(s[17]) << 16;
      /* FALLTHRU */
    case 17:
      c += char2unsigned64(s[16]) << 8;
      // The first byte of c is reserved for the length.
      /* FALLTHRU */
    case 16:
      b += Word64At(s + 8);
      a += Word64At(s);
      break;
    case 15:
      b += char2unsigned64(s[14]) << 48;
      /* FALLTHRU */
    case 14:
      b += char2unsigned64(s[13]) << 40;
      /* FALLTHRU */
    case 13:
      b += char2unsigned64(s[12]) << 32;
      /* FALLTHRU */
    case 12:
      b += char2unsigned64(s[11]) << 24;
      /* FALLTHRU */
    case 11:
      b += char2unsigned64(s[10]) << 16;
      /* FALLTHRU */
    case 10:
      b += char2unsigned64(s[9]) << 8;
      /* FALLTHRU */
    case 9:
      b += char2unsigned64(s[8]);
      /* FALLTHRU */
    case 8:
      a += Word64At(s);
      break;
    case 7:
      a += char2unsigned64(s[6]) << 48;
      /* FALLTHRU */
    case 6:
      a += char2unsigned64(s[5]) << 40;
      /* FALLTHRU */
    case 5:
      a += char2unsigned64(s[4]) << 32;
      /* FALLTHRU */
    case 4:
      a += char2unsigned64(s[3]) << 24;
      /* FALLTHRU */
    case 3:
      a += char2unsigned64(s[2]) << 16;
      /* FALLTHRU */
    case 2:
      a += char2unsigned64(s[1]) << 8;
      /* FALLTHRU */
    case 1:
      a += char2unsigned64(s[0]);
      /* FALLTHRU */
    // case 0: Nothing left to add.
  }
  mix(a, b, c);
  return c;
}

} // namespace cass
