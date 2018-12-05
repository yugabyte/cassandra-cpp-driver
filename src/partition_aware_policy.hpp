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
#ifndef __CASS_PARTITION_AWARE_POLICY_HPP_INCLUDED__
#define __CASS_PARTITION_AWARE_POLICY_HPP_INCLUDED__

#include "load_balancing.hpp"
#include "periodic_task.hpp"

namespace cass {

class CASS_EXPORT PartitionAwarePolicy: public ChainedLoadBalancingPolicy {
public:
  PartitionAwarePolicy(LoadBalancingPolicy *child_policy,
                       unsigned refresh_frequency_secs)
    : ChainedLoadBalancingPolicy(child_policy)
    , hosts_(new HostVec)
    , index_(0)
    , refresh_frequency_secs_(refresh_frequency_secs) {}

  virtual void init(const Host::Ptr& connected_host, const HostMap& hosts, Random* random);

  virtual void register_handles(uv_loop_t* loop);
  virtual void close_handles();

  virtual QueryPlan* new_query_plan(const std::string& keyspace,
                                    RequestHandler* request_handler);

  virtual void on_add(const Host::Ptr& host);
  virtual void on_remove(const Host::Ptr& host);
  virtual void on_up(const Host::Ptr& host);
  virtual void on_down(const Host::Ptr& host);

  virtual LoadBalancingPolicy* new_instance() {
    return new PartitionAwarePolicy(child_policy_->new_instance(), refresh_frequency_secs_);
  }

  static bool get_hash_code(
        const Request* request, int32_t* hash_key, std::string* full_table_name);

  static bool get_yb_hash_code(
      const Request* request, int64_t* hash_key, std::string* full_table_name);

private:
  class PartitionAwareQueryPlan : public QueryPlan {
  public:
    PartitionAwareQueryPlan(
        const Host::Ptr& leader, const CopyOnWriteHostVec& followers,
        size_t start_index, QueryPlan* child_plan)
      : use_leader_(true)
      , leader_host_(leader)
      , followers_(followers)
      , index_(start_index)
      , remaining_(followers->size())
      , child_plan_(child_plan) {}

    virtual Host::Ptr compute_next();

  private:
    bool use_leader_;
    Host::Ptr leader_host_;
    const CopyOnWriteHostVec followers_;
    size_t index_;
    size_t remaining_;
    ScopedPtr<QueryPlan> child_plan_;
  };

  static void on_work(PeriodicTask* task);
  static void on_after_work(PeriodicTask*);

  PeriodicTask::Ptr refresh_metadata_task_;
  CopyOnWriteHostVec hosts_;
  int index_;
  unsigned refresh_frequency_secs_;

private:
  DISALLOW_COPY_AND_ASSIGN(PartitionAwarePolicy);
};

} // namespace cass

#endif
