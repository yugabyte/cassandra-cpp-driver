#include "cassandra.h"
#include "response.hpp"
#include "result_response.hpp"
#inlcude "future.hpp"
#include "session.hpp"
#include "cluster.hpp"

#include <map>
#include <unordered_map>
#include <map>

namespace cass {

class PartitionMetadata {
 public:
  PartitionMetadata(Cluster *cluster, Session *session, Future *future) {
    cluster_ = cluster;
    session_ = session;
    future_ = future;
    execute_query();
  }

  void update_result_set();

 private:
  // static bool isSystem();

  // static int getKey();
  unordered_map <TableMetadata, map<int, HostVec>> tableMap;

  static string partitions_query_;

  Cluster *cluster_;

  Session *session_;

  Future *close_future_;

  ResultResponse *result_response_;

  HostVec all_hosts_;

  HostVec up_hosts_;

  Metadata *cluster_metadata_;

  void execute_query();

  void load();

  // Not sure if this is absolutely needed.
  void loadAsync();

  int refresh_frequency_in_seconds_;

  string PARTITIONS_QUERY =
      "select keyspace_name, table_name, replica_addresses from system.partitions;";

};

} // namespace cass