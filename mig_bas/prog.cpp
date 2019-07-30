// Copyright (c) 2014-2016 Hartmut Kaiser
// Copyright (c) 2016 Thomas Heller
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_init.hpp>

#include <hpx/hpx_main.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/iostreams.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/serialization.hpp>

#include <chrono>
#include <cstddef>
#include <utility>
#include <vector>

template <typename T>
using migratable_component_base =
    hpx::components::migration_support<hpx::components::component_base<T>>;

///////////////////////////////////////////////////////////////////////////////
class mgcex_srv : public migratable_component_base<mgcex_srv>
{
public:
    using base_type = migratable_component_base<mgcex_srv>;

    mgcex_srv(int data = 0)
      : data_(data)
    {
    }
    mgcex_srv(mgcex_srv const& other)
      : base_type(other)
      , data_(other.data_)
    {
    }
    mgcex_srv& operator=(mgcex_srv const& other)
    {
        data_ = other.data_;
        return *this;
    }
    mgcex_srv(mgcex_srv&& other)
      : base_type(std::move(other))
      , data_(other.data_)
    {
    }
    mgcex_srv& operator=(mgcex_srv&& other)
    {
        data_ = other.data_;
        return *this;
    }
    ~mgcex_srv() = default;

    hpx::id_type call() const
    {
        HPX_ASSERT(pin_count() != 0);
        return hpx::find_here();
    }
    HPX_DEFINE_COMPONENT_ACTION(mgcex_srv, call);

    //void busy_work() const
    //{
    //    HPX_ASSERT(pin_count() != 0);
    //    hpx::this_thread::sleep_for(std::chrono::seconds(1));
    //    HPX_ASSERT(pin_count() != 0);
    //}
    //HPX_DEFINE_COMPONENT_ACTION(mgcex_srv, busy_work);

    //hpx::future<void> lazy_busy_work() const
    //{
    //    HPX_ASSERT(pin_count() != 0);

    //    auto f = hpx::make_ready_future_after(std::chrono::seconds(1));

    //    return f.then(
    //        [this](hpx::future<void>&& f) -> void
    //        {
    //            HPX_ASSERT(pin_count() != 0);
    //            f.get();
    //            HPX_ASSERT(pin_count() != 0);
    //        });
    //}
    //HPX_DEFINE_COMPONENT_ACTION(mgcex_srv, lazy_busy_work);

    //int get_data() const {
    //    HPX_ASSERT(pin_count() != 0);
    //    return data_;
    //}
    //HPX_DEFINE_COMPONENT_ACTION(mgcex_srv, get_data);

    //hpx::future<int> lazy_get_data() const
    //{
    //    HPX_ASSERT(pin_count() != 0);

    //    auto f = hpx::make_ready_future(data_);

    //    return f.then(
    //        [this](hpx::future<int>&& f) -> int
    //        {
    //            HPX_ASSERT(pin_count() != 0);
    //            auto result = f.get();
    //            HPX_ASSERT(pin_count() != 0);
    //            return result;
    //        });
    //}
    //HPX_DEFINE_COMPONENT_ACTION(mgcex_srv, lazy_get_data);

    template <typename Archive>
    void serialize(Archive& ar, unsigned version)
    {
        ar & data_;
    }

private:
    int data_;
};

using bas_mig_server_t = hpx::components::component<mgcex_srv>;
HPX_REGISTER_COMPONENT(bas_mig_server_t);

using call_action = mgcex_srv::call_action;
HPX_REGISTER_ACTION(call_action);

//using busy_work_action = mgcex_srv::busy_work_action;
//HPX_REGISTER_ACTION(busy_work_action);
//
//using lazy_busy_work_action = mgcex_srv::lazy_busy_work_action;
//HPX_REGISTER_ACTION(lazy_busy_work_action);
//
//using get_data_action = mgcex_srv::get_data_action;
//HPX_REGISTER_ACTION(get_data_action);
//
//using lazy_get_data_action = mgcex_srv::lazy_get_data_action;
//HPX_REGISTER_ACTION(lazy_get_data_action);

///////////////////////////////////////////////////////////////////////////////
class mgcex_client
  : public hpx::components::client_base<mgcex_client, mgcex_srv>
{
public:
    using base_type = hpx::components::client_base<mgcex_client, mgcex_srv>;

    mgcex_client() = default;
    mgcex_client(hpx::shared_future<hpx::id_type> const& id)
      : base_type(id)
    {
    }
    mgcex_client(hpx::id_type&& id)
      : base_type(std::move(id))
    {
    }

    hpx::id_type call() const {
        return mgcex_srv::call_action()(this->get_id());
    }

    //hpx::future<void> busy_work() const
    //{
    //    return hpx::async<busy_work_action>(this->get_id());
    //}

    //hpx::future<void> lazy_busy_work() const
    //{
    //    return hpx::async<lazy_busy_work_action>(this->get_id());
    //}

    //int get_data() const
    //{
    //    return get_data_action()(this->get_id());
    //}

    //int lazy_get_data() const
    //{
    //    return lazy_get_data_action()(this->get_id()).get();
    //}
};


///////////////////////////////////////////////////////////////////////////////
void migrate_component(hpx::id_type src, hpx::id_type target)
{
    auto t1 = hpx::new_<mgcex_client>(src, 42);

    HPX_ASSERT(t1.call() == src);
    //HPX_ASSERT(t1.get_data() == 42);

    try
    {
        mgcex_client t2(hpx::components::migrate(t1, target));
    }
    catch (std::exception const& ex)
    {
        hpx::cout << hpx::get_error_what(ex) << hpx::endl;
    }
}

void do_all_work()
{
    std::vector<hpx::id_type> localities = hpx::find_remote_localities();

    HPX_ASSERT(!localities.empty());

    for (auto const& id : localities)
    {
        migrate_component(hpx::find_here(), id);
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(boost::program_options::variables_map& vm)
{
    //vm["nt"].as<std::uint64_t>();   // Number of steps.
    //vm["nx"].as<std::uint64_t>();   // Number of grid points.
    //vm["np"].as<std::uint64_t>();   // Number of partitions.
    //vm["nd"].as<std::uint64_t>();   // Max depth of dep tree.
    //vm.count("no-header")
    //vm.count("results")

    do_all_work();
    //do_all_work(nt, nx, np, nd);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    using namespace boost::program_options;

    options_description desc_commandline;
    //desc_commandline.add_options()
    //    ("results", "print generated results (default: false)")
    //    ("nx", value<std::uint64_t>()->default_value(10),
    //     "Local x dimension (of each partition)")
    //    ("nt", value<std::uint64_t>()->default_value(45),
    //     "Number of time steps")
    //    ("nd", value<std::uint64_t>()->default_value(10),
    //     "Number of time steps to allow the dependency tree to grow to")
    //    ("np", value<std::uint64_t>()->default_value(10),
    //     "Number of partitions")
    //    ("k", value<double>(&k)->default_value(0.5),
    //     "Heat transfer coefficient (default: 0.5)")
    //    ("dt", value<double>(&dt)->default_value(1.0),
    //     "Timestep unit (default: 1.0[s])")
    //    ("dx", value<double>(&dx)->default_value(1.0),
    //     "Local x dimension")
    //    ( "no-header", "do not print out the csv header row")
    //;

    //// Initialize and run HPX, this example requires to run hpx_main on all
    //// localities
    std::vector<std::string> const cfg = {
        //"hpx.run_hpx_main!=1"
    };

    return hpx::init(desc_commandline, argc, argv, cfg);
}
