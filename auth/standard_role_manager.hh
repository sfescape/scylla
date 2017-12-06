/*
 * Copyright (C) 2017 ScyllaDB
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

#include "auth/role_manager.hh"

#include <experimental/string_view>
#include <unordered_set>

#include <seastar/core/future.hh>
#include <seastar/core/sstring.hh>

#include "delayed_tasks.hh"
#include "stdx.hh"
#include "seastarx.hh"

namespace cql3 {
class query_processor;
}

namespace service {
class migration_manager;
}

namespace auth {

stdx::string_view standard_role_manager_name() noexcept;

class standard_role_manager final : public role_manager {
    cql3::query_processor& _qp;

    ::service::migration_manager& _migration_manager;

    delayed_tasks<> _delayed{};

public:
    standard_role_manager(cql3::query_processor& qp, ::service::migration_manager& mm)
            : _qp(qp), _migration_manager(mm) {
    }

    virtual stdx::string_view qualified_java_name() const noexcept override;

    virtual future<> start() override;

    virtual future<> stop() override {
        return make_ready_future();
    }

    virtual future<>
    create(const authenticated_user& performer, stdx::string_view role_name, const role_config&) override;

    virtual future<> drop(const authenticated_user& performer, stdx::string_view role_name) override;

    virtual future<>
    alter(const authenticated_user& performer, stdx::string_view role_name, const role_config_update&) override;

    virtual future<>
    grant(const authenticated_user& performer, stdx::string_view grantee_name, stdx::string_view role_name) override;

    virtual future<>
    revoke(const authenticated_user& performer, stdx::string_view revokee_name, stdx::string_view role_name) override;

    virtual future<std::unordered_set<sstring>>
    query_granted(stdx::string_view grantee_name, recursive_role_query) const override;

    virtual future<std::unordered_set<sstring>> query_all() const override;

    virtual future<bool> exists(stdx::string_view role_name) const override;

    virtual future<bool> is_superuser(stdx::string_view role_name) const override;

    virtual future<bool> can_login(stdx::string_view role_name) const override;

private:
    enum class membership_change { add, remove };

    future<> create_metadata_tables_if_missing();

    bool has_existing_roles() const;

    future<> modify_membership(stdx::string_view role_name, stdx::string_view grantee_name, membership_change);
};

}
