/**
 * @file paths.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::Paths
 */
#pragma once
#include <boost/filesystem.hpp>
#include <iostream>
#include <sstream>
#include "output_format.hpp"
#include <summit/grid/logger.hpp>
#include <fmt/format.h>
#include "task_id.hpp"
#include "is_chip_dir.hpp"
namespace summit::app::grid {

/**
 * @brief The output file path specification
 * @details The output path specification includes 2 modes in-place and normal, see
 *    @ref output-in-place-mode "in-place mode specification" and 
 *    @ref output-normal-mode "normal mode specification" for details
 * 
 */
struct Paths {
    /**
     * @brief Mode tags
     * 
     */
    enum Mode {
        inplace, normal
    };
    /**
     * @brief Initialize the output paths, normalize the parameter to the full path.
     * 
     * @param output Output path specified by command-line options
     * @param input Input path specified by command-line options 
     * @param shared_dir Shared directory specified by command-line options
     * @param secure_dir Secure directory specified by command-line options
     */
    void set(
        const std::string& output, 
        const boost::filesystem::path& input,
        const boost::filesystem::path& shared_dir,
        const boost::filesystem::path& secure_dir,
        bool  force_inplace
    ) 
    {
        shared_dir_ = shared_dir;
        secure_dir_ = secure_dir;
        boost::filesystem::path abs_output(output);
        boost::filesystem::path abs_input(input);
        abs_output = boost::filesystem::absolute(abs_output);
        abs_input = boost::filesystem::absolute(abs_input);
        summit::grid::log.trace("shared_dir: {}", shared_dir.string());
        summit::grid::log.trace("secure_dir: {}", secure_dir.string());
        if( shared_dir != "" && secure_dir != "" ) {
            enable_secure_output_ = true;
            boost::filesystem::path abs_shared_dir;
            boost::filesystem::path abs_secure_dir;
            abs_shared_dir = boost::filesystem::absolute(shared_dir);
            abs_secure_dir = boost::filesystem::absolute(secure_dir);
            secure_output_ = abs_secure_dir 
                / boost::filesystem::relative(abs_input, abs_shared_dir);
        } else {
            enable_secure_output_ = false;
        } 
        summit::grid::log.trace("abs_output: {}", abs_output.string());
        summit::grid::log.trace("secure_output_: {}", secure_output_.string());
        if( abs_output.make_preferred() == abs_input.make_preferred() 
            || abs_output == secure_output_.make_preferred()) {
            summit::grid::log.info("use inplace mode paths");
            mode_ = inplace;
        } else {
            if(force_inplace) {
                mode_ = inplace;
            } else {
                summit::grid::log.info("use normal mode paths");
                mode_ = normal;
            }
        }
        output_ = abs_output;
        input_  = abs_input;
        is_group_input_ = !is_chip_dir(input_);
    }
    
    boost::filesystem::path task_output(const TaskID& task_id) const {
        if(mode_ == inplace) {
            if(is_group_input_) {
                return task_id.path();
            } else {
                return output_;
            }
        } else {
            return output_;
        }
    }

