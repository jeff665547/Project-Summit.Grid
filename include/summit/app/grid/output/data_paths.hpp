#pragma once
#include <boost/filesystem.hpp>
#include <iostream>
namespace summit::app::grid::output {

struct DataPaths {
    enum Mode {
        inplace, normal
    };
    DataPaths(const std::string& output, const boost::filesystem::path& input) {
        boost::filesystem::path abs_output(output);
        boost::filesystem::path abs_input(input);
        abs_output = boost::filesystem::absolute(abs_output);
        abs_input = boost::filesystem::absolute(abs_input);
        if( abs_input.make_preferred() == abs_output.make_preferred() ) {
            std::cout << "use inplace mode" << std::endl;
            mode_ = inplace;
        } else {
            std::cout << "use normal mode" << std::endl;
            mode_ = normal;
        }
    }
    boost::filesystem::path heatmap(
        const std::string& output, 
        const std::string& task_id,
        const std::string& ch,
        const std::string& format
    ) const {
        boost::filesystem::path odir(output);
        switch(mode_) {
            case normal:
                return check_path(output + "-" + ch + "." + format);
            case inplace:
                return check_path(odir / "grid" / "channels" / ch / ( "heatmap." + format ));
            default:
                throw std::runtime_error("unsupport mode");
        }
    }
    boost::filesystem::path fov_image(
        const std::string& output, 
        const std::string& task_id,
        const std::string& tag,
        int r, int c, const std::string& ch
    ) const {
        return check_path(general_prefix(output, task_id, ch) / fov_image_posfix(tag, r, c, ch));
    }
    boost::filesystem::path stitch_image(
        const std::string& output,
        const std::string& task_id,
        const std::string& tag,
        const std::string& ch
    ) const {
        return check_path(general_prefix(output, task_id, ch) / stitch_image_posfix(tag, ch));
    }
    boost::filesystem::path single_image(
        const std::string& output,
        const std::string& task_id,
        const std::string& tag
    ) const {
        boost::filesystem::path output_p(output);
        switch(mode_) {
            case normal:
                return check_path(output_p / task_id / ("viewable_" + tag + ".png"));
            default:
                throw std::runtime_error("unsupport mode");
        }
    }
    boost::filesystem::path gridline(
        const std::string& output,
        const std::string& task_id,
        const std::string& channel
    ) const {
        return check_path(general_prefix(output, task_id, channel) / "gridline.csv");
    }
    boost::filesystem::path array_cen( 
        const std::string& output,
        const std::string& task_id
    ) const {
        boost::filesystem::path output_p(output);
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
private:
    boost::filesystem::path check_path( const boost::filesystem::path& p ) const {
        auto _dir = p.parent_path();
        if(!boost::filesystem::exists(_dir)) {
            boost::filesystem::create_directories(_dir);
        }
        return p;
    }
    std::string fov_image_posfix(
        const std::string& tag,
        int r, int c, const std::string& ch
    ) const {
        boost::filesystem::path viewable_tag("viewable_" + tag);
        return ( viewable_tag /
            (std::to_string(r) + "-" +
            std::to_string(c) + ".png") ).string();
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
};

}