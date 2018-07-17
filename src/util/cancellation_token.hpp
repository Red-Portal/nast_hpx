#ifndef NAST_HPX_UTIL_CANCELLATION_TOKEN_HPP_
#define NAST_HPX_UTIL_CANCELLATION_TOKEN_HPP_

#include <atomic>
#include <memory>

namespace nast_hpx { namespace util {
    
struct cancellation_token
{
private:
    typedef std::atomic<bool> flag_type;
    std::shared_ptr<flag_type> was_cancelled_;

public:
    cancellation_token()
      : was_cancelled_(std::make_shared<flag_type>(false))
    {}

    bool was_cancelled() const noexcept
    {
        return was_cancelled_->load(std::memory_order_relaxed);
    }

    void cancel() noexcept
    {
        was_cancelled_->store(true, std::memory_order_relaxed);
    }
    
    void reset() noexcept
    {
        was_cancelled_->store(false, std::memory_order_relaxed);
    }
};

}
}

#endif
