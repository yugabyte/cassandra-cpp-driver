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

#ifndef __CASS_PARTITION_AWARE_POLICY_HPP_INCLUDED__
#define __CASS_PARTITION_AWARE_POLICY_HPP_INCLUDED__

#include "load_balancing.hpp"
#include "host.hpp"
#include "random.hpp"
#include "cassandra.h"
#include "metadata.hpp"
#include "list_policy.hpp"

namespace cass {

class PartitionAwareQueryPlan: public QueryPlan {
 public:
  PartitionAwareQueryPlan(const PartitionAwarePolicy *policy,
                          cass::PartitionMetadata *partition_metadata,
                          int refresh_frequency_in_seconds);

  Host::Ptr compute_next();

  HostVec get_partition_aware_hosts(); // should take tablet metadata and key

  void recompute_partition_metadata(); // will force a recomputation of partition metadata

 private:
  const PartitionAwarePolicy *policy_;
  PartitionMetadata *partition_metadata_;
  HostVec partition_aware_hosts_;
  int refresh_frequency_in_seconds_;
};

class PartitionAwarePolicy: public ChainedLoadBalancingPolicy {
 public:
  PartitionAwarePolicy(LoadBalancingPolicy *child_policy,
                       int refresh_frequency_in_seconds,
                       SessionObjects session_objects)
      : child_policy_(child_policy),
        refresh_frequency_in_seconds_(refresh_frequency_in_seconds),
        session_objects_(session_objects) {}

  ~PartitionAwarePolicy() {}

   void init(const Host::Ptr &connected_host, const HostMap &hosts, Random *random);

  CassHostDistance distance(const Host::Ptr &host) { child_policy_->distance(host); };

  void on_add(const Host::Ptr &host) { child_policy_->on_add(host); }

  void on_remove(const Host::Ptr &host) { child_policy_->on_remove(host); }

  void on_up(const Host::Ptr &host) { child_policy_->on_up(host); }

  void on_down(const Host::Ptr &host) { child_policy_->on_down(host); }

  void QueryPlan* new_query_plan(const std::string& keyspace,
                                 RequestHandler* request_handler,
                                 const TokenMap* token_map);

 private:
  PartitionAwareQueryPlan* partition_aware_query_plan_;

  PartitionAwarePolicy *new_instance() {

  }

  SessionObjects session_objects_;

  int refresh_frequency_in_seconds_;

  bool is_valid_host(const Host::Ptr& host) { return true; }

};

} // namespace cass