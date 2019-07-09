#if !defined(PARTITION_DATA_HPP_)
#define PARTITION_DATA_HPP_

#include "partition_allocator.hpp"

struct partition_data
{
private:
    using buffer_type = hpx::serialization::serialize_buffer<double>;

    struct hold_reference
    {
        hold_reference(buffer_type const& data)
          : data_(data)
        {}

        void operator()(double*) {}     // no deletion necessary

        buffer_type data_;
    };

    static void deallocate(double* p)
    {
        alloc_.deallocate(p);
    }

    static partition_allocator<double> alloc_;

public:
    partition_data()
        : size_(0)
    {}

    // Create a new (uninitialized) partition of the given size.
    partition_data(std::size_t size)
      : data_(alloc_.allocate(size), size, buffer_type::take,
            &partition_data::deallocate)
      , size_(size)
      , min_index_(0)
    {}

    // Create a new (initialized) partition of the given size.
    partition_data(std::size_t size, double initial_value);

    // Create a partition which acts as a proxy to a part of the embedded array.
    // The proxy is assumed to refer to either the left or the right boundary
    // element.
    partition_data(partition_data const& base, std::size_t min_index);

    double& operator[](std::size_t idx);
    double operator[](std::size_t idx) const;

    std::size_t size() const;

private:
    std::size_t index(std::size_t idx) const;

private:
    // Serialization support: even if all of the code below runs on one
    // locality only, we need to provide an (empty) implementation for the
    // serialization as all arguments passed to actions have to support this.
    friend class hpx::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar& data_& size_& min_index_;
    }

private:
    buffer_type data_;
    std::size_t size_;
    std::size_t min_index_;
};

///////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, partition_data const& c);

#endif // PARTITION_DATA_HPP_
