#pragma once
#include <nlohmann/json.hpp>
#include "paths.hpp"
#include <Nucleona/language.hpp>
#include <type_traits>
#include "heatmap_writer.hpp"
#include "background_writer.hpp"
#include "format_decoder.hpp"
#include <memory>
#include <Nucleona/parallel/asio_pool.hpp>
#include <Nucleona/range.hpp>
#include "model/macro.hpp"
#include "model/task.hpp"

namespace summit::app::grid2 {

struct Model 
: public Paths
, public FormatDecoder
{
    using Float = float;
    using GridLineID = std::uint16_t;
    using HmWriter = HeatmapWriter<Float, GridLineID>;
    using TaskMap = std::map<
        std::string,    // task id
        model::Task     // task
    >; 
    using Executor = nucleona::parallel::BasicAsioPool<
        boost::asio::io_service
    >;

    template<class... Args>
    decltype(auto) set_paths(Args&&... args) {
        return Paths::set(FWD(args)...);
    }

    auto get_tasks() {
        return tasks_ | ranges::view::values;
    }

    model::Task& create_task(const std::string& task_id) {
        auto itr = tasks_.find(task_id);
        if(itr != tasks_.end())
            tasks_.erase(itr);
        auto [task_itr, flag] = tasks_.emplace(task_id, model::Task());
        auto& task = task_itr->second;
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
    BackgroundWriter& background_writer() {
        if(!background_writer_) {
            background_writer_.reset(
                new BackgroundWriter(*this)
            );
        }
        return *background_writer_;
    }
    void set_executor(std::size_t thread_num) {
        executor_.reset(
            new Executor(thread_num)
        );
    }
    Executor& executor() {
        return *executor_;
    }

    VAR_GET(TaskMap, tasks)
private:
    std::unique_ptr<HmWriter>         heatmap_writer_       ;
    std::unique_ptr<BackgroundWriter> background_writer_    ;
    std::unique_ptr<Executor>         executor_             ;
};

}
#undef VAR_GET
#undef VAR_PTR_GET