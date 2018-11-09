# YugaByte C++ Driver for YugaByte DB's Cassandra-compatible YCQL API

A modern, feature-rich] and highly tunable C/C++ client library for YugaByte DB's Cassandra-compatible YCQL API
using exclusively Cassandra's binary protocol and Cassandra Query Language v3. 

## Getting the Driver

The source code is made available via [GitHub]. The build instructions are at [Getting Started].

## Features

* [Asynchronous API]
* [Simple], [Prepared], and [Batch] statements
* [Asynchronous I/O], [parallel execution], and request pipelining
* Connection pooling
* Automatic node discovery
* Automatic reconnection
* Configurable [load balancing]
* Works with any cluster size
* [Authentication]
* [SSL]
* [Latency-aware routing]
* [Performance metrics]
* [Tuples] and [UDTs]
* [Nested collections]
* [Retry policies]
* [Client-side timestamps]
* [Data types]
* [Idle connection heartbeats]
* Support for materialized view and secondary index metadata
* Support for clustering key order, `frozen<>` and Cassandra version metadata
* [Blacklist], [whitelist DC], and [blacklist DC] load balancing policies
* [Custom] authenticators
* [Reverse DNS] with SSL peer identity verification support
* Randomized contact points
* [Speculative execution]

## Compatibility

This driver works exclusively with the Cassandra Query Language v3 (CQL3) and
Cassandra's native protocol for YugaByte DB's Cassandra-compatible YCQL API.

## Documentation

* [Home]
* [API]
* [Getting Started]
* [Building]

## Getting Help

Please report any issues on github.

## Examples

The driver includes several examples in the [examples] directory.

## A Simple Example
```c
#include <cassandra.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
  /* Setup and connect to cluster */
  CassFuture* connect_future = NULL;
  CassCluster* cluster = cass_cluster_new();
  CassSession* session = cass_session_new();
  char* hosts = "127.0.0.1";
  if (argc > 1) {
    hosts = argv[1];
  }

  /* Add contact points */
  cass_cluster_set_contact_points(cluster, hosts);

  /* Provide the cluster object as configuration to connect the session */
  connect_future = cass_session_connect(session, cluster);

  if (cass_future_error_code(connect_future) == CASS_OK) {
    CassFuture* close_future = NULL;

    /* Build statement and execute query */
    const char* query = "SELECT release_version FROM system.local";
    CassStatement* statement = cass_statement_new(query, 0);

    CassFuture* result_future = cass_session_execute(session, statement);

    if (cass_future_error_code(result_future) == CASS_OK) {
      /* Retrieve result set and get the first row */
      const CassResult* result = cass_future_get_result(result_future);
      const CassRow* row = cass_result_first_row(result);

      if (row) {
        const CassValue* value = cass_row_get_column_by_name(row, "release_version");

        const char* release_version;
        size_t release_version_length;
        cass_value_get_string(value, &release_version, &release_version_length);
        printf("release_version: '%.*s'\n", (int)release_version_length, release_version);
      }

      cass_result_free(result);
    } else {
      /* Handle error */
      const char* message;
      size_t message_length;
      cass_future_error_message(result_future, &message, &message_length);
      fprintf(stderr, "Unable to run query: '%.*s'\n", (int)message_length, message);
    }

    cass_statement_free(statement);
    cass_future_free(result_future);

    /* Close the session */
    close_future = cass_session_close(session);
    cass_future_wait(close_future);
    cass_future_free(close_future);
  } else {
    /* Handle error */
    const char* message;
    size_t message_length;
    cass_future_error_message(connect_future, &message, &message_length);
    fprintf(stderr, "Unable to connect: '%.*s'\n", (int)message_length, message);
  }

  cass_future_free(connect_future);
  cass_cluster_free(cluster);
  cass_session_free(session);

  return 0;
}
```

## License
Copyright (c) 2018 YugaByte, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

