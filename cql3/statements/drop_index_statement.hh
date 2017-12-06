/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Copyright (C) 2017 ScyllaDB
 *
 * Modified by ScyllaDB
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

#include "cql3/statements/schema_altering_statement.hh"
#include "cql3/index_name.hh"

#include <seastar/core/distributed.hh>
#include <seastar/core/shared_ptr.hh>

#include <memory>

namespace cql3 {

namespace statements {

class drop_index_statement : public schema_altering_statement {
    sstring _index_name;
    bool _if_exists;
public:
    drop_index_statement(::shared_ptr<index_name> index_name, bool if_exists);

    virtual const sstring& column_family() const override;

    virtual future<> check_access(const service::client_state& state) override;

    virtual void validate(distributed<service::storage_proxy>&, const service::client_state& state) override;

    virtual future<shared_ptr<cql_transport::event::schema_change>> announce_migration(distributed<service::storage_proxy>& proxy, bool is_local_only) override;

    virtual std::unique_ptr<prepared> prepare(database& db, cql_stats& stats) override;
private:
    schema_ptr lookup_indexed_table() const;
};

}

}