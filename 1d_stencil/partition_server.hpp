#if !defined(PARTITION_SERVER_HPP_)
#define PARTITION_SERVER_HPP_

#include "partition_data.hpp"

#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>

///////////////////////////////////////////////////////////////////////////////
// This is the server side representation of the data. We expose this as a HPX
// component which allows for it to be created and accessed remotely through
// a global address (hpx::id_type).
struct partition_server : hpx::components::component_base<partition_server>
{
    enum partition_type
    {
        left_partition,
        middle_partition,
        right_partition
    };

    // construct new instances
    partition_server() {}

    partition_server(partition_data const& data)
      : data_(data)
    {
    }

    partition_server(std::size_t size, double initial_value)
      : data_(size, initial_value)
    {
    }

    // Access data. The parameter specifies what part of the data should be
    // accessed. As long as the result is used locally, no data is copied,
    // however as soon as the result is requested from another locality only
    // the minimally required amount of data will go over the wire.
    partition_data get_data(partition_type t) const;

    // Every member function which has to be invoked remotely needs to be
    // wrapped into a component action. The macro below defines a new type
    // 'get_data_action' which represents the (possibly remote) member function
    // partition::get_data().
    HPX_DEFINE_COMPONENT_DIRECT_ACTION(
        partition_server, get_data, get_data_action);

private:
    partition_data data_;
};

// HPX_REGISTER_ACTION() exposes the component member function for remote
// invocation.
using get_data_action = partition_server::get_data_action;
HPX_REGISTER_ACTION_DECLARATION(get_data_action);

#endif    // PARTITION_SERVER_HPP_
