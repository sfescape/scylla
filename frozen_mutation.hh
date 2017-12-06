/*
 * Copyright (C) 2015 ScyllaDB
 */

/*
 * This file is part of Scylla.
 *
 * Scylla is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Scylla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Scylla.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "dht/i_partitioner.hh"
#include "atomic_cell.hh"
#include "database_fwd.hh"
#include "mutation_partition_view.hh"
#include "streamed_mutation.hh"
#include "flat_mutation_reader.hh"

class mutation;
class streamed_mutation;

namespace ser {
class mutation_view;
}

// Immutable, compact form of mutation.
//
// This form is primarily destined to be sent over the network channel.
// Regular mutation can't be deserialized because its complex data structures
// need schema reference at the time object is constructed. We can't lookup
// schema before we deserialize column family ID. Another problem is that even
// if we had the ID somehow, low level RPC layer doesn't know how to lookup
// the schema. Data can be wrapped in frozen_mutation without schema
// information, the schema is only needed to access some of the fields.
//
class frozen_mutation final {
private:
    bytes_ostream _bytes;
    partition_key _pk;
private:
    partition_key deserialize_key() const;
    ser::mutation_view mutation_view() const;
public:
    frozen_mutation(const mutation& m);
    explicit frozen_mutation(bytes_ostream&& b);
    frozen_mutation(bytes_ostream&& b, partition_key key);
    frozen_mutation(frozen_mutation&& m) = default;
    frozen_mutation(const frozen_mutation& m) = default;
    frozen_mutation& operator=(frozen_mutation&&) = default;
    frozen_mutation& operator=(const frozen_mutation&) = default;
    const bytes_ostream& representation() const { return _bytes; }
    utils::UUID column_family_id() const;
    utils::UUID schema_version() const; // FIXME: Should replace column_family_id()
    partition_key_view key(const schema& s) const;
    dht::decorated_key decorated_key(const schema& s) const;
    mutation_partition_view partition() const;
    mutation unfreeze(schema_ptr s) const;

    struct printer {
        const frozen_mutation& self;
        schema_ptr schema;
        friend std::ostream& operator<<(std::ostream&, const printer&);
    };

    printer pretty_printer(schema_ptr) const;
};

frozen_mutation freeze(const mutation& m);

// Can receive streamed_mutation in reversed order.
class streamed_mutation_freezer {
    const schema& _schema;
    partition_key _key;
    bool _reversed;

    tombstone _partition_tombstone;
    stdx::optional<static_row> _sr;
    std::deque<clustering_row> _crs;
    range_tombstone_list _rts;
public:
    streamed_mutation_freezer(const schema& s, const partition_key& key, bool reversed = false)
        : _schema(s), _key(key), _reversed(reversed), _rts(s) { }

    stop_iteration consume(tombstone pt);

    stop_iteration consume(static_row&& sr);
    stop_iteration consume(clustering_row&& cr);

    stop_iteration consume(range_tombstone&& rt);

    frozen_mutation consume_end_of_stream();
};

future<frozen_mutation> freeze(streamed_mutation sm);

static constexpr size_t default_frozen_fragment_size = 128 * 1024;

using frozen_mutation_consumer_fn = std::function<future<stop_iteration>(frozen_mutation, bool)>;
future<> fragment_and_freeze(flat_mutation_reader mr, frozen_mutation_consumer_fn c,
                             size_t fragment_size = default_frozen_fragment_size);

