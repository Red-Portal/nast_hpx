#include "io/config.hpp"
#include "stepper/stepper.hpp"

#include <iostream>
#include <chrono>
#include <hpx/hpx_init.hpp>
#include <hpx/hpx.hpp>

int hpx_main(boost::program_options::variables_map& vm)
{
    const auto cfg_path = vm["cfg"].as<std::string>();
    const auto grid_path = vm["grid"].as<std::string>();
    const auto iterations = vm["iterations"].as<std::size_t>();
    const auto timesteps = vm["timesteps"].as<std::size_t>();

    nast_hpx::io::config cfg = nast_hpx::io::config::read_config_from_file(cfg_path.c_str(), grid_path.c_str(), hpx::get_locality_id(), hpx::get_initial_num_localities());
    cfg.max_timesteps = timesteps;
    cfg.verbose = vm.count("verbose") ? true : false;

    if (cfg.verbose)
        std::cout << "Threads on locality " << hpx::get_locality_id()
            << " = " << hpx::get_os_thread_count() << std::endl;

    cfg.threads = hpx::get_os_thread_count();

    auto rank = hpx::get_locality_id();

    if (rank == 0)
    {
        std::cout
            << "Running simulation on " << cfg.i_max + 2 << "x" << cfg.j_max + 2
            << " cells on " << cfg.num_localities << " nodes ";

        if (timesteps == 0)
            std::cout << "until t_end " << cfg.t_end << std::endl;
        else
            std::cout << "for " << timesteps << " iterations"
                << " and " << iterations << " runs!" << std::endl;
    }

    cfg.idx = (rank % (cfg.num_localities_x * cfg.num_localities_y)) % cfg.num_localities_x;
    cfg.idy = (rank % (cfg.num_localities_x * cfg.num_localities_y)) / cfg.num_localities_x;
    cfg.idz = rank / (cfg.num_localities_x * cfg.num_localities_y);

    cfg.num_localities_x = cfg.num_localities_x;
    cfg.num_localities_y = cfg.num_localities_y;
    cfg.num_localities_z = cfg.num_localities_z;
    cfg.num_localities = cfg.num_localities;

    double avgtime = 0.;
    double maxtime = 0.;
    double mintime = 365. * 24. * 3600.;

   /* std::vector<hpx::performance_counters::performance_counter> idle_rate_counters(num_localities);
    std::vector<std::size_t> avg_idle_rates(num_localities, 0);
    std::vector<std::size_t> max_idle_rates(num_localities, 0);
    std::vector<std::size_t> min_idle_rates(num_localities, 20000);
    std::size_t avgidlerate = 0;
    std::size_t maxidlerate = 0;
    std::size_t minidlerate = 20000;

    for (std::size_t loc = 0; loc < num_localities; ++loc)
        idle_rate_counters[loc] = hpx::performance_counters::performance_counter("/threads{locality#" + std::to_string(loc) + "/total}/idle-rate");
    */
    nast_hpx::stepper::stepper step;
    step.setup(cfg);

    for (std::size_t iter = 0; iter < iterations; ++iter)
    {
        hpx::util::high_resolution_timer t;
        auto start = std::chrono::steady_clock::now(); 
        step.run();
        auto stop = std::chrono::steady_clock::now(); 

        double elapsed = t.elapsed();
        std::cout << "iteration " << iter
                  << " elapsed " << std::chrono::duration_cast<
                      std::chrono::duration<double, std::ratio<1>>>(stop - start).count()
                  << "s" << std::endl;

        if (iter > 0 || iterations == 1)
        {
            avgtime += elapsed;
            maxtime = std::max(maxtime, elapsed);
            mintime = std::min(mintime, elapsed);
        }
    }

    if (rank == 0)
    {
        avgtime = avgtime / static_cast<double>(
                    (std::max)(iterations-1, static_cast<boost::uint64_t>(1)));

        std::cout << "Avg time (s):\t" << avgtime << "\n"
                  << "Min time (s):\t" << mintime << "\n"
                  << "Max time (s):\t" << maxtime << "\n";

        std::cout << std::endl;

    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    using namespace boost::program_options;

    options_description desc_commandline;

    desc_commandline.add_options()
    ("cfg", value<std::string>()->required(),
         "path to config xml file")
    ("grid", value<std::string>()->required(),
         "path to grid file")
    ("iterations", value<std::size_t>()->default_value(1),
         "Number of runs of the simulation")
    ("timesteps", value<std::size_t>()->default_value(0),
         "Number of timesteps per run (0 = use t_end)")
     ( "verbose", "Verbose output");

    std::vector<std::string> cfg;
    cfg.push_back("hpx.run_hpx_main!=1");

    return hpx::init(desc_commandline, argc, argv, cfg);
}
