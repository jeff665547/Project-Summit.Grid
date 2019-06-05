#pragma once
#include <boost/filesystem.hpp>
#include <iostream>
namespace summit::app::grid2 {

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
        if( abs_output.make_preferred() == abs_input.make_preferred() 
            || abs_output == secure_output_.make_preferred()) {
            std::cout << "use inplace mode" << std::endl;
            mode_ = inplace;
        } else {
            std::cout << "use normal mode" << std::endl;
            mode_ = normal;
        }
        output_ = abs_output;
        input_  = abs_input;
    }
    bool secure_output_enabled() const {
        return enable_secure_output_;
    }
    boost::filesystem::path marker_append(
        const std::string& task_id,
        int r, int c, const std::string& ch
    ) const {
        boost::filesystem::path odir(output_);
        auto res = check_path(odir / "marker_append" / (
            task_id + "-" + 
            std::to_string(r) + "-" + std::to_string(c) + "-" + ch +
            ".tiff"
        ));
        std::cout << "marker append path: " << res << std::endl;
        return res;
    }
    boost::filesystem::path raw_img_dir() const {
        if(!enable_secure_output_) throw std::runtime_error("secure output not enable");
        check_path(secure_output_ / "tmp");
        return secure_output_ ;
    }
    boost::filesystem::path heatmap(
        const std::string& ch,
        const std::string& postfix
    ) const {
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
    boost::filesystem::path single_image(
        const std::string& task_id,
        const std::string& tag
    ) const {
        boost::filesystem::path output_p(output_);
        switch(mode_) {
            case normal:
                return check_path(output_p / task_id / ("viewable_" + tag + ".png"));
            default:
                throw std::runtime_error("unsupport mode");
        }
    }
    boost::filesystem::path gridline(
        const std::string& task_id,
        const std::string& channel
    ) const {
        return check_path(general_prefix(output_.string(), task_id, channel) / "gridline.csv");
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
    bool enable_secure_output_;
    boost::filesystem::path secure_output_;
    boost::filesystem::path output_;
    boost::filesystem::path input_;
};

}