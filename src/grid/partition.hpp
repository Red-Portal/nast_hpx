#ifndef NAST_HPX_GRID_PARTITION_HPP_
#define NAST_HPX_GRID_PARTITION_HPP_

#include "server/partition_server.hpp"

namespace nast_hpx { namespace grid {

/// This class is a client for the partition_server component, simplifying
/// access.
struct partition
    : hpx::components::client_base<partition, server::partition_server>
{
    typedef hpx::components::client_base<partition, server::partition_server>
        base_type;

    partition() {}

    partition(hpx::id_type where, io::config const& cfg)
        : base_type(hpx::new_<server::partition_server>(where, cfg))
    {
        if (cfg.verbose)
            std::cout << "registering " << cfg.idy * cfg.num_localities_x + cfg.idx << " " << cfg.idx << " " << cfg.idy <<  std::endl;
        hpx::register_with_basename(server::partition_basename, get_id(),
                                        cfg.idz * cfg.num_localities_x * cfg.num_localities_y + cfg.idy * cfg.num_localities_x + cfg.idx);
    }

    // Create a new component on the locality co-located to the id 'where'. The
    // new instance will be initialized from the given partition_data.
    partition(hpx::id_type where)
      : base_type(hpx::new_<server::partition_server>(hpx::colocated(where)))
    {}

    // Attach a future representing a (possibly remote) partition.
    partition(hpx::future<hpx::id_type> && id)
      : base_type(std::move(id))
    {}

    // Unwrap a future<partition> (a partition already holds a future to the
    // id of the referenced object, thus unwrapping accesses this inner future).
    partition(hpx::future<partition> && c)
      : base_type(std::move(c))
    {}

    partition(hpx::shared_future<hpx::id_type> const& id)
      : base_type(id)
    {}

    partition(partition const& other)
      : base_type(other)
    {}

    hpx::future<void> init()
    {
        return hpx::async<server::partition_server::init_action>(this->get_id());
    }

    void init_sync()
    {
        init().wait();
    }

    hpx::future<triple<double>> do_timestep(double dt)
    {
        typename server::partition_server::do_timestep_action act;
        return hpx::async(act, get_id(), dt);
    }
};

}//namespace grid
}

#endif
