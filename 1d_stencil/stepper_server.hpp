#if !defined(STEPPER_SERVER_HPP_)
#define STEPPER_SERVER_HPP_

#include "partition.hpp"

#include <cstddef>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Command-line variables
bool header = true; // print csv heading
bool print_results = false;
double k = 0.5;     // heat transfer coefficient
double dt = 1.;     // time step
double dx = 1.;     // grid spacing

char const* stepper_basename = "/1d_stencil_8/stepper/";
char const* gather_basename = "/1d_stencil_8/gather/";

///////////////////////////////////////////////////////////////////////////////
// Data for one time step on one locality
struct stepper_server : hpx::components::component_base<stepper_server>
{
    // Our data for one time step
    using space = std::vector<partition>;

    stepper_server() {}

    stepper_server(std::size_t nl)
        : left_(hpx::find_from_basename(
            stepper_basename, idx(hpx::get_locality_id(), -1, nl))),
        right_(hpx::find_from_basename(
            stepper_basename, idx(hpx::get_locality_id(), +1, nl))),
        U_(2)
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
    space do_work(std::size_t local_np, std::size_t nx, std::size_t nt,
        std::uint64_t nd);

    HPX_DEFINE_COMPONENT_ACTION(stepper_server, do_work, do_work_action);

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

    HPX_DEFINE_COMPONENT_ACTION(stepper_server, from_right, from_right_action);
    HPX_DEFINE_COMPONENT_ACTION(stepper_server, from_left, from_left_action);

    // release dependencies
    void release_dependencies()
    {
        left_ = hpx::shared_future<hpx::id_type>();
        right_ = hpx::shared_future<hpx::id_type>();
    }

    HPX_DEFINE_COMPONENT_ACTION(stepper_server, release_dependencies,
        release_dependencies_action);

protected:
    // Our operator
    static double heat(double left, double middle, double right)
    {
        return middle + (k * dt / (dx * dx)) * (left - 2 * middle + right);
    }

    // The partitioned operator, it invokes the heat operator above on all
    // elements of a partition.
    static partition heat_part(partition const& left, partition const& middle,
        partition const& right);

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
// HPX_REGISTER_COMPONENT() exposes the component creation
// through hpx::new_<>().
using stepper_server_type = hpx::components::component<stepper_server>;
HPX_REGISTER_COMPONENT(stepper_server_type, stepper_server);

// HPX_REGISTER_ACTION() exposes the component member function for remote
// invocation.
using from_right_action = stepper_server::from_right_action;
HPX_REGISTER_ACTION(from_right_action);

using from_left_action = stepper_server::from_left_action;
HPX_REGISTER_ACTION(from_left_action);

using do_work_action = stepper_server::do_work_action;
HPX_REGISTER_ACTION(do_work_action);

using release_dependencies_action = stepper_server::release_dependencies_action;
HPX_REGISTER_ACTION(release_dependencies_action);

void stepper_server::send_left(std::size_t t, partition p) const
{
    hpx::apply(from_right_action(), left_.get(), t, std::move(p));
}
void stepper_server::send_right(std::size_t t, partition p) const
{
    hpx::apply(from_left_action(), right_.get(), t, std::move(p));
}

#endif // STEPPER_SERVER_HPP_