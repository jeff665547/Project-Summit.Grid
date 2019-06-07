#pragma once
#include <nlohmann/json.hpp>
#include "../paths.hpp"
#include <Nucleona/language.hpp>
#include <type_traits>
#include "../heatmap_writer.hpp"
#include "../background_writer.hpp"
#include "../format_decoder.hpp"
#include <memory>
#include <Nucleona/parallel/asio_pool.hpp>
#include <Nucleona/range.hpp>
#include "macro.hpp"
#include <summit/config/cell_fov.hpp>
#include <summit/config/chip.hpp>
#include <Nucleona/util/remove_const.hpp>
#include "type.hpp"
#include "task.hpp"
#include "task_group.hpp"

namespace summit::app::grid2::model {

struct Model 
: public Paths
, public FormatDecoder
{
    using HmWriter = HeatmapWriter<Float, GLID>;
    using Executor = nucleona::parallel::BasicAsioPool<
        boost::asio::io_service
    >;

    template<class... Args>
    decltype(auto) set_paths(Args&&... args) {
        return Paths::set(FWD(args)...);
    }

    auto get_task_groups() {
        return task_groups_ | ranges::view::values;
    }

    model::Task& create_task(const TaskID& task_id) {
        auto tasks_itr = task_groups_.find(task_id.rfid());
        if(tasks_itr == task_groups_.end()) {
            task_groups_.emplace(task_id.rfid(), model::TaskGroup());
            tasks_itr->second.set_model(*this);
            tasks_itr->second.set_rfid(task_id.rfid());
        }
        auto& tasks = tasks_itr->second;
        auto itr = tasks.find(task_id.string());
        if(itr != tasks.end())
            tasks.erase(itr);
        auto [task_itr, flag] = tasks.emplace(task_id.string(), model::Task());
        auto& task = task_itr->second;
        task.set_task_id(task_id);
        task.set_model(*this);
        return task;
    }

    HmWriter& heatmap_writer() {
        if(!heatmap_writer_)  {
            heatmap_writer_.reset(
                new HmWriter(*this, this->enabled_heatmap_fmts())
            );
        }
        return *heatmap_writer_;
    }
    HmWriter& heatmap_writer() const {
        return nucleona::remove_const(*this).heatmap_writer();
    }
    
    BackgroundWriter& background_writer() {
        if(!background_writer_) {
            background_writer_.reset(
                new BackgroundWriter(*this)
            );
        }
        return *background_writer_;
    }
    BackgroundWriter& background_writer() const {
        return nucleona::remove_const(*this).background_writer();
    }
    void set_executor(std::size_t thread_num) {
        executor_.reset(
            new Executor(thread_num)
        );
    }
    void set_marker_append(bool mk_ap) {
        marker_append_ = mk_ap;
    }
    void set_no_bgp(bool flag) {
        no_bgp_ = flag;
    }
    void set_filter(const std::string& _filter) {
        filter_ = _filter;
    }
    Executor& executor() {
        return *executor_;
    }
    Executor& executor() const {
        return nucleona::remove_const(*executor_);
    }

    VAR_GET(model::TaskGroupMap, task_groups    )
    VAR_GET(bool,                marker_append  )
    VAR_GET(bool,                no_bgp         )
    VAR_GET(std::string,         filter         )
private:
    std::unique_ptr<HmWriter>         heatmap_writer_       ;
    std::unique_ptr<BackgroundWriter> background_writer_    ;
    std::unique_ptr<Executor>         executor_             ;
};

}