[Apache Cassandra]: http://cassandra.apache.org
[DSE C/C++ driver]: http://docs.datastax.com/en/developer/cpp-driver-dse/latest
[DataStax Enterprise]: http://www.datastax.com/products/datastax-enterprise
[Examples]: examples/
[download server]: http://downloads.datastax.com/cpp-driver
[GitHub]: https://github.com/datastax/cpp-driver
[cpp-driver-compatability-matrix]: https://docs.datastax.com/en/developer/driver-matrix/doc/cppDrivers.html#cpp-drivers
[Home]: https://github.com/YugaByte/cassandra-cpp-driver
[API]: http://docs.datastax.com/en/developer/cpp-driver/latest/api
[Getting Started]: https://docs.yugabyte.com/latest/develop/client-drivers/cpp/
[Building]: http://docs.datastax.com/en/developer/cpp-driver/latest/topics/building
[Provide your input]: http://goo.gl/forms/ihKC5uEQr6
[centos-6-dependencies]: http://downloads.datastax.com/cpp-driver/centos/6/dependencies
[centos-7-dependencies]: http://downloads.datastax.com/cpp-driver/centos/7/dependencies
[ubuntu-14-04-dependencies]: http://downloads.datastax.com/cpp-driver/ubuntu/14.04/dependencies
[ubuntu-16-04-dependencies]: http://downloads.datastax.com/cpp-driver/ubuntu/16.04/dependencies
[ubuntu-18-04-dependencies]: http://downloads.datastax.com/cpp-driver/ubuntu/18.04/dependencies
[windows-dependencies]: http://downloads.datastax.com/cpp-driver/windows/dependencies

[Asynchronous API]: http://datastax.github.io/cpp-driver/topics/#futures
[Simple]: http://datastax.github.io/cpp-driver/topics/#executing-queries
[Prepared]: http://datastax.github.io/cpp-driver/topics/basics/prepared_statements/
[Batch]: http://datastax.github.io/cpp-driver/topics/basics/batches/
[Asynchronous I/O]: http://datastax.github.io/cpp-driver/topics/#asynchronous-i-o
[parallel execution]: http://datastax.github.io/cpp-driver/topics/#thread-safety
[load balancing]: http://datastax.github.io/cpp-driver/topics/configuration/#load-balancing
[Authentication]: http://datastax.github.io/cpp-driver/topics/security/#authentication
[SSL]: http://datastax.github.io/cpp-driver/topics/security/ssl/
[Latency-aware routing]: http://datastax.github.io/cpp-driver/topics/configuration/#latency-aware-routing
[Performance metrics]: http://datastax.github.io/cpp-driver/topics/metrics/
[Tuples]: http://datastax.github.io/cpp-driver/topics/basics/tuples/
[UDTs]: http://datastax.github.io/cpp-driver/topics/basics/user_defined_types/
[Nested collections]: http://datastax.github.io/cpp-driver/topics/basics/binding_parameters/#nested-collections
[Data types]: http://datastax.github.io/cpp-driver/topics/basics/data_types/
[Retry policies]: http://datastax.github.io/cpp-driver/topics/configuration/retry_policies/
[Client-side timestamps]: http://datastax.github.io/cpp-driver/topics/basics/client_side_timestamps/
[Idle connection heartbeats]: http://datastax.github.io/cpp-driver/topics/configuration/#connection-heartbeats
[Blacklist]: http://datastax.github.io/cpp-driver/topics/configuration/#blacklist
[whitelist DC]: http://datastax.github.io/cpp-driver/topics/configuration/#datacenter
[blacklist DC]: http://datastax.github.io/cpp-driver/topics/configuration/#datacenter
[Custom]: http://datastax.github.io/cpp-driver/topics/security/#custom
[Reverse DNS]: http://datastax.github.io/cpp-driver/topics/security/ssl/#enabling-cassandra-identity-verification
[Speculative execution]: http://datastax.github.io/cpp-driver/topics/configuration/#speculative-execution
