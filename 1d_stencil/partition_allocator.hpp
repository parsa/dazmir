#if !defined(PARTITION_ALLOCATOR_HPP_)
#define PARTITION_ALLOCATOR_HPP_

#include <hpx/hpx.hpp>

#include <mutex>
#include <stack>

///////////////////////////////////////////////////////////////////////////////
// Use a special allocator for the partition data to remove a major contention
// point - the constant allocation and deallocation of the data arrays.
template <typename T>
struct partition_allocator
{
private:
    typedef hpx::lcos::local::spinlock mutex_type;

public:
    partition_allocator(std::size_t max_size = std::size_t(-1))
      : max_size_(max_size)
    {
    }

    ~partition_allocator()
    {
        std::lock_guard<mutex_type> l(mtx_);
        while (!heap_.empty())
        {
            T* p = heap_.top();
            heap_.pop();
            delete[] p;
        }
    }

    T* allocate(std::size_t n)
    {
        std::lock_guard<mutex_type> l(mtx_);
        if (heap_.empty())
        {
            return new T[n];
        }

        T* next = heap_.top();
        heap_.pop();
        return next;
    }

    void deallocate(T* p)
    {
        std::lock_guard<mutex_type> l(mtx_);
        if (max_size_ == static_cast<std::size_t>(-1) ||
            heap_.size() < max_size_)
            heap_.push(p);
        else
            delete[] p;
    }

private:
    mutex_type mtx_;
    std::size_t max_size_;
    std::stack<T*> heap_;
};

#endif    // PARTITION_ALLOCATOR_HPP_
