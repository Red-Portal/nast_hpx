#ifndef STEPPER_STEPPER_HPP
#define STEPPER_STEPPER_HPP

#include "server/stepper_server.hpp"

namespace stepper {

struct stepper
    : hpx::components::client_base<stepper, server::stepper_server>
{
    typedef hpx::components::client_base<stepper, server::stepper_server> base_type;

    stepper()
      : base_type(hpx::new_<server::stepper_server>
          (hpx::find_here()))
    {}

    stepper(uint num_local_partitions_x, uint num_local_partitions_y, uint num_cells_x, uint num_cells_y, RealType dx, RealType dy)
      : base_type(hpx::new_<server::stepper_server>
          (hpx::find_here(), num_local_partitions_x, num_local_partitions_y, num_cells_x, num_cells_y, dx, dy))
    {}

    // construct new instances/wrap existing steppers from other localities
    stepper(hpx::id_type loc)
      : base_type(hpx::new_<server::stepper_server>
          (loc))
    {
       // hpx::register_with_basename(
        //    stepper_basename, get_id(), hpx::get_locality_id());
    }

    stepper(hpx::future<hpx::id_type> && id)
      : base_type(std::move(id))
    {}

    hpx::future<uint> do_work()
    {
        server::stepper_server::do_work_action act;
        return hpx::async(act, get_id());
    }
};


}//namespace stepper

#endif