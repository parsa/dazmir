#include "partition_server.hpp"

#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>

partition_data partition_server::get_data(partition_type t) const
{
    switch (t)
    {
    case left_partition:
        return partition_data(data_, data_.size() - 1);

    case middle_partition:
        break;

    case right_partition:
        return partition_data(data_, 0);

    default:
        HPX_ASSERT(false);
        break;
    }
    return data_;
}

// The macros below are necessary to generate the code required for exposing
// our partition type remotely.
//
// HPX_REGISTER_COMPONENT() exposes the component creation
// through hpx::new_<>().
using partition_server_type = hpx::components::component<partition_server>;
HPX_REGISTER_COMPONENT(partition_server_type, partition_server);

HPX_REGISTER_ACTION(get_data_action);
