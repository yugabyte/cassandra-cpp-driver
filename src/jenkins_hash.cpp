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
// Contains the legacy Bob Jenkins Lookup2-based hashing routines. These need to
// always return the same results as their values have been recorded in various
// places and cannot easily be updated. Original author: Sanjay Ghemawat

#include "jenkins_hash.hpp"

namespace cass {

uint64_t Hash64StringWithSeed(const char *s, uint32_t len, uint64_t c) {
  uint64_t a, b;
  uint32_t keylen;

  a = b = JENKINS_ULONGLONG(0xe08c1d668b756f82);   // the golden ratio; an arbitrary value

  for (keylen = len; keylen >= 3 * sizeof(a);
      keylen -= 3 * static_cast<uint32_t>(sizeof(a)), s += 3 * sizeof(a)) {
    a += Word64At(s);
    b += Word64At(s + sizeof(a));
    c += Word64At(s + sizeof(a) * 2);
    mix(a, b, c);
  }

  c += len;
  switch (keylen) {           // deal with rest.  Cases fall through
    case 23:
      c += char2unsigned64(s[22]) << 56;
    case 22:
      c += char2unsigned64(s[21]) << 48;
    case 21:
      c += char2unsigned64(s[20]) << 40;
    case 20:
      c += char2unsigned64(s[19]) << 32;
    case 19:
      c += char2unsigned64(s[18]) << 24;
    case 18:
      c += char2unsigned64(s[17]) << 16;
    case 17:
      c += char2unsigned64(s[16]) << 8;
      // the first byte of c is reserved for the length
    case 16:
      b += Word64At(s + 8);
      a += Word64At(s);
      break;
    case 15:
      b += char2unsigned64(s[14]) << 48;
    case 14:
      b += char2unsigned64(s[13]) << 40;
    case 13:
      b += char2unsigned64(s[12]) << 32;
    case 12:
      b += char2unsigned64(s[11]) << 24;
    case 11:
      b += char2unsigned64(s[10]) << 16;
    case 10:
      b += char2unsigned64(s[9]) << 8;
    case 9:
      b += char2unsigned64(s[8]);
    case 8:
      a += Word64At(s);
      break;
    case 7:
      a += char2unsigned64(s[6]) << 48;
    case 6:
      a += char2unsigned64(s[5]) << 40;
    case 5:
      a += char2unsigned64(s[4]) << 32;
    case 4:
      a += char2unsigned64(s[3]) << 24;
    case 3:
      a += char2unsigned64(s[2]) << 16;
    case 2:
      a += char2unsigned64(s[1]) << 8;
    case 1:
      a += char2unsigned64(s[0]);
      // case 0: nothing left to add
  }
  mix(a, b, c);
  return c;
}

} // namespace cass
