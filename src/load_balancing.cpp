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

#include "load_balancing.hpp"

#include "session.hpp"

namespace cass {

ControlConnection* LoadBalancingPolicy::control_connection() {
  auto* session = session_.load(std::memory_order_acquire);
  return session ? session->control_connection() : nullptr;
}

const Metadata* LoadBalancingPolicy::metadata() const {
  const auto* session = session_.load(std::memory_order_acquire);
  return session ? &session->metadata() : nullptr;
}

const TokenMap* LoadBalancingPolicy::token_map() const {
  auto* session = session_.load(std::memory_order_acquire);
  return session ? session->token_map() : nullptr;
}

} // namespace cass
