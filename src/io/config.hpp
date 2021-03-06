#ifndef NAST_HPX_IO_CONFIG_HPP_
#define NAST_HPX_IO_CONFIG_HPP_

#include "util/defines.hpp"
#include "grid/boundary_data.hpp"

#include <iostream>
#include <bitset>
#include <vector>

namespace nast_hpx { namespace io {

/// This class represents the configuration of the simulation provided by the
/// csv file passed in as an argument to the executable.
struct config
{
        uint i_max;
        uint j_max;
        uint k_max;
        uint num_fluid_cells;

        double x_length;
        double y_length;
        double z_length;
        double dx;
        double dy;
        double dz;
        double over_dx;
        double over_dy;
        double over_dz;
        double dx_sq;
        double dy_sq;
        double dz_sq;
        double over_dx_sq;
        double over_dy_sq;
        double over_dz_sq;

        double part1;
        double part2;
        double factor_jacobi;

        double re;
        double pr;
        double omega;
        double tau;
        double alpha;
        double beta;
        double gx;
        double gy;
        double gz;

        bool vtk;
        double delta_vec;
        bool verbose;

        double t_end;
        double initial_dt;
        std::size_t max_timesteps;

        uint iter_max;
        double eps;
        double eps_sq;

        std::size_t num_localities;
        std::size_t num_localities_x;
        std::size_t num_localities_y;
        std::size_t num_localities_z;

        std::size_t cells_x_per_partition;
        std::size_t cells_y_per_partition;
        std::size_t cells_z_per_partition;

        std::size_t rank;
        std::size_t idx;
        std::size_t idy;
        std::size_t idz;

        std::size_t threads;

        grid::boundary_condition bnd_condition;

        std::vector<std::bitset<9> > flag_grid;

        bool with_initial_uv_grid;
        std::vector<std::pair<double, double> > initial_uv_grid;

        template <typename Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar & i_max & j_max & k_max & num_fluid_cells & x_length
                & y_length & z_length & dx & dy & dz & over_dx & over_dy & over_dz
                & dx_sq & dy_sq & dz_sq & part1 & part2 & factor_jacobi & re & pr & omega & tau & alpha
                & beta & gx & gy & gz & vtk & delta_vec & verbose & t_end & initial_dt & max_timesteps
                & iter_max & eps & eps_sq & num_localities
                & num_localities_x & num_localities_y & num_localities_z
                & cells_x_per_partition & cells_y_per_partition & cells_z_per_partition
                & rank & idx & idy & idz & threads & with_initial_uv_grid
                & bnd_condition;
        }

        friend std::ostream& operator<<(std::ostream& os, config const& config)
        {
            os  << "config:"
                << "\n{"
                << "\nGEOMETRY:"
                << "\n\ti_max = " << config.i_max
                << "\n\tj_max = " << config.j_max
                << "\n\tk_max = " << config.k_max
                << "\n\tdx = " << config.dx
                << "\n\tdy = " << config.dy
                << "\n\tdz = " << config.dz
                << "\n\tdx_sq = " << config.dx_sq
                << "\n\tdy_sq = " << config.dy_sq
                << "\n\tdz_sq = " << config.dz_sq
                << "\n\tover_dx = " << config.over_dx
                << "\n\tover_dy = " << config.over_dy
                << "\n\tover_dz = " << config.over_dz
                << "\n\tover_dx_sq = " << config.over_dx_sq
                << "\n\tover_dy_sq = " << config.over_dy_sq
                << "\n\tover_dz_sq = " << config.over_dz_sq
                << "\n\tnum_localities = " << config.num_localities
                << "\n\tnum_localities_x = " << config.num_localities_x
                << "\n\tnum_localities_y = " << config.num_localities_y
                << "\n\tnum_localities_z = " << config.num_localities_z
                << "\n\tnum_cells_x_per_partition = " << config.cells_x_per_partition
                << "\n\tnum_cells_y_per_partition = " << config.cells_y_per_partition
                << "\n\tnum_cells_z_per_partition = " << config.cells_z_per_partition
                << "\n\trank = " << config.rank
                << "\n\tidx = " << config.idx
                << "\n\tidy = " << config.idy
                << "\n\tidy = " << config.idz
                << "\n\tnumFluid = " << config.num_fluid_cells
                << "\n\txLength = " << config.x_length
                << "\n\tyLength = " << config.y_length
                << "\n\tyLength = " << config.z_length
                << "\nDATA:"
                << "\n\tRe = " << config.re
                << "\n\tPr = " << config.pr
                << "\n\tomega = " << config.omega
                << "\n\tgx = " << config.gx
                << "\n\tgy = " << config.gy
                << "\n\tgz = " << config.gz
                << "\n\tbeta = " << config.beta
                << "\n\tdt = " << config.initial_dt
                << "\n\tt_end = " << config.t_end
                << "\n\tmax_timesteps = " << config.max_timesteps
                << "\n\tboundary = " << config.bnd_condition
                << "\nSIMULATION:"
                << "\n\ttau = " << config.tau
                << "\n\teps = " << config.eps
                << "\n\teps_sq = " << config.eps_sq
                << "\n\talpha = " << config.alpha
                << "\n\tbeta = " << config.beta
                << "\n\tpart1 = " << config.part1
                << "\n\tpart2 = " << config.part2
                << "\n\tfactor_jacobi = " << config.factor_jacobi
                << "\n\tdelta_vec = " << config.delta_vec
                << "\n\titer_max = " << config.iter_max
                << "\n\tvtk = " << config.vtk
                << "\n}";
            return os;
        }

        static config read_config_from_file(const char *xml_path, const char *grid_path, std::size_t rank, std::size_t num_localities);

};

} //namespace
}

#endif

