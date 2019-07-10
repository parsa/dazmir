#include "stepper.hpp"

///////////////////////////////////////////////////////////////////////////////
// The partitioned operator, it invokes the heat operator above on all elements
// of a partition.
partition stepper_server::heat_part(partition const& left,
    partition const& middle, partition const& right)
{
    hpx::shared_future<partition_data> middle_data =
        middle.get_data(partition_server::middle_partition);

    hpx::future<partition_data> next_middle = middle_data.then(
        hpx::util::unwrapping(
            [middle](partition_data const& m) -> partition_data
            {
                HPX_UNUSED(middle);

                // All local operations are performed once the middle data of
                // the previous time step becomes available.
                std::size_t size = m.size();
                partition_data next(size);
                for (std::size_t i = 1; i != size - 1; ++i)
                    next[i] = heat(m[i - 1], m[i], m[i + 1]);
                return next;
            }
        )
    );

    return hpx::dataflow(
        hpx::launch::async,
        hpx::util::unwrapping(
            [left, middle, right](partition_data next, partition_data const& l,
                partition_data const& m, partition_data const& r) -> partition
            {
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
            }
        ),
        std::move(next_middle),
                left.get_data(partition_server::left_partition),
                middle_data,
                right.get_data(partition_server::right_partition)
                );
}