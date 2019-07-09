#include "partition_data.hpp"

partition_allocator<double> partition_data::alloc_;

partition_data::partition_data(std::size_t size, double initial_value)
    : data_(alloc_.allocate(size), size, buffer_type::take,
        &partition_data::deallocate),
    size_(size),
    min_index_(0)
{
    double base_value = double(initial_value * size);
    for (std::size_t i = 0; i != size; ++i)
        data_[i] = base_value + double(i);
}

partition_data::partition_data(partition_data const& base, std::size_t min_index)
    : data_(base.data_.data() + min_index, 1, buffer_type::reference,
        hold_reference(base.data_)),      // keep referenced partition alive
    size_(base.size()),
    min_index_(min_index)
{
    HPX_ASSERT(min_index < base.size());
}

double& partition_data::operator[](std::size_t idx) { return data_[index(idx)]; }

double partition_data::operator[](std::size_t idx) const { return data_[index(idx)]; }

std::size_t partition_data::size() const { return size_; }

std::size_t partition_data::index(std::size_t idx) const
{
    HPX_ASSERT(idx >= min_index_ && idx < size_);
    return idx - min_index_;
}

std::ostream& operator<<(std::ostream& os, partition_data const& c)
{
    os << "{";
    for (std::size_t i = 0; i != c.size(); ++i)
    {
        if (i != 0)
            os << ", ";
        os << c[i];
    }
    os << "}";
    return os;
}
