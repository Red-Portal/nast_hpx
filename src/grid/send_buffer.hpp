#ifndef NAST_HPX_GRID_SEND_BUFFER_HPP_
#define NAST_HPX_GRID_SEND_BUFFER_HPP_

#include "partition_data.hpp"
#include "pack_buffer.hpp"

#include "util/hpx_wrap.hpp"

namespace nast_hpx { namespace grid {

    template <typename BufferType, direction dir, typename Action>
    struct send_buffer
    {
    public:
        typedef hpx::lcos::local::spinlock mutex_type;

        typedef typename BufferType::value_type value_type;

        typedef BufferType buffer_type;

        send_buffer()
          : dest_(hpx::invalid_id)
        {}

        send_buffer(send_buffer&& other)
          : dest_(std::move(other.dest_))
        {
        }
        send_buffer& operator=(send_buffer&& other)
        {
            if(this != &other)
            {
                dest_ = std::move(other.dest_);
            }
            return *this;
        }

        void operator()(partition_data<value_type> const& p, std::size_t step, std::size_t var)
        {
            HPX_ASSERT(dest_);

            buffer_type buffer;

            pack_buffer<dir>::call(p, buffer);

            hpx::apply(Action(), dest_, buffer, step, var);
        }

        friend class hpx::serialization::access;

        template<typename Archive>
        void serialize(Archive& ar, unsigned version)
        {
            ar & dest_;
        }

        hpx::id_type dest_;
    };

}
}

#endif
