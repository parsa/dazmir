#include "stepper_server.hpp"

#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/naming.hpp>

void stepper_server::send_left(std::size_t t, partition p) const
{
    hpx::apply(from_right_action(), left_.get(), t, std::move(p));
}
void stepper_server::send_right(std::size_t t, partition p) const
{
    hpx::apply(from_left_action(), right_.get(), t, std::move(p));
}

///////////////////////////////////////////////////////////////////////////////
// This is the implementation of the time step loop
//
// Do all the work on 'np' partitions, 'nx' data points each, for 'nt'
// time steps, limit depth of dependency tree to 'nd'.
stepper_server::space stepper_server::do_work(
    std::size_t local_np, std::size_t nx, std::size_t nt, std::uint64_t nd)
{
    // U[t][i] is the state of position i at time t.
    for (space& s : U_)
    {
        s.resize(local_np);
    }

    // Initial conditions: f(0, i) = i
    hpx::id_type here = hpx::naming::get_id_from_locality_id(0);  //hpx::find_here();
    for (std::size_t i = 0; i != local_np; ++i)
    {
        U_[0][i] = partition(here, nx, double(i));
    }

    // send initial values to neighbors
    if (nt != 0)
    {
        send_left(0, U_[0][0]);
        send_right(0, U_[0][local_np - 1]);
    }

    // limit depth of dependency tree
    hpx::lcos::local::sliding_semaphore sem(nd);

    for (std::size_t t = 0; t != nt; ++t)
    {
        if (t == nt / 2)
        {
            U_[0][t] = partition(hpx::components::migrate(U_[0][t], hpx::find_here()));
        }
        space const& current = U_[t % 2];
        space& next = U_[(t + 1) % 2];

        // handle special case (one partition per locality) in a special way
        if (local_np == 1)
        {
            next[0] = hpx::dataflow(
                hpx::launch::async, &stepper_server::heat_part,
                receive_left(t), current[0], receive_right(t)
            );

            // send to left and right if not last time step
            if (t != nt - 1)
            {
                send_left(t + 1, next[0]);
                send_right(t + 1, next[0]);
            }
        }
        else
        {
            next[0] = hpx::dataflow(
                hpx::launch::async, &stepper_server::heat_part,
                receive_left(t), current[0], current[1]
            );

            // send to left if not last time step
            if (t != nt - 1) send_left(t + 1, next[0]);

            for (std::size_t i = 1; i != local_np - 1; ++i)
            {
                next[i] = hpx::dataflow(
                    hpx::launch::async, &stepper_server::heat_part,
                    current[i - 1], current[i], current[i + 1]
                );
            }

            next[local_np - 1] = hpx::dataflow(
                hpx::launch::async, &stepper_server::heat_part,
                current[local_np - 2], current[local_np - 1], receive_right(t)
            );

            // send to right if not last time step
            if (t != nt - 1) send_right(t + 1, next[local_np - 1]);
        }

        // every nd time steps, attach additional continuation which will
        // trigger the semaphore once computation has reached this point
        if ((t % nd) == 0)
        {
            next[0].then(
                [&sem, t](partition&&)
                {
                    // inform semaphore about new lower limit
                    sem.signal(t);
                });
        }

        // suspend if the tree has become too deep, the continuation above
        // will resume this thread once the computation has caught up
        sem.wait(t);
    }

    return U_[nt % 2];
}

///////////////////////////////////////////////////////////////////////////////
// The partitioned operator, it invokes the heat operator above on all elements
// of a partition.
partition stepper_server::heat_part(
    partition const& left, partition const& middle, partition const& right)
{
    hpx::shared_future<partition_data> middle_data =
        middle.get_data(partition_server::middle_partition);

    hpx::future<partition_data> next_middle =
        middle_data.then(hpx::util::unwrapping(
            [middle](partition_data const& m) -> partition_data {
                HPX_UNUSED(middle);

                // All local operations are performed once the middle data of
                // the previous time step becomes available.
                std::size_t size = m.size();
                partition_data next(size);
                for (std::size_t i = 1; i != size - 1; ++i)
                {
                    next[i] = heat(m[i - 1], m[i], m[i + 1]);
                }
                return next;
            }));

    return hpx::dataflow(hpx::launch::async,
        hpx::util::unwrapping(
            [left, middle, right](partition_data next, partition_data const& l,
                partition_data const& m, partition_data const& r) -> partition {
                    HPX_UNUSED(left);
                    HPX_UNUSED(right);

                    // Calculate the missing boundary elements once the
                    // corresponding data has become available.
                    std::size_t size = m.size();
                    next[0] = heat(l[size - 1], m[0], m[1]);
                    next[size - 1] = heat(m[size - 2], m[size - 1], r[0]);

                    // The new partition_data will be allocated on the same locality
                    // as 'middle'.
                    return partition(middle.get_id(), next);
            }),
        std::move(next_middle),
                left.get_data(partition_server::left_partition),
                middle_data,
                right.get_data(partition_server::right_partition));
}

// The macros below are necessary to generate the code required for exposing
// our partition type remotely.
//

// HPX_REGISTER_COMPONENT() exposes the component creation
// through hpx::new_<>().
HPX_REGISTER_COMPONENT(hpx::components::component<stepper_server>, stepper_server);

// HPX_REGISTER_ACTION() exposes the component member function for remote
// invocation.
HPX_REGISTER_ACTION(from_right_action);

HPX_REGISTER_ACTION(from_left_action);

HPX_REGISTER_ACTION(do_work_action);

HPX_REGISTER_ACTION(release_dependencies_action);

HPX_REGISTER_GATHER(stepper_server::space, stepper_server_space_gatherer);
