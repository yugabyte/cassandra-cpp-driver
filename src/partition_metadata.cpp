#include "partition_metadata.hpp"
#include "metadata.hpp"
#incldue <map>

namespace cass {

  class PartitionMetadata {
   public:


   private:
    void load(ResultResponse rs) {
      unordered_map<TableMetadata, map<int, HostVec> > tableMap = new unordered_map<TableMetadata, map<int, HostVec> > ();
      for (Row row;;) { // figure out the right way to iterate over rows.
        string keyspace_name;
        KeyspaceMetadata *keyspace = cluster_metadata_->get_keyspace(row.get_string_by_name("keyspace_name", &keyspace_name));
        if (!keyspace) {
          continue;
        }

        TableMetadata table = keyspace->get_table(row.get_string_by_name("table_name"));
        if (!table) {
          continue;
        }

        // This is explanatory now, just implement.

      }

    }

  };


} // namespace cass
