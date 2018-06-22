// Copyright 2011 Google Inc. All Rights Reserved.
//
// The following only applies to changes made to this file as part of YugaByte development.
//
// Portions Copyright (c) YugaByte, Inc.
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

/**
 * A hash function that returns a 64-bit hash value.
 *
 * This is the implementation of
 * Bob Jenkin's hash function (http://burtleburtle.net/bob/hash/evahash.html).
 *
 * The hash function takes a string as a input and a start seed. Hashing the same
 * string with two different seeds should give two independent hash values. The
 * functions just do a single mix, in order to convert the key into something
 * *random*. This hash function is guarenteed to have no funnels, i.e there is no
 * theoretical limit on the number of bits in the internal state a subset of bits
 * of the input can change. Hence the chances of collisions are low even if the
 * input differs only by a few bits.
 *
 */

#ifndef __CASS_JENKINS_HASH_HPP_INCLUDED__
#define __CASS_JENKINS_HASH_HPP_INCLUDED__


#include <stdint.h>

namespace cass {
/*
 * Detecting the *endianness* of the machine.
 * Default detection implemented with reference to
 * http://www.boost.org/doc/libs/1_42_0/boost/detail/endian.hpp
*/

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
#else
# error The file cassandra-cpp-driver/src/jenkins_hash.hpp needs to be set up for your CPU type.
#endif

#define JENKINS_ULONGLONG(x) x##ULL
// ----------------------------------------------------------------------
// mix()
//    The mix function is due to Bob Jenkins (see
//    http://burtleburtle.net/bob/hash/index.html).
//    Each mix takes 36 instructions, in 18 cycles if you're lucky.
//
//    On x86 architectures, this requires 45 instructions in 27 cycles,
//    if you're lucky.
// ----------------------------------------------------------------------

static inline void mix(uint64_t &a, uint64_t &b, uint64_t &c) {     // 64bit version
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

#define UNALIGNED_LOAD64(_p) (*reinterpret_cast<const uint64_t *>(_p))

#if defined(JENKINS_LITTLE_ENDIAN) && !defined(NEED_ALIGNED_LOADS)
static inline uint64_t Word64At(const char *ptr) {
  return UNALIGNED_LOAD64(ptr);
}

#else

// Note this code is unlikely to be used.
static inline uint64_t Word64At(const char *ptr) {
    return (static_cast<uint64_t>(ptr[0]) +
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

uint64_t Hash64StringWithSeed(const char *s, uint32_t len, uint64_t c);

} // namespace cass

#endif // __CASS_JENKINS_HASH_HPP_INCLUDED__