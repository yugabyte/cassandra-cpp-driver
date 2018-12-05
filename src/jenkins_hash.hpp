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

#include <cstdint>

namespace cass {

uint64_t Hash64StringWithSeed(const char *s, uint32_t len, uint64_t c);

} // namespace cass

#endif // __CASS_JENKINS_HASH_HPP_INCLUDED__
