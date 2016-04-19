#include <hpx/hpx_init.hpp>
#include <hpx/hpx.hpp>

#include "io/io.hpp"
#include "stepper/stepper.hpp"

int hpx_main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./main <input.xml>" << std::endl;
        exit(0);
    }

    io::config config = io::read_config_from_file(argv[1]);

    //std::cout << "lalala on " << hpx::get_locality_id();
    stepper::stepper step;
    step.setup(config).wait();


    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::init(argc, argv);
}
