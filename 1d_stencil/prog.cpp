// Copyright (c) 2014-2016 Hartmut Kaiser
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This is the eighth in a series of examples demonstrating the development
// of a fully distributed solver for a simple 1D heat distribution problem.
//
// This example builds upon and extends example seven.

#include <hpx/hpx_init.hpp>

#include "options.hpp"
#include "partition.hpp"
#include "partition_data.hpp"
#include "partition_server.hpp"
#include "print_time_results.hpp"
#include "stepper.hpp"
#include "stepper_server.hpp"

#include <hpx/hpx.hpp>
#include <hpx/lcos/gather.hpp>
#include <hpx/runtime/serialization/serialize.hpp>
#include <hpx/util/unused.hpp>

#include <boost/shared_array.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
void do_all_work(std::uint64_t nt, std::uint64_t nx, std::uint64_t np,
    std::uint64_t nd)
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    std::size_t nl = localities.size();    // Number of localities

    if (np < nl)
    {
        std::cout << "The number of partitions should not be smaller than "
                     "the number of localities" << std::endl;
        return;
    }

    // Create the local stepper instance, register it
    stepper step(nl);

    // Measure execution time.
    std::uint64_t t = hpx::util::high_resolution_clock::now();

    // Perform all work and wait for it to finish
    hpx::future<stepper_server::space> result =
        step.do_work(np / nl, nx, nt, nd);

    // Gather results from all localities
    if (0 == hpx::get_locality_id())
    {
        std::uint64_t const num_worker_threads = hpx::get_num_worker_threads();

        hpx::future<std::vector<stepper_server::space>> overall_result =
            hpx::lcos::gather_here(gather_basename, std::move(result), nl);

        std::vector<stepper_server::space> solution = overall_result.get();
        for (std::size_t i = 0; i != nl; ++i)
        {
            stepper_server::space const& s = solution[i];
            for (std::size_t i = 0; i != s.size(); ++i)
            {
                s[i].get_data(partition_server::middle_partition).wait();
            }
        }

        std::uint64_t elapsed = hpx::util::high_resolution_clock::now() - t;

        // Print the solution at time-step 'nt'.
        if (print_results)
        {
            for (std::size_t i = 0; i != nl; ++i)
            {
                stepper_server::space const& s = solution[i];
                for (std::size_t j = 0; j != s.size(); ++j)
                {
                    std::cout << "U[" << i*(s.size()) + j << "] = "
                        << s[j].get_data(partition_server::middle_partition).get()
                        << std::endl;
                }
            }
        }

        print_time_results(std::uint32_t(nl), num_worker_threads, elapsed,
            nx, np, nt, header);
    }
    else
    {
        hpx::lcos::gather_there(gather_basename, std::move(result)).wait();
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(boost::program_options::variables_map& vm)
{
    std::uint64_t nt = vm["nt"].as<std::uint64_t>();   // Number of steps.
    std::uint64_t nx = vm["nx"].as<std::uint64_t>();   // Number of grid points.
    std::uint64_t np = vm["np"].as<std::uint64_t>();   // Number of partitions.
    std::uint64_t nd = vm["nd"].as<std::uint64_t>();   // Max depth of dep tree.

    if (vm.count("no-header"))
        header = false;
    if (vm.count("results"))
        print_results = true;

    do_all_work(nt, nx, np, nd);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    using namespace boost::program_options;

    options_description desc_commandline;
    desc_commandline.add_options()
        ("results", "print generated results (default: false)")
        ("nx", value<std::uint64_t>()->default_value(10),
         "Local x dimension (of each partition)")
        ("nt", value<std::uint64_t>()->default_value(45),
         "Number of time steps")
        ("nd", value<std::uint64_t>()->default_value(10),
         "Number of time steps to allow the dependency tree to grow to")
        ("np", value<std::uint64_t>()->default_value(10),
         "Number of partitions")
        ("k", value<double>(&k)->default_value(0.5),
         "Heat transfer coefficient (default: 0.5)")
        ("dt", value<double>(&dt)->default_value(1.0),
         "Timestep unit (default: 1.0[s])")
        ("dx", value<double>(&dx)->default_value(1.0),
         "Local x dimension")
        ( "no-header", "do not print out the csv header row")
    ;

    // Initialize and run HPX, this example requires to run hpx_main on all
    // localities
    std::vector<std::string> const cfg = {
        "hpx.run_hpx_main!=1"
    };

    return hpx::init(desc_commandline, argc, argv, cfg);
}
