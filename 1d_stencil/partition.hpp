#if !defined(PARTITION_HPP_)
#define PARTITION_HPP_

#include "partition_server.hpp"

///////////////////////////////////////////////////////////////////////////////
// This is a client side helper class allowing to hide some of the tedious
// boilerplate while referencing a remote partition.
struct partition : hpx::components::client_base<partition, partition_server>
{
    using base_type = hpx::components::client_base<partition, partition_server>;

    partition() {}

    // Create new component on locality 'where' and initialize the held data
    partition(hpx::id_type where, std::size_t size, double initial_value)
        : base_type(hpx::new_<partition_server>(where, size, initial_value))
    {}

    // Create a new component on the locality co-located to the id 'where'. The
    // new instance will be initialized from the given partition_data.
    partition(hpx::id_type where, partition_data const& data)
        : base_type(hpx::new_<partition_server>(hpx::colocated(where), data))
    {}

    // Attach a future representing a (possibly remote) partition.
    partition(hpx::future<hpx::id_type>&& id)
        : base_type(std::move(id))
    {}

    // Unwrap a future<partition> (a partition already is a future to the
    // id of the referenced object, thus unwrapping accesses this inner future).
    partition(hpx::future<partition>&& c)
        : base_type(std::move(c))
    {}

    ///////////////////////////////////////////////////////////////////////////
    // Invoke the (remote) member function which gives us access to the data.
    // This is a pure helper function hiding the async.
    hpx::future<partition_data> get_data(partition_server::partition_type t) const
    {
        partition_server::get_data_action act;
        return hpx::async(act, get_id(), t);
    }
};

#endif // PARTITION_HPP_
