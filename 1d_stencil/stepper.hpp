#if !defined(STEPPER_HPP_)
#define STEPPER_HPP_

#include "stepper_server.hpp"

#include <hpx/include/components.hpp>
#include <hpx/include/future.hpp>

///////////////////////////////////////////////////////////////////////////////
// This is a client side member function can now be implemented as the
// stepper_server has been defined.
struct stepper : hpx::components::client_base<stepper, stepper_server>
{
    using base_type = hpx::components::client_base<stepper, stepper_server>;

    // construct new instances/wrap existing steppers from other localities
    stepper(std::size_t num_localities)
      : base_type(hpx::new_<stepper_server>(hpx::find_here(), num_localities))
    {
        hpx::register_with_basename(
            stepper_basename, get_id(), hpx::get_locality_id());
    }

    stepper(hpx::future<hpx::id_type>&& id)
      : base_type(std::move(id))
    {
    }

    ~stepper()
    {
        // break cyclic dependencies
        hpx::future<void> f1 =
            hpx::async(release_dependencies_action(), get_id());

        // release the reference held by AGAS
        hpx::future<void> f2 = hpx::unregister_with_basename(
            stepper_basename, hpx::get_locality_id());

        hpx::wait_all(f1, f2);    // ignore exceptions
    }

    hpx::future<stepper_server::space> do_work(
        std::size_t local_np, std::size_t nx, std::size_t nt, std::uint64_t nd)
    {
        return hpx::async(do_work_action(), get_id(), local_np, nx, nt, nd);
    }
};

#endif    // STEPPER_HPP_
