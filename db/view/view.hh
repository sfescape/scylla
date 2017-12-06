/*
 * Copyright (C) 2016 ScyllaDB
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
#include "gc_clock.hh"
#include "query-request.hh"
#include "schema.hh"
#include "streamed_mutation.hh"
#include "stdx.hh"

namespace db {

namespace view {

/**
 * Whether the view filter considers the specified partition key.
 *
 * @param base the base table schema.
 * @param view_info the view info.
 * @param key the partition key that is updated.
 * @return false if we can guarantee that inserting an update for specified key
 * won't affect the view in any way, true otherwise.
 */
bool partition_key_matches(const schema& base, const view_info& view, const dht::decorated_key& key);

/**
 * Whether the view might be affected by the provided update.
 *
 * Note that having this method return true is not an absolute guarantee that the view will be
 * updated, just that it most likely will, but a false return guarantees it won't be affected.
 *
 * @param base the base table schema.
 * @param view_info the view info.
 * @param key the partition key that is updated.
 * @param update the base table update being applied.
 * @return false if we can guarantee that inserting update for key
 * won't affect the view in any way, true otherwise.
 */
bool may_be_affected_by(const schema& base, const view_info& view, const dht::decorated_key& key, const rows_entry& update);

/**
 * Whether a given base row matches the view filter (and thus if the view should have a corresponding entry).
 *
 * Note that this differs from may_be_affected_by in that the provide row must be the current
 * state of the base row, not just some updates to it. This function also has no false positive: a base
 * row either does or doesn't match the view filter.
 *
 * Also note that this function doesn't check the partition key, as it assumes the upper layers
 * have already filtered out the views that are not affected.
 *
 * @param base the base table schema.
 * @param view_info the view info.
 * @param key the partition key that is updated.
 * @param update the current state of a particular base row.
 * @param now the current time in seconds (to decide what is live and what isn't).
 * @return whether the base row matches the view filter.
 */
bool matches_view_filter(const schema& base, const view_info& view, const partition_key& key, const clustering_row& update, gc_clock::time_point now);

bool clustering_prefix_matches(const schema& base, const partition_key& key, const clustering_key_prefix& ck);

future<std::vector<mutation>> generate_view_updates(
        const schema_ptr& base,
        std::vector<view_ptr>&& views_to_update,
        streamed_mutation&& updates,
        streamed_mutation_opt&& existings);

query::clustering_row_ranges calculate_affected_clustering_ranges(
        const schema& base,
        const dht::decorated_key& key,
        const mutation_partition& mp,
        const std::vector<view_ptr>& views);

void mutate_MV(const dht::token& base_token,
        std::vector<mutation> mutations);

}

}
