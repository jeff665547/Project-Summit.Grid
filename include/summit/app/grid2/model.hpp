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

#define VAR_GET(T, sym) \
public: std::add_const_t<T>& sym() const { return sym##_; } \
private: T sym##_;

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
    using Exectuor = nucleona::parallel::BasicAsioPool<
        boost::asio::io_service
    >;

    template<class... Args>
    decltype(auto) set_paths(Args&&... args) {
        return Paths::set(FWD(args)...);
    }

    model::Task& create_task(const std::string& task_id) {
        auto itr = tasks_.find(task_id);
        if(itr != tasks_.end())
            tasks_.erase(itr);
        auto [task_itr, flag] = tasks_.emplace(task_id, model::Task());
        return task_itr->second;
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
            new Exectuor(thread_num)
        );
    }
    Executor& executor() {
        return *exectuor_;
    }

    VAR_GET(TaskMap, tasks)
private:
    std::unique_ptr<HmWriter>         heatmap_writer_       ;
    std::unique_ptr<BackgroundWriter> background_writer_    ;
    std::unique_ptr<Executor>         executor_             ;
};

#undef VAR_GET
}