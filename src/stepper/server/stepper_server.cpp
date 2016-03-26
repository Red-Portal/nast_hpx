#include <hpx/include/iostreams.hpp>
#include <hpx/lcos/gather.hpp>
#include <hpx/lcos/broadcast.hpp>

#include <cmath>

#include "stepper_server.hpp"

#include "computation/with_for_each.cpp"

#include "io/vtk_writer.hpp"

typedef stepper::server::stepper_server stepper_component;
typedef hpx::components::component<stepper_component> stepper_server_type;

HPX_REGISTER_COMPONENT_MODULE();

HPX_REGISTER_COMPONENT(stepper_server_type, stepper_component);
HPX_REGISTER_ACTION(stepper::server::stepper_server::setup_action, stepper_server_setup_action);

//HPX_REGISTER_GATHER(RealType, stepper_server_space_gatherer);

namespace stepper { namespace server {

stepper_server::stepper_server(uint nl)
    : num_localities(nl)
{}

void stepper_server::setup(io::config cfg)
{
    c = cfg;

            std::cout << "forcing to 1x1 partitions" << std::endl;
        c.i_res = c.i_max + 2;
        c.j_res = c.j_max + 2;

    initialize_parameters();
    initialize_grids();
    initialize_communication();

    if (hpx::get_locality_id() == 0)
        std::cout << cfg << std::endl;

    hpx::cout << "stepper on " << hpx::get_locality_id() << " with " <<  params.num_partitions_x-2 << "x"<< params.num_partitions_y-2 << " partitions, "
             << params.num_cells_per_partition_x << "x" <<  params.num_cells_per_partition_y << " cells each, " << "dx=" << params.dx << " dy=" << params.dx
             << hpx::endl << hpx::flush;

    if (c.output_skip_size != 0)
        write_vtk(0);

    strategy = new computation::with_for_each();

    if (hpx::get_locality_id() == 0)
        do_work();
}

void stepper_server::initialize_parameters()
{
    if (num_localities == 2)
    {
        num_localities_x = 2;
        num_localities_y = 1;
    }
    else
    {
        num_localities_x = static_cast<uint>(sqrt(num_localities));
        num_localities_y = num_localities_x;
    }

    params.num_cells_per_partition_x = c.i_res;
    params.num_cells_per_partition_y = c.j_res;

    params.num_partitions_x = ((c.i_max + 2) / num_localities_x) / c.i_res + 2;
    params.num_partitions_y = ((c.j_max + 2) / num_localities_y) / c.j_res + 2;

    params.re = c.re;
    params.alpha = c.alpha;
    params.omega = c.omega;
    params.dx = c.x_length / c.i_max;
    params.dy = c.y_length / c.j_max;

    params.i_max = c.i_max;
    params.j_max = c.j_max;
}

void stepper_server::initialize_grids()
{
    index_grid.resize(params.num_partitions_x * params.num_partitions_y);
    uv_grid.resize(params.num_partitions_x * params.num_partitions_y);
    fg_grid.resize(params.num_partitions_x * params.num_partitions_y);
    p_grid.resize(params.num_partitions_x * params.num_partitions_y);
    rhs_grid.resize(params.num_partitions_x * params.num_partitions_y);

    for (uint l = 0; l < params.num_partitions_y; l++)
        for (uint k = 0; k < params.num_partitions_x; k++)
        {
            index_grid[get_index(k, l)] =
                std::pair<RealType, RealType>
                    (
                    (hpx::get_locality_id() % num_localities_x) * (params.num_partitions_x - 2) * params.num_cells_per_partition_x + (k - 1) * params.num_cells_per_partition_x,
                    (hpx::get_locality_id() / num_localities_x) * (params.num_partitions_y - 2) * params.num_cells_per_partition_y + (l - 1) * params.num_cells_per_partition_y
                    );

            uv_grid[get_index(k, l)] = vector_partition(hpx::find_here(), params.num_cells_per_partition_x, params.num_cells_per_partition_y);
            fg_grid[get_index(k, l)] = vector_partition(hpx::find_here(), params.num_cells_per_partition_x, params.num_cells_per_partition_y);
            p_grid[get_index(k, l)] = scalar_partition(hpx::find_here(), params.num_cells_per_partition_x, params.num_cells_per_partition_y);
            rhs_grid[get_index(k, l)] = scalar_partition(hpx::find_here(), params.num_cells_per_partition_x, params.num_cells_per_partition_y);
        }

    scalar_dummy = scalar_partition(hpx::find_here(), 1, 1);
    vector_dummy = vector_partition(hpx::find_here(), 1, 1);
}

void stepper_server::initialize_communication()
{
    has_neighbor[LEFT] = !(hpx::get_locality_id() % num_localities_x == 0);
    has_neighbor[RIGHT] = !(hpx::get_locality_id() % num_localities_x == num_localities_x - 1);
    has_neighbor[BOTTOM] = !(hpx::get_locality_id() / num_localities_x == 0);
    has_neighbor[TOP] = !(hpx::get_locality_id() / num_localities_x == num_localities_y - 1);
    has_neighbor[BOTTOM_LEFT] = (has_neighbor[BOTTOM] && has_neighbor[LEFT]);
    has_neighbor[BOTTOM_RIGHT] = (has_neighbor[BOTTOM] && has_neighbor[RIGHT]);
    has_neighbor[TOP_LEFT] = (has_neighbor[TOP] && has_neighbor[LEFT]);
    has_neighbor[TOP_RIGHT] = (has_neighbor[TOP] && has_neighbor[RIGHT]);

    for (int directionInt = LEFT; directionInt != NUM_DIRECTIONS; directionInt++)
    {
        neighbor_steppers_[directionInt] = hpx::find_from_basename(stepper_basename,
                                                                   get_neighbor_id(hpx::get_locality_id(), static_cast<direction>(directionInt), num_localities));
    }

    for (uint loc = 0; loc < num_localities; loc++)
                localities.push_back(hpx::find_from_basename(stepper_basename, loc).get());
}

void stepper_server::do_work()
{
    std::pair<RealType, RealType> max_uv(2, 0);
    RealType t = 0;
    RealType dt = c.dt;

    for(uint step = 1; t + dt < c.t_end; step++)
    {
        if (c.output_skip_size != 0 && (step  % c.output_skip_size == 0))
        {
            if (hpx::get_locality_id() == 0)
                std::cout << "t " << t << " | dt " << dt;
        }

        hpx::future<std::vector<std::pair<RealType, RealType> > > local_max_uvs = hpx::lcos::broadcast<do_timestep_action> (localities, step, dt);

        local_max_uvs.get();

        t += dt;

        dt = c.tau * std::min(c.re/2. * 1./(1./(params.dx * params.dx) + 1./(params.dy * params.dy)),
                                        std::min(params.dx/max_uv.first, params.dy/max_uv.second));
    }
}

std::pair<RealType, RealType> stepper_server::do_timestep(uint step, RealType dt)
{
    hpx::util::high_resolution_timer t;

    uint k = 1;
    uint l = 1;

    uint global_i = index_grid[get_index(k, l)].first;
    uint global_j = index_grid[get_index(k, l)].second;

    vector_data uv_center = uv_grid[get_index(k, l)].get_data(CENTER).get();

    strategy->set_velocity_on_boundary(uv_center, global_i, global_j, params.i_max, params.j_max);

    vector_data fg_center = fg_grid[get_index(k, l)].get_data(CENTER).get();

    vector_data uv_left = uv_grid[get_index(k-1, l)].get_data(LEFT).get();
    vector_data uv_right = uv_grid[get_index(k+1, l)].get_data(RIGHT).get();
    vector_data uv_bottom = uv_grid[get_index(k, l-1)].get_data(BOTTOM).get();
    vector_data uv_top = uv_grid[get_index(k, l+1)].get_data(TOP).get();
    vector_data uv_bottomright = uv_grid[get_index(k+1, l-1)].get_data(BOTTOM_RIGHT).get();
    vector_data uv_topleft = uv_grid[get_index(k-1, l+1)].get_data(TOP_LEFT).get();

    strategy->compute_fg(fg_center, uv_center, uv_left, uv_right, uv_bottom, uv_top, uv_bottomright, uv_topleft,
                            global_i, global_j, params.i_max, params.j_max, params.re,
                            params.dx, params.dy, dt, params.alpha);

    fg_grid[get_index(k, l)] = vector_partition(hpx::find_here(), fg_center);

    scalar_data rhs_center = rhs_grid[get_index(k, l)].get_data(CENTER).get();
    vector_data fg_left = fg_grid[get_index(k-1, l)].get_data(LEFT).get();
    vector_data fg_bottom = fg_grid[get_index(k, l-1)].get_data(BOTTOM).get();

    strategy->compute_rhs(rhs_center, fg_center, fg_left, fg_bottom, global_i, global_j, params.i_max,
                            params.j_max, params.dx, params.dy, dt);

    rhs_grid[get_index(k, l)] = scalar_partition(hpx::find_here(), rhs_center);

    scalar_data p_center = p_grid[get_index(k, l)].get_data(CENTER).get();
    scalar_data p_left = p_grid[get_index(k-1, l)].get_data(LEFT).get();
    scalar_data p_right = p_grid[get_index(k+1, l)].get_data(RIGHT).get();
    scalar_data p_bottom = p_grid[get_index(k, l-1)].get_data(BOTTOM).get();
    scalar_data p_top = p_grid[get_index(k, l+1)].get_data(TOP).get();

    hpx::util::high_resolution_timer t2;

    uint iter = 0;
    RealType res = 0;
    do
    {
        strategy->set_pressure_on_boundary(p_center, global_i, global_j, params.i_max, params.j_max);

        strategy->sor_cycle(p_center, p_left, p_right, p_bottom, p_top, rhs_center, global_i, global_j,
                                params.i_max, params.j_max, params.omega, params.dx, params.dy);

        res = strategy->compute_residual(p_center, p_left, p_right, p_bottom, p_top,
                                                    rhs_center, global_i, global_j, params.i_max,
                                                    params.j_max, params.dx, params.dy);

        iter++;
    } while(iter < c.iter_max && res > c.eps_sq);

    std::cout << "sor " << t2.elapsed();

    p_grid[get_index(k, l)] = scalar_partition(hpx::find_here(), p_center);

    strategy->update_velocities(uv_center, p_center, p_right, p_top, fg_center, global_i, global_j,
                                    params.i_max, params.j_max, params.dx, params.dy, dt);

    uv_grid[get_index(k, l)] = vector_partition(hpx::find_here(), uv_center);

    if (c.output_skip_size != 0 && ((step + 1) % c.output_skip_size == 0))
    {
        write_vtk(step / c.output_skip_size);

        if (hpx::get_locality_id() == 0)
            std::cout << "iterations: " << iter << " | residual " << res << std::endl;
    }

    std::cout << " | total " << t.elapsed() << std::endl;

    return std::pair<RealType, RealType>(2, 0);
}

void stepper_server::do_sor_cycle()
{

}

void stepper_server::sor()
{

}

// ---------------------------------------- COMMUNICATION ---------------------------------------- //
void stepper_server::set_keep_running(uint iter, bool kr)
{
    keep_running.store_received(iter, std::move(kr));
}

void stepper_server::communicate_p_grid(uint iter)
{
    //SEND
    for (uint l = 1; l < params.num_partitions_y - 1; l++)
    {
        send_p_to_neighbor(iter * params.num_partitions_y + l, p_grid[get_index(1, l)], LEFT);
        send_p_to_neighbor(iter * params.num_partitions_y + l, p_grid[get_index(params.num_partitions_x - 2, l)], RIGHT);
    }

    for (uint k = 1; k < params.num_partitions_x - 1; k++)
    {
        send_p_to_neighbor(iter * params.num_partitions_x + k, p_grid[get_index(k, 1)], BOTTOM);
        send_p_to_neighbor(iter * params.num_partitions_x + k, p_grid[get_index(k, params.num_partitions_y - 2)], TOP);
    }

    //RECEIVE
    for(uint l = 1; l < params.num_partitions_y - 1; l++)
    {
        p_grid[get_index(0, l)] = receive_p_from_neighbor(iter * params.num_partitions_y+l, LEFT);
        p_grid[get_index(params.num_partitions_x - 1, l)] = receive_p_from_neighbor(iter * params.num_partitions_y + l, RIGHT);
    }

    for(uint k = 1; k < params.num_partitions_x - 1; k++)
    {
        p_grid[get_index(k, 0)] = receive_p_from_neighbor(iter * params.num_partitions_x + k, BOTTOM);
        p_grid[get_index(k, params.num_partitions_y - 1)] = receive_p_from_neighbor(iter * params.num_partitions_x + k, TOP);
    }
}

void stepper_server::receive_p_action_(uint t, scalar_partition p, direction to_dir)
{
    //direction is now opposite.
    p_recv_buffs_[NUM_DIRECTIONS - to_dir - 1].store_received(t, std::move(p));
}

void stepper_server::send_p_to_neighbor(uint t, scalar_partition p, direction dir)
{
    if (has_neighbor[dir])
    {
        receive_p_action act;
        hpx::async(act, neighbor_steppers_[dir].get(), t, p, dir);
    }
}

scalar_partition stepper_server::receive_p_from_neighbor(uint t, direction dir)
{
    if (has_neighbor[dir])
        return p_recv_buffs_[dir].receive(t);
    else
        return scalar_dummy;
}

void stepper_server::communicate_fg_grid(uint step)
{
    //SEND
     for (uint l = 1; l < params.num_partitions_y - 1; l++)
        send_fg_to_neighbor(step * params.num_partitions_y + l, fg_grid[get_index(params.num_partitions_x - 2, l)], RIGHT);

    for (uint k = 1; k < params.num_partitions_x - 1; k++)
        send_fg_to_neighbor(step * params.num_partitions_x + k, fg_grid[get_index(k, params.num_partitions_y - 2)], TOP);

    //RECEIVE
    for(uint l = 1; l < params.num_partitions_y - 1; l++)
        fg_grid[get_index(0, l)] = receive_fg_from_neighbor(step * params.num_partitions_y + l, LEFT);

    for(uint k = 1; k < params.num_partitions_x-1; k++)
        fg_grid[get_index(k, 0)] = receive_fg_from_neighbor(step * params.num_partitions_x + k, BOTTOM);
}

void stepper_server::receive_fg_action_(uint t, vector_partition fg, direction to_dir)
{
    //direction is now opposite.
    fg_recv_buffs_[NUM_DIRECTIONS - to_dir - 1].store_received(t, std::move(fg));
}

void stepper_server::send_fg_to_neighbor(uint t, vector_partition fg, direction dir)
{
    if (has_neighbor[dir])
    {
        receive_fg_action act;
        hpx::async(act, neighbor_steppers_[dir].get(), t, fg, dir);
    }
}

vector_partition stepper_server::receive_fg_from_neighbor(uint t, direction dir)
{
    if (has_neighbor[dir])
        return fg_recv_buffs_[dir].receive(t);
    else
        return vector_dummy;
}

uint stepper_server::get_index(uint k, uint l) const
{
    return l * params.num_partitions_x + k;
}

void stepper_server::communicate_uv_grid(uint step)
{
    // SEND
     for (uint l = 1; l < params.num_partitions_y - 1; l++)
    {
        if (l == 1)
            send_uv_to_neighbor(step, uv_grid[get_index(params.num_partitions_x - 2, l)], BOTTOM_RIGHT);

        if (l == params.num_partitions_y - 2)
            send_uv_to_neighbor(step, uv_grid[get_index(1, l)], TOP_LEFT);

        send_uv_to_neighbor(step * params.num_partitions_y + l, uv_grid[get_index(1, l)], LEFT);
        send_uv_to_neighbor(step * params.num_partitions_y + l, uv_grid[get_index(params.num_partitions_x - 2, l)], RIGHT);
    }

    for (uint k = 1; k < params.num_partitions_x - 1; k++)
    {
        if (k == 1)
                send_uv_to_neighbor(step, uv_grid[get_index(k, 1)], BOTTOM_LEFT);

        if (k == params.num_partitions_x- 2)
                send_uv_to_neighbor(step, uv_grid[get_index(k, params.num_partitions_y - 2)], TOP_RIGHT);

        send_uv_to_neighbor(step * params.num_partitions_x + k, uv_grid[get_index(k, 1)], BOTTOM);
        send_uv_to_neighbor(step * params.num_partitions_x + k, uv_grid[get_index(k, params.num_partitions_y - 2)], TOP);
    }

    // RECEIVE
    for(uint l = 1; l < params.num_partitions_y - 1; l++)
    {
        uv_grid[get_index(0, l)] = receive_uv_from_neighbor(step * params.num_partitions_y + l, LEFT);
        uv_grid[get_index(params.num_partitions_x - 1, l)] = receive_uv_from_neighbor(step * params.num_partitions_y + l, RIGHT);
    }

    for(uint k = 1; k < params.num_partitions_x - 1; k++)
    {
        uv_grid[get_index(k, 0)] = receive_uv_from_neighbor(step * params.num_partitions_x + k, BOTTOM);
        uv_grid[get_index(k, params.num_partitions_y - 1)] = receive_uv_from_neighbor(step * params.num_partitions_x + k, TOP);
    }

    uv_grid[get_index(0, params.num_partitions_y - 1)] = receive_uv_from_neighbor(step, TOP_LEFT);
    uv_grid[get_index(params.num_partitions_x - 1, params.num_partitions_y -1)] = receive_uv_from_neighbor(step, TOP_RIGHT);
    uv_grid[get_index(0, 0)] = receive_uv_from_neighbor(step, BOTTOM_LEFT);
    uv_grid[get_index(params.num_partitions_x - 1, 0)] = receive_uv_from_neighbor(step, BOTTOM_RIGHT);
}

void stepper_server::receive_uv_action_(uint t, vector_partition uv, direction to_dir)
{
    //direction is now opposite.
    uv_recv_buffs_[NUM_DIRECTIONS - to_dir - 1].store_received(t, std::move(uv));
}

void stepper_server::send_uv_to_neighbor(uint t, vector_partition uv, direction dir)
{
    if (has_neighbor[dir])
    {
        receive_uv_action act;
        hpx::async(act, neighbor_steppers_[dir].get(), t, uv, dir);
    }
}

vector_partition stepper_server::receive_uv_from_neighbor(uint t, direction dir)
{
    if (has_neighbor[dir])
        return uv_recv_buffs_[dir].receive(t);
    else
        return vector_dummy;
}

template<typename T>
void stepper_server::print_grid(std::vector<grid::partition<T> > const& grid, const std::string message) const
{
    std::vector<std::vector<grid::partition_data<T> > > data;

    data.resize(params.num_partitions_x - 2);

    for (uint k = 1; k < params.num_partitions_x - 1; k++)
    {
        data[k-1].resize(params.num_partitions_y - 2);
        for (uint l = 1; l < params.num_partitions_y - 1; l++)
        {
            grid::partition_data<T> base = grid[get_index(k, l)].get_data(CENTER).get();
            data[k-1][l-1] = grid::partition_data<T>(base);
        }
    }

    boost::shared_ptr<hpx::lcos::local::promise<int> > p = boost::make_shared<hpx::lcos::local::promise<int> >();
    io::do_async_print(data, message, params.num_partitions_x - 2, params.num_partitions_y - 2, params.num_cells_per_partition_x, params.num_cells_per_partition_y, p);
}

void stepper_server::write_vtk(uint step) const
{
    std::vector<std::vector<scalar_data> > p_data;

    p_data.resize(params.num_partitions_x - 2);

    for (uint k = 1; k < params.num_partitions_x - 1; k++)
    {
        p_data[k-1].resize(params.num_partitions_y - 2);
        for (uint l = 1; l < params.num_partitions_y - 1; l++)
        {
            scalar_data base = p_grid[get_index(k, l)].get_data(CENTER).get();
            p_data[k-1][l-1] = scalar_data(base);
        }
    }

    std::vector<std::vector<vector_data> > uv_data;

    uv_data.resize(params.num_partitions_x - 2);

    for (uint k = 1; k < params.num_partitions_x - 1; k++)
    {
        uv_data[k-1].resize(params.num_partitions_y - 2);
        for (uint l = 1; l < params.num_partitions_y - 1; l++)
        {
            vector_data base = uv_grid[get_index(k, l)].get_data(CENTER).get();
            uv_data[k-1][l-1] = vector_data(base);
        }
    }

    boost::shared_ptr<hpx::lcos::local::promise<int> > p = boost::make_shared<hpx::lcos::local::promise<int> >();
    io::do_async_write(p_data, uv_data, params.dx, params.dy, step, params.i_max, params.j_max, params.num_partitions_x - 2, params.num_partitions_y - 2, params.num_cells_per_partition_x, params.num_cells_per_partition_y, p);

}

RealType stepper_server::compute_new_dt(std::pair<RealType, RealType> max_uv) const
{
    return c.tau * std::min(c.re/2. * 1./(1./(params.dx * params.dx) + 1./(params.dy * params.dy)), std::min(params.dx/max_uv.first, params.dy/max_uv.second));
}

}//namespace server
}//namespace stepper
