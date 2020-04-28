/**
 * @file model.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::model::Model
 */
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

namespace summit::app::grid::model {

/**
 * @brief Global data model, 
 *    provide and integrate the parameters used in the all levels.
 * @details Initialized by command-line options and Summit.Grid feature specification,
 * includes heatmap format writer and output file paths.
 * 
 */
struct Model 
: public Paths
, public FormatDecoder
{
    /**
     * @brief Heatmap format data writer;
     * 
     */
    using HmWriter = HeatmapWriter<Float, GLID>;
    /**
     * @brief Parallel executor
     * 
     */
    using Executor = nucleona::parallel::BasicAsioPool<
        boost::asio::io_service
    >;

    /**
     * @brief Set the paths parameter, directly call summit::app::grid::Paths::set.
     * 
     * @tparam Args Same as call summit::app::grid::Paths::set.
     * @param args Same as call summit::app::grid::Paths::set.
     * @return decltype(auto) Same as call summit::app::grid::Paths::set.
     */
    template<class... Args>
    decltype(auto) set_paths(Args&&... args) {
        return Paths::set(FWD(args)...);
    }
    /**
     * @brief Get integrated heatmap data writer.
     * 
     * @return HmWriter& Integrated heatmap data writer.
     */
    HmWriter& heatmap_writer() {
        if(!heatmap_writer_)  {
            heatmap_writer_.reset(
                new HmWriter(*this, this->enabled_heatmap_fmts())
            );
        }
        return *heatmap_writer_;
    }
    /**
     * @brief Same as heatmap_writer.
     * 
     * @return HmWriter& Same as heatmap_writer.
     */
    HmWriter& heatmap_writer() const {
        return nucleona::remove_const(*this).heatmap_writer();
    }
    
    /**
     * @brief Backgrouond data writer.
     * 
     * @return BackgroundWriter& background data writer.
     */
    BackgroundWriter& background_writer() {
        if(!background_writer_) {
            background_writer_.reset(
                new BackgroundWriter(*this)
            );
        }
        return *background_writer_;
    }
    /**
     * @brief Same as background_writer.
     * 
     * @return BackgroundWriter& Same as background_writer.
     */
    BackgroundWriter& background_writer() const {
        return nucleona::remove_const(*this).background_writer();
    }
    /**
     * @brief Create the parallel executor.
     * 
     * @param thread_num Executor thread number.
     */
    void set_executor(std::size_t thread_num) {
        executor_.reset(
            new Executor(thread_num)
        );
    }
    /**
     * @brief Set the command-line marker append flag.
     * 
     * @param mk_ap The flag represent the user wants to generate marker append image(true) or not(false).
     */
    void set_marker_append(bool mk_ap) {
        marker_append_ = mk_ap;
    }
    /**
     * @brief Set the background subtraction direct effect in gridding result or not.
     * 
     * @param flag The flag represent the user wants to background direct subtracted in result or not.
     */
    void set_no_bgp(bool flag) {
        no_bgp_ = flag;
    }
    /**
     * @brief Set the filter string. See @ref doc/command-line.md for more details.
     * 
     * @param _filter The filter string. See @ref doc/command-line.md for more details.
     */
    void set_filter(const std::string& _filter) {
        filter_ = _filter;
    }
    /**
     * @brief Set the debug level.
     * 
     * @param level A value in [0, 6], bigger shows more informations.
     */
    void set_debug(int level) {
        debug_ = level;
    }
    /**
     * @brief Set the current run is auto-grdding or manual gridding.
     * @details For manual gridding case, 
     *  a set_not_auto_gridding function is provided to call this function and do additional initialization.
     * @param flag The flag represent auto-gridding or manual gridding.
     */
    void set_auto_gridding(bool flag) {
        auto_gridding_ = flag;
    }
    void set_not_auto_gridding(const std::string& in_grid_log_path) {
        std::ifstream fin(in_grid_log_path);
        fin >> in_grid_log_;
        set_paths(
            in_grid_log_.at("output"),
            in_grid_log_.at("input").get<std::string>(),
            in_grid_log_.at("shared_dir").get<std::string>(),
            in_grid_log_.at("secure_dir").get<std::string>()
        );
        set_formats(in_grid_log_.at("output_formats").get<std::vector<std::string>>());
        set_marker_append(in_grid_log_.at("marker_append"));
        set_filter("all");
        set_no_bgp(in_grid_log_.at("no_bgp"));
        set_auto_gridding(false);
        if (in_grid_log_.count("method"))
            set_method(in_grid_log_.at("method"));
        else
            set_method(method_);
    }
    /**
     * @brief Set the signal extraction and grid cell margin method.
     * @details See chipimgproc::Margin for more details.
     * @param method The method name. Can be auto_min_cv, mid_seg, percentil, max, only_stat
     */
    void set_method(const std::string& method) {
        method_ = method;
    }
    /**
     * @brief Get the parallel executor.
     * 
     * @return Executor& The parallel executor.
     */
    Executor& executor() {
        return *executor_;
    }
    /**
     * @brief Same as executor().
     * 
     * @return Executor& Same as executor().
     */
    Executor& executor() const {
        return nucleona::remove_const(*executor_);
    }

    VAR_GET(bool,                marker_append  )
    VAR_GET(bool,                no_bgp         )
    VAR_GET(std::string,         filter         )
    VAR_GET(int,                 debug          )
    VAR_GET(bool,                auto_gridding  )
    VAR_GET(nlohmann::json,      in_grid_log    )
    VAR_GET(std::string,         method         )
private:
    std::unique_ptr<HmWriter>         heatmap_writer_       ;
    std::unique_ptr<BackgroundWriter> background_writer_    ;
    std::unique_ptr<Executor>         executor_             ;
};

}
