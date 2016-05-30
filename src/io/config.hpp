#ifndef NAST_HPX_IO_CONFIG_HPP
#define NAST_HPX_IO_CONFIG_HPP

#include <iostream>
#include <bitset>
#include <vector>

#include "util/typedefs.hpp"
#include "util/boundary_data.hpp"

namespace nast_hpx { namespace io {

/// This class represents the configuration of the simulation provided by the
/// csv file passed in as an argument to the executable.
struct config
{
        uint i_max;
        uint j_max;
        uint i_res;
        uint j_res;
        uint num_fluid_cells;
        
        Real x_length;
        Real y_length;

        Real re;
        Real pr;
        Real omega;
        Real tau;
        Real eps;
        Real eps_sq;
        Real alpha;
        Real beta;
        Real gx;
        Real gy;
        Real dx;
        Real dy;

        bool vtk;
        uint output_skip_size;
        Real delta_vec;

        Real t_end;
        Real dt;
        uint sub_iterations;
        uint iter_max;

        uint wfe;

        std::size_t num_localities;
        std::size_t num_localities_x;
        std::size_t num_localities_y;
        
        std::size_t num_local_partitions;
        std::size_t num_local_partitions_x;
        std::size_t num_local_partitions_y;
        std::size_t cells_x_per_partition;
        std::size_t cells_y_per_partition;
        
        std::size_t rank;

        boundary_data u_bnd;
        boundary_data v_bnd;
        boundary_data data_type;

        boundary_data temp_bnd;
        boundary_data temp_data_type;
        Real ti;

        bool with_flag_grid;
        std::vector<std::bitset<6> > flag_grid;

        bool with_initial_uv_grid;
        std::vector<std::pair<Real, Real> > initial_uv_grid;
        
        template <typename Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar & i_max & j_max & i_res & j_res & num_fluid_cells & x_length
                & y_length & re & pr & omega & tau & eps & eps_sq & alpha
                & beta & t_end & dt & delta_vec & vtk & output_skip_size
                & sub_iterations & iter_max & wfe & gx & gy & num_localities
                & num_localities_x & num_localities_y & num_local_partitions
                & num_local_partitions_x & num_local_partitions_y & cells_x_per_partition 
                & cells_y_per_partition & rank & dx & dy;
            //& u_bnd & v_bnd & data_type
            //& temp_bnd & temp_data_type & ti & with_flag_grid & flag_grid
            //& with_initial_uv_grid & initial_uv_grid;
        }

        friend std::ostream& operator<<(std::ostream& os, config const& config)
        {
            os  << "config:"
                << "\n{"
                << "\nGEOMETRY:"
                << "\n\tiMax = " << config.i_max
                << "\n\tjMax = " << config.j_max
                << "\n\tdx = " << config.dx
                << "\n\tdy = " << config.dy
                << "\n\tnum_localities = " << config.num_localities
                << "\n\tnum_localities_x = " << config.num_localities_x
                << "\n\tnum_localities_y = " << config.num_localities_y                
                << "\n\tnum_local_partitions = " << config.num_local_partitions
                << "\n\tnum_local_partitions_x = " << config.num_local_partitions_x
                << "\n\tnum_local_partitions_y = " << config.num_local_partitions_y
                << "\n\tnum_cells_x_per_partition = " << config.cells_x_per_partition
                << "\n\tnum_cells_y_per_partition = " << config.cells_y_per_partition
                << "\n\trank = " << config.rank
                << "\n\tnumFluid = " << config.num_fluid_cells
                << "\n\txLength = " << config.x_length
                << "\n\tyLength = " << config.y_length
                << "\nDATA:"
                << "\n\tRe = " << config.re
                << "\n\tPr = " << config.pr
                << "\n\tomega = " << config.omega
                << "\n\tGX = " << config.gx
                << "\n\tGY = " << config.gy
                << "\n\tbeta = " << config.beta
                << "\n\tdt = " << config.dt
                << "\n\tu_bnd " << config.u_bnd
                << "\n\tv_bnd " << config.v_bnd
                << "\n\tbnd_data_type " << config.data_type
                << "\n\ttemp_bnd " << config.temp_bnd
                << "\n\ttemp_data_type " << config.temp_data_type
                << "\n\tTI = " << config.ti
                << "\nSIMULATION:"
                << "\n\ttau = " << config.tau
                << "\n\teps = " << config.eps
                << "\n\talpha = " << config.alpha
                << "\n\toutputSkipSize = " << config.output_skip_size
                << "\n\tdeltaVec = " << config.delta_vec
                << "\n\titerMax = " << config.iter_max
                << "\n\tsubIterations = " << config.sub_iterations
                << "\n\twfe = " << config.wfe
                << "\n\tvtk = " << config.vtk
                << "\n}";
            return os;
        }
        
        static config read_config_from_file(const char *path, std::size_t rank, std::size_t num_localities);

};

} //namespace
}

#endif

