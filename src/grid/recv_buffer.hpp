#ifndef NAST_HPX_GRID_RECV_BUFFER_HPP_
#define NAST_HPX_GRID_RECV_BUFFER_HPP_

#include "partition_data.hpp"
#include "unpack_buffer.hpp"

#include "util/hpx_wrap.hpp"

namespace nast_hpx { namespace grid {

    template <typename BufferType, direction dir>
    struct recv_buffer
    {
    public:
        typedef hpx::lcos::local::spinlock mutex_type;

        typedef typename BufferType::value_type value_type;

        typedef BufferType buffer_type;

        recv_buffer()
          : valid_(false)
        {}

        recv_buffer(recv_buffer&& other)
          : buffer_(std::move(other.buffer_))
          , valid_(other.valid_)
        {
        }

        recv_buffer& operator=(recv_buffer&& other)
        {
            if(this != &other)
            {
                buffer_ = std::move(other.buffer_);
                valid_ = other.valid_;
            }
            return *this;
        }

        ~recv_buffer()
        {
        }

        void operator()(partition_data<value_type>& p, std::size_t step)
        {
            HPX_ASSERT(valid_);

            buffer_type buffer = buffer_.receive(step).get();

            unpack_buffer<dir>::call(p, buffer);
        }

        void set_buffer(buffer_type buffer, std::size_t step)
        {
            HPX_ASSERT(valid_);
            buffer_.store_received(step, std::move(buffer));
        }

        hpx::lcos::local::receive_buffer<buffer_type, mutex_type> buffer_;
        bool valid_;

    private:
        friend class hpx::serialization::access;

        template<typename Archive>
        void serialize(Archive& ar, unsigned version)
        {
            ar & valid_;
        }
    };

}
}

#endif
