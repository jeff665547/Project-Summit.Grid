#pragma once
#include <boost/filesystem.hpp>
#include <iostream>
#include <sstream>
#include "output_format.hpp"
#include <summit/grid/logger.hpp>
namespace summit::app::grid {

struct Paths {
    enum Mode {
        inplace, normal
    };
    void set(
        const std::string& output, 
        const boost::filesystem::path& input,
        const boost::filesystem::path& shared_dir,
        const boost::filesystem::path& secure_dir
    ) 
    {
        shared_dir_ = shared_dir;
        secure_dir_ = secure_dir;
        boost::filesystem::path abs_output(output);
        boost::filesystem::path abs_input(input);
        abs_output = boost::filesystem::absolute(abs_output);
        abs_input = boost::filesystem::absolute(abs_input);
        if( shared_dir != "" && secure_dir != "" ) {
            enable_secure_output_ = true;
            boost::filesystem::path abs_shared_dir;
            boost::filesystem::path abs_secure_dir;
            abs_shared_dir = boost::filesystem::absolute(shared_dir);
            abs_secure_dir = boost::filesystem::absolute(secure_dir);
            secure_output_ = abs_secure_dir 
                / boost::filesystem::relative(abs_input, abs_shared_dir);
        } 
        summit::grid::log.trace("abs_output: {}", abs_output.string());
        summit::grid::log.trace("secure_output_: {}", secure_output_.string());
        if( abs_output.make_preferred() == abs_input.make_preferred() 
            || abs_output == secure_output_.make_preferred()) {
            summit::grid::log.info("use inplace mode paths");
            mode_ = inplace;
        } else {
            summit::grid::log.info("use normal mode paths");
            mode_ = normal;
        }
        output_ = abs_output;
        input_  = abs_input;
    }
    auto shared_dir_path() const {
        return shared_dir_;
    }
    auto secure_dir_path() const {
        return secure_dir_;
    }
    bool secure_output_enabled() const {
        return enable_secure_output_;
    }
    auto sc_chip_log() const {
        if(!enable_secure_output_) throw std::runtime_error("secure output not enable");
        auto chip_log = secure_output_ / "chip_log.json";
        return check_path(chip_log);

    }
    boost::filesystem::path marker_append_path(
        const std::string& task_id,
        int r, int c, const std::string& ch
    ) const {
        boost::filesystem::path odir(output_);
        auto res = check_path(odir / "marker_append" / (
            task_id + "-" + 
            std::to_string(r) + "-" + std::to_string(c) + "-" + ch +
            ".tiff"
        ));
        // std::cout << "marker append path: " << res << std::endl;
        return res;
    }
    boost::filesystem::path input() const {
        return input_;
    }
    boost::filesystem::path output() const {
        return output_;
    }
    boost::filesystem::path sc_raw_img_dir() const {
        if(!enable_secure_output_) throw std::runtime_error("secure output not enable");
        check_path(secure_output_ / "tmp");
        return secure_output_;
    }
    boost::filesystem::path array_cen(
        const std::string& task_id
    ) const {
        boost::filesystem::path output_p(output_);
        switch(mode_) {
            case normal:
                output_p = output_p / task_id
                    / "array.cen";
                break;
            case inplace:
                output_p = output_p / "grid" 
                    / "array.cen";
                break;
            default:
                throw std::runtime_error("unsupport mode");
        }
        return check_path(output_p);
    }
    boost::filesystem::path heatmap(
        const std::string& task_id,
        const std::string& ch,
        const OutputFormat::Labels& ofm
    ) const {
        if(ofm == OutputFormat::cen_file) {
            return array_cen(task_id);
        }
        auto postfix = OutputFormat::to_file_postfix(ofm);
        boost::filesystem::path odir(output_);
        switch(mode_) {
            case normal:
                return check_path((output_ / (ch + postfix)).string());
            case inplace:
                return check_path(odir / "grid" / "channels" / ch / ( "heatmap" + postfix ));
            default:
                throw std::runtime_error("unsupport mode");
        }
    }
    boost::filesystem::path fov_image(
        const std::string& task_id,
        const std::string& tag,
        int r, int c, const std::string& ch
    ) const {
        return check_path(general_prefix(output_.string(), task_id, ch) / fov_image_posfix(tag, r, c, ch));
    }
    boost::filesystem::path stitch_image(
        const std::string& task_id,
        const std::string& tag,
        const std::string& ch
    ) const {
        return check_path(general_prefix(output_.string(), task_id, ch) / stitch_image_posfix(tag, ch));
    }
    boost::filesystem::path gridline(
        const std::string& task_id,
        const std::string& channel
    ) const {
        return check_path(general_prefix(output_.string(), task_id, channel) / "gridline.csv");
    }
    boost::filesystem::path complete_file(
        const std::string& task_id, 
        const std::string& path
    ) const {
        boost::filesystem::path path_p(path);
        switch(mode_) {
            case normal:
                path_p = path_p / task_id
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
    void create_complete_file(
        const std::string& task_id
    ) const {
        for(auto&& p : {input_, output_}){
            auto complete_file_path = complete_file(task_id, p.string());
            std::ofstream fout(complete_file_path.string(), std::fstream::trunc | std::fstream::out);
            fout.close();
        }
    }
    boost::filesystem::path background(
        const std::string& task_id,
        const std::string& channel
    ) const {
        boost::filesystem::path odir(output_);
        switch(mode_) {
            case normal:
                return check_path(odir /(channel + "_background.csv"));
            case inplace:
                return check_path(
                    general_prefix(output_.string(), task_id, channel) / "background.csv"
                );
            default:
                throw std::runtime_error("unsupport mode");
        }
    }
    boost::filesystem::path debug(
        const std::string& task_id, 
        const std::string& channel
    ) const {
        auto dir = check_path(general_prefix(output_.string(), task_id, channel) / "debug");
        return dir;
    }
    boost::filesystem::path debug_img(
        const std::string& task_id, 
        const std::string& channel,
        int fov_r, int fov_h,
        const std::string& tag
    ) const {
        auto debug_dir = debug(task_id, channel);
        std::stringstream ss;
        ss << fov_r << '-' << fov_h;
        if(!tag.empty()) {
            ss << '-' << tag;
        }
        ss << ".tiff";
        return check_path(debug_dir / ss.str());
    }
    boost::filesystem::path channel_grid_log(
        const std::string& task_id, 
        const std::string& channel
    ) const {
        return check_path(general_prefix(output_.string(), task_id, channel) / "grid_log.json");
    }
    boost::filesystem::path task_grid_log(
        const std::string& task_id
    ) const {
        switch(mode_) {
            case normal:
                return check_path(output_ / (task_id  + "-grid_log.json"));
            case inplace:
                return check_path(output_ / "grid" / "grid_log.json");
            default:
                throw std::runtime_error("unsupport mode");
        }
    }
private:
    boost::filesystem::path check_path( const boost::filesystem::path& p ) const {
        auto tmp = p;
        tmp.make_preferred();
        auto _dir = tmp.parent_path();
        if(!boost::filesystem::exists(_dir)) {
            boost::filesystem::create_directories(_dir);
        }
        return tmp;
    }
    std::string fov_image_posfix(
        const std::string& tag,
        int r, int c, const std::string& ch
    ) const {
        boost::filesystem::path viewable_tag("viewable_" + tag);
        return ( viewable_tag /
            (std::to_string(r) + "-" +
            std::to_string(c) + ".tiff") ).string();
    }
    std::string stitch_image_posfix(
        const std::string& tag,
        const std::string& ch
    ) const {
        boost::filesystem::path viewable_tag("viewable_" + tag);
        return ( viewable_tag / ("stitch-" + ch + ".png" )).string();

    }
    boost::filesystem::path general_prefix(
        const std::string& output,
        const std::string& task_id,
        const std::string& channel
    ) const {
        boost::filesystem::path output_p(output);
        switch(mode_) {
            case normal:
                return output_p / task_id 
                    / "channels" / channel;
            case inplace:
                return output_p / "grid" 
                    / "channels" / channel;
            default:
                throw std::runtime_error("unsupport mode");
        }
    }
    Mode mode_;
    bool enable_secure_output_;
    boost::filesystem::path secure_output_;
    boost::filesystem::path output_;
    boost::filesystem::path input_;
    boost::filesystem::path shared_dir_;
    boost::filesystem::path secure_dir_;
};

}