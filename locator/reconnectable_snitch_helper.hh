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
 *
 * Modified by ScyllaDB
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

#include "gms/i_endpoint_state_change_subscriber.hh"
#include "message/messaging_service.hh"
#include "locator/snitch_base.hh"

namespace locator {

// @note all callbacks should be called in seastar::async() context
class reconnectable_snitch_helper : public  gms::i_endpoint_state_change_subscriber {
private:
    static logging::logger& logger() {
        static logging::logger _logger("reconnectable_snitch_helper");
        return _logger;
    }

    sstring _local_dc;

private:

    void reconnect(gms::inet_address public_address, const gms::versioned_value& local_address_value) {
        reconnect(public_address, gms::inet_address(local_address_value.value));
    }

    void reconnect(gms::inet_address public_address, gms::inet_address local_address) {
        auto& ms = netw::get_local_messaging_service();
        auto& sn_ptr = locator::i_endpoint_snitch::get_local_snitch_ptr();

        if (sn_ptr->get_datacenter(public_address) == _local_dc &&
            ms.get_preferred_ip(public_address) != local_address) {
            //
            // First, store the local address in the system_table...
            //
            db::system_keyspace::update_preferred_ip(public_address, local_address).get();

            //
            // ...then update messaging_service cache and reset the currently
            // open connections to this endpoint on all shards...
            //
            netw::get_messaging_service().invoke_on_all([public_address, local_address] (auto& local_ms) {
                local_ms.cache_preferred_ip(public_address, local_address);

                netw::msg_addr id = {
                    .addr = public_address
                };
                local_ms.remove_rpc_client(id);
            }).get();

            logger().debug("Initiated reconnect to an Internal IP {} for the {}", local_address, public_address);
        }
    }

public:
    reconnectable_snitch_helper(sstring local_dc)
            : _local_dc(local_dc) {}

    void before_change(gms::inet_address endpoint, gms::endpoint_state cs, gms::application_state new_state_key, const gms::versioned_value& new_value) override {
        // do nothing.
    }

    void on_join(gms::inet_address endpoint, gms::endpoint_state ep_state) override {
        auto* internal_ip_state = ep_state.get_application_state_ptr(gms::application_state::INTERNAL_IP);
        if (internal_ip_state) {
            reconnect(endpoint, *internal_ip_state);
        }
    }

    void on_change(gms::inet_address endpoint, gms::application_state state, const gms::versioned_value& value) override {
        if (state == gms::application_state::INTERNAL_IP) {
            reconnect(endpoint, value);
        }
    }

    void on_alive(gms::inet_address endpoint, gms::endpoint_state ep_state) override {
        on_join(std::move(endpoint), std::move(ep_state));
    }

    void on_dead(gms::inet_address endpoint, gms::endpoint_state ep_state) override {
        // do nothing.
    }

    void on_remove(gms::inet_address endpoint) override {
        // do nothing.
    }

    void on_restart(gms::inet_address endpoint, gms::endpoint_state state) override {
        // do nothing.
    }
};
} // namespace locator
