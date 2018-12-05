#include "partition_aware_policy.hpp"

#include "logger.hpp"

namespace cass {

// Partition Aware Policy should also implement functions like:

// getKey(for a statement), getKey(for a byte), YBToCqlHashCode, CQlToYBHashCode.
// After that the implementation of the iterator is extremely simple.
// You have also decided to create a query plan object and pass down the Session object
// to the query object.

class PartitionAwareQueryPlan: public QueryPlan {

};

class PartitionAwarePolicy: public ChainedLoadBalancingPolicy {
 public:
  void init(const Host::Ptr &connected_host,
            const HostMap &hosts,
            Random *random) {
    HostMap valid_hosts;
    for (HostMap::iterator i = hosts.begin(),
        end = hosts.end(); i != end; ++i) {
      const Host::Ptr &host = i->second;
      if (is_valid_host(host)) { // might want to define valid hosts as the ones up.
        valid_hosts.insert(HostPair(i->first, host));
      }
    }

    if (valid_hosts.empty()) {
      LOG_ERROR("No valid hosts available for list policy.");
    }
  }

 private:


};

} // namespace cass