#include "partition_server.hpp"

#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>

// The macros below are necessary to generate the code required for exposing
// our partition type remotely.
//
// HPX_REGISTER_COMPONENT() exposes the component creation
// through hpx::new_<>().
using partition_server_type = hpx::components::component<partition_server>;
HPX_REGISTER_COMPONENT(partition_server_type, partition_server);

HPX_REGISTER_ACTION(get_data_action);
