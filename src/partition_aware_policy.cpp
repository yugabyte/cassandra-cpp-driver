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

#include <iomanip>

#include "partition_aware_policy.hpp"

#include "batch_request.hpp"
#include "execute_request.hpp"
#include "jenkins_hash.hpp"
#include "logger.hpp"
#include "metadata.hpp"
#include "random.hpp"
#include "request_handler.hpp"
#include "session.hpp"

namespace cass {

#if 0 // Enable if used
static int32_t yb_to_cql_hash_code(int64_t yb_hash) {
  // Flip first bit so that negative values are smaller than positives.
  return static_cast<int32_t>(yb_hash >> 48) ^ 0x8000;
}

// Just for debug printing in TRACE log.
static const char* bytes_to_str(const char* data, size_t size) {
  std::stringstream ss;
  std::string text;
  ss << std::hex << std::uppercase;
  for (size_t i = 0; i < size; ++i) {
    const int v = static_cast<int>(static_cast<unsigned char>(data[i]));
    ss << ' ' << std::setfill('0') << std::setw(2) << v;
    text += (v >= 32 ? data[i] : '.'); // printable text
  }

  static std::string str;
  str = ss.str() + ": '" + text + "'";
  return str.c_str();
}

static const char* bytes_to_str(const Buffer& b) {
  return bytes_to_str(b.data(), b.size());
}
#endif

static int64_t cql_to_yb_hash_code(int32_t hash) {
  return static_cast<int64_t>(hash ^ 0x8000) << 48;
}

// The number of replicas is bounded by replication factor. In practice, the number
// of followers is fairly small so a linear search should be extremely fast.
static inline bool contains(const CopyOnWriteHostVec& replicas, const Address& address) {
  for (HostVec::const_iterator i = replicas->begin(); i != replicas->end(); ++i) {
    if ((*i)->address() == address) {
      return true;
    }
  }
  return false;
}

static int32_t bytes_to_key(const char* bytes, size_t size) {
    const uint64_t seed = 97;
    const uint64_t h = Hash64StringWithSeed(bytes, size, seed);
    uint64_t h1 = h >> 48;
    uint64_t h2 = 3 * (h >> 32);
    uint64_t h3 = 5 * (h >> 16);
    uint64_t h4 = 7 * (h & 0xffff);
    const int32_t result = static_cast<int32_t>((h1 ^ h2 ^ h3 ^ h4) & 0xffff);
    return result;
}

static bool get_hash_key_from_execute(
    const ExecuteRequest* execute, int32_t* hash_key, std::string* full_table_name) {
  const Prepared::ConstPtr& prepared = execute->prepared();

  const ResultResponse::ConstPtr& result = prepared->result();
  if (result.get() == NULL) {
    return false;
  }

  // Set full table name.
  *full_table_name = result->keyspace().to_string() + '.' + result->table().to_string();

  // Get binded values and write it into binary stream.
  const AbstractData::ElementVec& elems = execute->elements();
  BufferVec buffers;

  for (size_t i = 0; i < elems.size(); ++i) {
    const Buffer b = elems[i].get_buffer(CASS_HIGHEST_SUPPORTED_PROTOCOL_VERSION);
  }

  // Get primary keys indexes.
  const ResultResponse::PKIndexVec& ki = prepared->key_indices();

  const size_t header_size = sizeof(int32_t);
  size_t total_size = 0;
  for (size_t i : ki) {
    const Buffer b = elems[i].get_buffer(CASS_HIGHEST_SUPPORTED_PROTOCOL_VERSION);

    if (b.size() >= header_size) {
      buffers.push_back(b);
      total_size += b.size() - header_size;
    }
  }

  Buffer total(total_size);
  size_t offset = 0;
  for (const Buffer& b : buffers) {
    // Cut 32-bit buffer length in begin.
    assert(b.size() >= header_size);
    total.copy(offset, b.data() + header_size, b.size() - header_size);
    offset += b.size() - header_size;
  }

  // Create hash key from binary stream.
  *hash_key = bytes_to_key(total.data(), total.size());
  return true;
}

bool PartitionAwarePolicy::get_hash_code(
    const Request* request, int32_t* hash_key, std::string* full_table_name) {
  assert(request != NULL);
  assert(hash_key != NULL);
  assert(full_table_name != NULL);

  switch (request->opcode()) {
  case CQL_OPCODE_EXECUTE:
    return get_hash_key_from_execute(
        static_cast<const ExecuteRequest*>(request), hash_key, full_table_name);

  case CQL_OPCODE_BATCH: {
      for (const Statement::Ptr& statement :
          static_cast<const BatchRequest*>(request)->statements()) {
        if (get_hash_code(statement.get(), hash_key, full_table_name)) {
          return true;
        }
      }
    }
    break;

  default:
    LOG_TRACE("PartitionAwarePolicy does not support request OPCODE %d",
        static_cast<int>(request->opcode()));
  }

  return false;
}

bool PartitionAwarePolicy::get_yb_hash_code(
      const Request* request, int64_t* hash_key, std::string* full_table_name) {
  int32_t hash_key_32b = 0;
  if (! get_hash_code(request, &hash_key_32b, full_table_name)) {
    return false;
  }

  *hash_key = cql_to_yb_hash_code(hash_key_32b);
  return true;
}

void PartitionAwarePolicy::init(const Host::Ptr& connected_host,
                                const HostMap& hosts,
                                Random* random) {
  hosts_->reserve(hosts.size());
  std::transform(hosts.begin(), hosts.end(), std::back_inserter(*hosts_), GetHost());

  if (random != NULL) {
    index_ = random->next(std::max(static_cast<size_t>(1), hosts.size()));
  }

  ChainedLoadBalancingPolicy::init(connected_host, hosts, random);
}

void PartitionAwarePolicy::register_handles(uv_loop_t* loop) {
  ChainedLoadBalancingPolicy::register_handles(loop);

  refresh_metadata_task_ = PeriodicTask::start(loop,
                                               refresh_frequency_secs_*1000,
                                               this,
                                               PartitionAwarePolicy::on_work,
                                               PartitionAwarePolicy::on_after_work);
}

void PartitionAwarePolicy::close_handles() {
  if (refresh_metadata_task_) {
    PeriodicTask::stop(refresh_metadata_task_);
    refresh_metadata_task_.reset();
  }

  ChainedLoadBalancingPolicy::close_handles();
}

QueryPlan* PartitionAwarePolicy::new_query_plan(const std::string& keyspace,
                                                RequestHandler* request_handler) {
  QueryPlan* const child_plan = child_policy_->new_query_plan(keyspace, request_handler);

  if (request_handler == NULL || request_handler->request() == NULL || metadata() == NULL) {
    LOG_TRACE("Request or metadata is not available - child policy will be used");
    return child_plan;
  }

  std::string full_table_name;
  int32_t hash_key = 0;

  if (! get_hash_code(request_handler->request(), &hash_key, &full_table_name)) {
    LOG_TRACE("Cannot get routing key - child policy will be used");
    return child_plan;
  }

  const Metadata::SchemaSnapshot schema = metadata()->schema_snapshot(
      CASS_HIGHEST_SUPPORTED_PROTOCOL_VERSION, VersionNumber(3, 0, 0));
  const TableSplitMetadata::MapPtr& partitions = schema.get_partitions();

  LOG_TRACE("Full table name='%s', Key=0x%08x", full_table_name.c_str(), hash_key);

  TableSplitMetadata::Map::const_iterator it = partitions->find(full_table_name);

  if (it == partitions->end()) {
    LOG_WARN("No partition info found for table %s", full_table_name.c_str());
    return child_plan;
  }

  const PartitionMetadata::IpList* const ip_list = it->second.get_hosts(hash_key);

  if (ip_list == NULL) {
    LOG_WARN("Cannot find hosts for table %s and key %08x", full_table_name.c_str(), hash_key);
    return child_plan;
  }

  Host::Ptr leader;
  CopyOnWriteHostVec followers(new HostVec);

  for (PartitionMetadata::IpList::const_iterator ip_it = ip_list->begin();
      ip_it != ip_list->end(); ++ip_it) {
    const bool is_leader = (ip_it == ip_list->begin());

    // TODO: replace host vector by host map
    for (const Host::Ptr& host : *hosts_) {
      if (host->address().to_string(false) == *ip_it) {
        if (is_leader) {
          leader = host;
        } else {
          followers->push_back(host);
        }
        break;
      }
    }
  }

  return new PartitionAwareQueryPlan(leader, followers, index_++, child_plan);
}

void PartitionAwarePolicy::on_add(const Host::Ptr& host) {
  add_host(hosts_, host);
  ChainedLoadBalancingPolicy::on_add(host);
}

void PartitionAwarePolicy::on_remove(const Host::Ptr& host) {
  remove_host(hosts_, host);
  ChainedLoadBalancingPolicy::on_remove(host);
}

void PartitionAwarePolicy::on_up(const Host::Ptr& host) {
  add_host(hosts_, host);
  ChainedLoadBalancingPolicy::on_up(host);
}

void PartitionAwarePolicy::on_down(const Host::Ptr& host) {
  remove_host(hosts_, host);
  ChainedLoadBalancingPolicy::on_down(host);
}

Host::Ptr PartitionAwarePolicy::PartitionAwareQueryPlan::compute_next() {
  // Leader is preferred host.
  if (use_leader_ && leader_host_->is_up()) {
    use_leader_ = false;
    return leader_host_;
  }

  // Try followers.
  while (remaining_ > 0) {
    --remaining_;
    const Host::Ptr& host((*followers_)[index_++ % followers_->size()]);
    if (host->is_up()) {
      return host;
    }
  }

  // In worse case try to use child query plan.
  Host::Ptr host;
  while ((host = child_plan_->compute_next())) {
    if (!contains(followers_, host->address()) && !(leader_host_->address() == host->address())) {
      return host;
    }
  }

  return Host::Ptr();
}

void PartitionAwarePolicy::on_work(PeriodicTask* task) {
  PartitionAwarePolicy* const policy = static_cast<PartitionAwarePolicy*>(task->data());
  if (policy->control_connection() != NULL) {
    policy->control_connection()->refresh_partitions();
  }
}

void PartitionAwarePolicy::on_after_work(PeriodicTask*) {
  // No-op.
}

} // namespace cass