    /**
     * @brief Get shared directory path
     * 
     * @return auto Shared directory path
     */
    auto shared_dir_path() const {
        return shared_dir_;
    }
    /**
     * @brief Get secure directory path
     * 
     * @return auto Secure directory path
     */
    auto secure_dir_path() const {
        return secure_dir_;
    }
    /**
     * @brief Get check the secure output is enabled or not
     * 
     * @return true Enabled
     * @return false Not enable
     */
    bool secure_output_enabled() const {
        return enable_secure_output_;
    }
    /**
     * @brief Get the chip log path in secure path
     * 
     * @return auto The chip log path in secure path
     */
    auto sc_chip_log() const {
        summit::grid::log.trace("enable_secure_output_: {}", enable_secure_output_);
        if(!enable_secure_output_) throw std::runtime_error("secure output not enable");
        auto chip_log = secure_output_ / "chip_log.json";
        return check_path(chip_log);
    }
    /**
     * @brief Get the chip log path in grid directory
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     * @return auto The chip log path in grid directory
     */
    auto grid_chip_log(const TaskID& task_id) const {
        return grid_dir(task_id) / "chip_log.json";
    }
    /**
     * @brief Get the marker append image path
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     * @param ch Channeld name
     * @return boost::filesystem::path The marker append image path
     */
    boost::filesystem::path marker_append_path(
        const TaskID& task_id,
        const std::string& ch
    ) const {
        boost::filesystem::path odir = task_output(task_id);
        std::string file_name;
        if(ch.empty()) {
            file_name = fmt::format("{}.tiff", task_id);
        } else {
            file_name = fmt::format("{}-{}.tiff", task_id, ch);
        }
        auto res = check_path(odir / "marker_append" / file_name);
        return res;
    }
    /**
     * @brief Get the input path
     * 
     * @return boost::filesystem::path The input path
     */
    boost::filesystem::path input() const {
        return input_;
    }
    /**
     * @brief Get the output path
     * 
     * @return boost::filesystem::path The output path
     */
    boost::filesystem::path output() const {
        return output_;
    }
    /**
     * @brief Get the raw image path in secure directory
     * 
     * @return boost::filesystem::path The raw image path in secure directory
     */
    boost::filesystem::path sc_raw_img_dir() const {
        if(!enable_secure_output_) throw std::runtime_error("secure output not enable");
        check_path(secure_output_ / "tmp");
        return secure_output_;
    }
    /**
     * @brief Get the array.cen path
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     * @return boost::filesystem::path The array.cen path
     */
    boost::filesystem::path array_cen(
        const TaskID& task_id
    ) const {
        return grid_dir(task_id) / "array.cen";
    }
    /**
     * @brief Get the output heatmap format data path(exclude array.cen)
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     * @param ch Channel name
     * @param ofm Output format label
     * @return boost::filesystem::path The output heatmap format data path
     */
    boost::filesystem::path heatmap(
        const TaskID& task_id,
        const std::string& ch,
        const OutputFormat::Labels& ofm
    ) const {
        if(ofm == OutputFormat::cen_file) {
            return array_cen(task_id);
        }
        auto postfix = OutputFormat::to_file_postfix(ofm);
        switch(mode_) {
            case normal:
                return check_path((output_ / (ch + postfix)).string());
            case inplace:
                return check_path(
                    task_id.path() / "grid" / "channels" / ch / ( "heatmap" + postfix )
                );
            default:
                throw std::runtime_error("unsupport mode");
        }
    }
    /**
     * @brief Get the FOV image path
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     * @param tag Viewable directory postfix
     * @param r FOV row position
     * @param c FOV column position
     * @param ch Channel name
     * @return boost::filesystem::path The FOV image path
     */
    boost::filesystem::path fov_image(
        const TaskID&       task_id,
        const std::string&  tag,
        int r, int c, const std::string& ch
    ) const {
        boost::filesystem::path odir = task_output(task_id);
        return check_path(general_prefix(task_id, ch) 
            / fov_image_postfix(tag, r, c, ch));
    }
    /**
     * @brief Get the stitched image path
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     * @param tag Viewable directory postfix
     * @param ch Channel name
     * @return boost::filesystem::path The stitched image path
     */
    boost::filesystem::path stitch_image(
        const TaskID& task_id,
        const std::string& tag,
        const std::string& ch
    ) const {
        return check_path(general_prefix(task_id, ch) / stitch_image_postfix(tag, ch));
    }
    /**
     * @brief Get the gridline.csv file path
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     * @param channel Channel name
     * @return boost::filesystem::path The gridline.csv file path
     */
    boost::filesystem::path gridline(
        const TaskID& task_id,
        const std::string& channel
    ) const {
        return check_path(general_prefix(task_id, channel) / "gridline.csv");
    }
    /**
     * @brief Get the complete file path. 
     *      This file is not in specification, because it is only used by Bamboo-lake and no informations in this file
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     * @param path The prefix of the file
     * @return boost::filesystem::path The complete file path
     */
    boost::filesystem::path complete_file(
        const TaskID& task_id, 
        const std::string& path
    ) const {
        boost::filesystem::path path_p(path);
        switch(mode_) {
            case normal:
                path_p = path_p / task_id.string()
                    / "COMPLETE";
                break;
            case inplace:
                path_p = path_p / "grid" 
                    / "COMPLETE";
                break;
            default:
                throw std::runtime_error("unsupport mode");
        }
        return check_path(path_p);
    }
    /**
     * @brief Create the complete file.
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     */
    void create_complete_file(
        const TaskID& task_id
    ) const {
        for(auto&& p : {
            // input COMPLETE
            task_id.path() / "grid" / "COMPLETE",
            // output COMPLETE
            task_output(task_id) / "grid" / "COMPLETE"
        }){
            std::ofstream fout(check_path(p).string(), std::fstream::trunc | std::fstream::out);
            fout.close();
        }
    }
    /**
     * @brief Get the background file path
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     * @param channel Channel name
     * @return boost::filesystem::path The background file path
     */
    boost::filesystem::path background(
        const TaskID& task_id,
        const std::string& channel
    ) const {
        boost::filesystem::path odir = task_output(task_id);
        switch(mode_) {
            case normal:
                return check_path(odir /(channel + "_background.csv"));
            case inplace:
                return check_path(
                    general_prefix(task_id, channel) / "background.csv"
                );
            default:
                throw std::runtime_error("unsupport mode");
        }
    }
    /**
     * @brief Get the debug path
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     * @param channel Channel name
     * @return boost::filesystem::path The debug path
     */
    boost::filesystem::path debug(
        const TaskID& task_id, 
        const std::string& channel
    ) const {
        auto dir = check_path(general_prefix(task_id, channel) / "debug");
        return dir;
    }
    /**
     * @brief Get debug image path
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     * @param channel Channel name
     * @param fov_r The FOV position in row
     * @param fov_c The FOV position in column
     * @param tag A string add to postfix of file name
     * @return boost::filesystem::path Debug image path
     */
    boost::filesystem::path debug_img(
        const TaskID& task_id, 
        const std::string& channel,
        int fov_r, int fov_c,
        const std::string& tag
    ) const {
        auto debug_dir = debug(task_id, channel);
        std::stringstream ss;
        ss << fov_r << '-' << fov_c;
        if(!tag.empty()) {
            ss << '-' << tag;
        }
        ss << ".tiff";
        return check_path(debug_dir / ss.str());
    }
    /**
     * @brief Get debug stitched image
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     * @param tag A string add to postfix of file name
     * @return boost::filesystem::path Debug stitched image path
     */
    boost::filesystem::path debug_stitch(
        const TaskID& task_id, 
        const std::string& tag
    ) const {
        return check_path(grid_dir(task_id) / fmt::format("stitch{}.png"
            , tag.empty() ? "" : "-" + tag
        ));
    }
    /**
     * @deprecated Channel level grid log is not generated anymore.
     * @brief Get channel level grid log path
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     * @param channel Channel name
     * @return boost::filesystem::path Channel leve grid log path
     */
    boost::filesystem::path channel_grid_log(
        const TaskID& task_id, 
        const std::string& channel
    ) const {
        return check_path(general_prefix(task_id, channel) / "grid_log.json");
    }
    /**
     * @brief Get grid log path
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     * @return boost::filesystem::path Chip level grid log path
     */
    boost::filesystem::path task_grid_log(
        const TaskID& task_id
    ) const {
        switch(mode_) {
            case normal:
                return check_path(output_ / (task_id.string()  + "-grid_log.json"));
            case inplace:
                return check_path(task_output(task_id) / "grid" / "grid_log.json");
            default:
                throw std::runtime_error("unsupport mode");
        }
    }
private:
    /**
     * @brief Check the parent path of input path is exist. Create if not exist.
     * 
     * @param p Check target
     * @return boost::filesystem::path The same path with p, but normalized.
     */
    boost::filesystem::path check_path( const boost::filesystem::path& p ) const {
        auto tmp = p;
        tmp.make_preferred();
        auto _dir = tmp.parent_path();
        summit::grid::log.debug("check directory: {}", _dir.string());
        if(!boost::filesystem::exists(_dir)) {
            boost::filesystem::create_directories(_dir);
        }
        return tmp;
    }
    /**
     * @brief Generate FOV image postfix.
     * 
     * @param tag Viewable directory postfix.
     * @param r FOV row position
     * @param c FOV column position
     * @param ch Channel name
     * @return std::string The FOV image postfix
     */
    std::string fov_image_postfix(
        const std::string& tag,
        int r, int c, const std::string& ch
    ) const {
        boost::filesystem::path viewable_tag("viewable_" + tag);
        return ( viewable_tag /
            (std::to_string(r) + "-" +
            std::to_string(c) + ".tiff") ).string();
    }
    /**
     * @brief Generate stitched image postfix
     * 
     * @param tag Viewable directory postfix.
     * @param ch Channel name
     * @return std::string The stitched image postfix
     */
    std::string stitch_image_postfix(
        const std::string& tag,
        const std::string& ch
    ) const {
        boost::filesystem::path viewable_tag("viewable_" + tag);
        std::string ext = (tag == "raw") ? ".tiff" : ".png";
        return ( viewable_tag / ("stitch-" + ch + ext )).string();
    }
    /**
     * @brief Get the general prefix path of most paths.
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     * @param channel Channel name
     * @return boost::filesystem::path The general prefix path of most paths.
     */
    boost::filesystem::path general_prefix(
        const TaskID& task_id,
        const std::string& channel
    ) const {
        return grid_dir(task_id) / "channels" / channel;
    }
    /**
     * @brief Get the grid directory
     * 
     * @param task_id The task ID generate by summit::app::grid::TaskID
     * @return boost::filesystem::path The general grid directory.
     */
    boost::filesystem::path grid_dir(
        const TaskID& task_id
    ) const {
        boost::filesystem::path output_p(task_output(task_id));
        switch(mode_) {
            case normal:
                output_p = output_p / task_id.string();
                break;
            case inplace:
                output_p = output_p / "grid";
                break;
            default:
                throw std::runtime_error("unsupport mode");
        }
        return check_path(output_p);
    }
    /**
     * @brief Output path specification mode.
     * 
     */
    Mode mode_;
    /**
     * @brief Is secure output enabled.
     * 
     */
    bool enable_secure_output_;
    /**
     * @brief The secure output path
     * 
     */
    boost::filesystem::path secure_output_;
    /**
     * @brief The output path
     * 
     */
    boost::filesystem::path output_;
    /**
     * @brief The input path
     * 
     */
    boost::filesystem::path input_;
    /**
     * @brief The shared directory
     * 
     */
    boost::filesystem::path shared_dir_;
    /**
     * @brief The secure directory
     * 
     */
    boost::filesystem::path secure_dir_;

    bool is_group_input_;
};

}