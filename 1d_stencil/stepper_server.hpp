#if !defined(STEPPER_SERVER_HPP_)
#define STEPPER_SERVER_HPP_

#include "defs.hpp"
#include "options.hpp"
#include "partition.hpp"

#include <hpx/include/actions.hpp>

#include <cstddef>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Data for one time step on one locality
struct stepper_server : hpx::components::component_base<stepper_server>
{
    // Our data for one time step
    using space = std::vector<partition>;

    stepper_server() {}

    stepper_server(std::size_t nl)
      : left_(hpx::find_from_basename(
            stepper_basename, idx(hpx::get_locality_id(), -1, nl)))
      , right_(hpx::find_from_basename(
            stepper_basename, idx(hpx::get_locality_id(), +1, nl)))
      , U_(2)
    {}

    static inline std::size_t idx(std::size_t i, int dir, std::size_t size)
    {
        if (i == 0 && dir == -1)
            return size - 1;
        if (i == size - 1 && dir == +1)
            return 0;

        HPX_ASSERT((i + dir) < size);

        return i + dir;
    }

    // Do all the work on 'np' partitions, 'nx' data points each, for 'nt'
    // time steps, limit depth of dependency tree to 'nd'.
    space do_work(
        std::size_t local_np, std::size_t nx, std::size_t nt, std::uint64_t nd);

    HPX_DEFINE_COMPONENT_ACTION(stepper_server, do_work);

    // receive the left-most partition from the right
    void from_right(std::size_t t, partition p)
    {
        right_receive_buffer_.store_received(t, std::move(p));
    }

    // receive the right-most partition from the left
    void from_left(std::size_t t, partition p)
    {
        left_receive_buffer_.store_received(t, std::move(p));
    }

    HPX_DEFINE_COMPONENT_ACTION(stepper_server, from_right);
    HPX_DEFINE_COMPONENT_ACTION(stepper_server, from_left);

    // release dependencies
    void release_dependencies()
    {
        left_ = hpx::shared_future<hpx::id_type>();
        right_ = hpx::shared_future<hpx::id_type>();
    }

    HPX_DEFINE_COMPONENT_ACTION(stepper_server, release_dependencies);

protected:
    // Our operator
    static double heat(double left, double middle, double right)
    {
        return middle + (k * dt / (dx * dx)) * (left - 2 * middle + right);
    }

    // The partitioned operator, it invokes the heat operator above on all
    // elements of a partition.
    static partition heat_part(
        partition const& left, partition const& middle, partition const& right);

    // Helper functions to receive the left and right boundary elements from
    // the neighbors.
    partition receive_left(std::size_t t)
    {
        return left_receive_buffer_.receive(t);
    }
    partition receive_right(std::size_t t)
    {
        return right_receive_buffer_.receive(t);
    }

    // Helper functions to send our left and right boundary elements to
    // the neighbors.
    inline void send_left(std::size_t t, partition p) const;
    inline void send_right(std::size_t t, partition p) const;

private:
    hpx::shared_future<hpx::id_type> left_, right_;
    std::vector<space> U_;
    hpx::lcos::local::receive_buffer<partition> left_receive_buffer_;
    hpx::lcos::local::receive_buffer<partition> right_receive_buffer_;
};

// The macros below are necessary to generate the code required for exposing
// our partition type remotely.
//

// HPX_REGISTER_ACTION() exposes the component member function for remote
// invocation.
using from_right_action = stepper_server::from_right_action;
HPX_REGISTER_ACTION_DECLARATION(from_right_action);

using from_left_action = stepper_server::from_left_action;
HPX_REGISTER_ACTION_DECLARATION(from_left_action);

using do_work_action = stepper_server::do_work_action;
HPX_REGISTER_ACTION_DECLARATION(do_work_action);

using release_dependencies_action = stepper_server::release_dependencies_action;
HPX_REGISTER_ACTION_DECLARATION(release_dependencies_action);

HPX_REGISTER_GATHER_DECLARATION(stepper_server::space, stepper_server_space_gatherer);

#endif    // STEPPER_SERVER_HPP_
