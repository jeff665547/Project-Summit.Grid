/**
 * @file main.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief The option parser and basic wrapper to main function
 * 
 */
#pragma once
#include <string>
#include <Nucleona/app/cli/option_parser.hpp>
#include <ChipImgProc/utils.h>
#include <regex>
#include <Nucleona/sys/executable_dir.hpp>
#include <boost/optional.hpp>
#include <Nucleona/algo/split.hpp>
#include <ChipImgProc/marker/layout.hpp>
#include <ChipImgProc/multi_tiled_mat.hpp>
#include <Nucleona/proftool/timer.hpp>
#include <summit/grid/version.hpp>
#include "model.hpp"
#include "utils.hpp"
#include "pipeline.hpp"

#include <Nucleona/proftool/gprofiler.hpp>

/**
 * Bright-field Images Gridding Process
 * 
 * 0.  Load chip images, parameters and configs.
 * 1.  Take the bright-field (BF, white channel) images as the reference for the 
 *     entire gridding process.
 * 2.  Downsample bright-field images and identify positions of ArUco code 
 *     surrounded by rectangle markers.
 * 3.  Filter ArUco code to exclude those which should not be shown in the current 
 *     FOV (field of vision).
 * 4.  Search ArUco marker again to estimate the warp matrix for calculating the
 *     rotation angle and getting the relationship between bright-field images 
 *     and the theoretically rescaled GDS spec for the corresponding chip type.
 * 
 * ##  For each remaining probe channel:
 * 
 * 5.  Estimate the sub-pixel bias from the marker positions between that probe 
 *     channel and the white channel via the bright-filed warp matrix.
 * 6.  Add the bias to the bright-filed warp matrix to get the true position of 
 *     the fluorescent markers and probes due to the invariant of the rotation 
 *     degree among bright-filed images and fluorescent images.
 * 7.  By using the information from the updated warp matrix and the theoretical 
 *     GDS spec, grid line can be known and can be mapped into any pixel and 
 *     subpixel domain.
 * 8.  For each cell (feature/probe) in every FOV, find a fixed size rectangle 
 *     whose coefficient of variation is the smallest to calculate the heatmap 
 *     intensity (mean value of that rectangle) for the corresponding probe.
 * 9.  Rotate and crop the tilt images. Use markers as anchers, and stitch all FOVs
 *     together to get the stitched chip image.
 * 10. Output heatmap for downstream analysis, and all other images (marker-append 
 *     images, stitched images, gridline images, etc.) for debug and shown only.
 * 
 */

/**
 * Probe Channel (Fluorescent) Images Gridding Process
 * 
 * 0.  Load chip images, parameters and configs.
 * 1.  Take images of some probe channel as the reference channel for the entire 
 *     gridding process. 
 * 2.  Downsample referenced-channel images and identify positions of each fluorescent 
 *     marker.
 * 3.  Estimate the warp matrix from the referenced-channel marker positions for 
 *     calculating the rotation angle and getting the relationship between the 
 *     referenced-channel images and the theoretically rescaled GDS spec for the 
 *     corresponding chip type.
 * 
 * ##  For each remaining probe channel:
 * 
 * 4.  Estimate the sub-pixel bias from the marker positions between that probe channel
 *     and the referenced channel via the referenced-channel warp matrix.
 * 5.  Add the bias to the referenced-channel warp matrix to get the true positions of 
 *     fluorescent markers and probes under that probe channel due to the invariant of 
 *     the rotation degree among referenced-channel images and remaining probe channel 
 *     images.
 * 
 * ##  The subsequent process is similar to that in the bright-field images gridding 
 *     process.
 * 
 */
 
namespace summit::app::grid{

/**
 * @brief The storage data structure of command-line parameters, 
 *        basically 1 to 1 mapping to each option
 * @details See @ref doc/command-line.md for more details.
 */
struct Parameters
{
    /**
     * @brief See @ref doc/command-line.md for more details. 
     */
    std::string input_path;
    /**
     * @brief See @ref doc/command-line.md for more details. 
     */
    std::vector<std::string> output_formats;
    /**
     * @brief See @ref doc/command-line.md for more details. 
     */
    std::string filter;
    /**
     * @brief See @ref doc/command-line.md for more details. 
     */
    std::string output;
    /**
     * @brief See @ref doc/command-line.md for more details. 
     */
    std::string method;
    /**
     * @brief See @ref doc/command-line.md for more details. 
     */
    int debug;
    /**
     * @brief See @ref doc/command-line.md for more details. 
     */
    bool enable_log;
    /**
     * @brief See @ref doc/command-line.md for more details. 
     */
    bool no_bgp;
    /**
     * @brief See @ref doc/command-line.md for more details. 
     */
    std::string shared_dir;
    /**
     * @brief See @ref doc/command-line.md for more details. 
     */
    std::string secure_dir;
    /**
     * @brief See @ref doc/command-line.md for more details. 
     */
    int thread_num;
    /**
     * @brief See @ref doc/command-line.md for more details. 
     */
    bool marker_append;


    bool force_inplace;
};

/**
 * @brief Option parser of Summit.Grid
 */
class OptionParser : public Parameters, public nucleona::app::cli::OptionParser
{
    using Base = nucleona::app::cli::OptionParser;
public:
    /**
     * @brief Construct a new Option Parser object from command-line argc and argv
     * 
     * @param argc command-line arguments count
     * @param argv command-line arguments
     */
    OptionParser(int argc, char* argv[])
    {
        namespace po = boost::program_options;
        po::options_description desc("Summit image gridding tool, version: " + summit::grid::version().to_string() + ", allowed options");
        desc.add_options()
            ("help,h"           ,       "show help message")
            ("input_path,i"     ,       po::value<std::string>()->required(),                  "The input path, can be directory or file."
                                                                                               "If the path is directory, a chip_log.json file must in the directory."
                                                                                               "If the path is a file, it must be grid_log.json"
            )
            ("output_formats,r" ,       po::value<std::string>()->required()
                                                       ->default_value("csv_probe_list"),      "Identify the output file format"
            )
            ("filter,l"         ,       po::value<std::string>()->required()
                                                       ->default_value("all"),                 "The output feature filter"
            )
            ("output,o"         ,       po::value<std::string>()->required()
                                                       ->default_value(""),                    "The output path."
            )
            ("debug,d"          ,       po::value<int>()->required()
                                                        ->default_value(0),                    "Verbose levels, can be [0,6] and if the level >= 2, the program will generate debug image."
            )
            ("enable_log,g"     ,       "Show program log file (summit-grid.log).")
            ("no_bgp,b"         ,       "No background process.")
            ("shared_dir,a"     ,       po::value<std::string>()->default_value(""),           "The share directory from reader IPC to image server")
            ("secure_dir,e"     ,       po::value<std::string>()->default_value(""),           "The private directory on image server")
            ("thread_num,n"     ,       po::value<int>()->default_value(1),                    "The thread number used in the image process")
            ("method,s"         ,       po::value<std::string>()->required()
                                                        ->default_value("auto_min_cv"),        "The signal extraction method")
            ("marker_append,m"  ,       "Show marker append")
            ("force_inplace,f"  ,       "Force to use inplace output mode")
            ("version,v"        ,       "Show version info")
        ;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if(argc == 1 || vm.count("help"))
        {
            std::cout << desc << std::endl;
            std::exit(0);
        }
        if(vm.count("version")) {
            std::cout << summit::grid::version().to_string() << std::endl;
            std::exit(0);
        }
        po::notify(vm);
        Base::get_parameter( "input_path"        , input_path        );
        get_parameter(       "output_formats"    , output_formats    );
        Base::get_parameter( "filter"            , filter            );
        Base::get_parameter( "output"            , output            );
        Base::get_parameter( "debug"             , debug             );
        Base::get_parameter( "enable_log"        , enable_log        );
        Base::get_parameter( "no_bgp"            , no_bgp            );
        Base::get_parameter( "shared_dir"        , shared_dir        );
        Base::get_parameter( "secure_dir"        , secure_dir        );
        Base::get_parameter( "thread_num"        , thread_num        );
        Base::get_parameter( "marker_append"     , marker_append     );
        Base::get_parameter( "force_inplace"     , force_inplace     );
        Base::get_parameter( "method"            , method            );
        auto input_path_  = boost::filesystem::absolute(input_path);
        auto output_      = boost::filesystem::absolute(output);
        input_path = input_path_.make_preferred().string();
        output     = output_.make_preferred().string();

        if(enable_log) view_log_at(output_.make_preferred() / "grid");

        if(debug < 0 || debug > 7) {
            throw std::runtime_error("debug level should be [0,7], but " + std::to_string(debug));
        }
    }

    /**
     * @brief Parse list parameter
     * 
     * @tparam StringType deduced, string type
     * @tparam ParType deduced, parse result storage type
     * @param parameter_name Parameter name
     * @param variable Target storage
     */
    template< class StringType, class ParType >
    void get_parameter(
        StringType&& parameter_name, 
        std::vector<ParType>& variable
    )
    {
        if(vm.count(parameter_name)) {
            variable = nucleona::algo::split(
                vm[parameter_name].template as<ParType>(), ","
            );
        }
    }

    void view_log_at(
        const boost::filesystem::path& log_dst_dir
    ) {
        // auto log_dst_dir(boost::filesystem::absolute(args_.output).make_preferred() / "grid");
        if(!boost::filesystem::exists(log_dst_dir)) {
            boost::filesystem::create_directories(log_dst_dir);
        }
        summit::grid::log.setup_log_path(log_dst_dir.string());
    }
};

/**
 * @brief Program start point wrapper
 * 
 * @tparam OPTION_PARSER parameter storage type
 */
template<class OPTION_PARSER>
class Main
{
    /**
     * @brief command-line parameter storage
     * 
     */
    OPTION_PARSER args_;

  public:
    /**
     * @brief Construct a new Main object
     * 
     * @param args command-line parameter storage
     */
    Main( OPTION_PARSER&& args )
    : args_( std::forward<OPTION_PARSER>( args ) )
    {}

    /**
     * @brief Detect the user requirement is auto gridding or not.
     * 
     * @param grid_log_path The user given check target, may not a valid grid log.
     * @return true User require manual gridding
     * @return false User require auto gridding
     */
    bool is_not_auto_gridding(boost::filesystem::path& grid_log_path) {
        boost::filesystem::path path(args_.input_path);
        path = boost::filesystem::absolute(path);
        path = path.make_preferred();
        if(path.filename() == "grid_log.json") {
            grid_log_path = path;
            return true;
        } else {
            return false;
        }
    }
    static int no_cv_error(
        int status, const char *func_name, const char *err_msg, 
        const char *file_name, int line, void *userdata)
    {
        //Do nothing -- will suppress console output
        return 0;
    }

    /**
     * @brief Summit.Grid start point.
     * 
     * @return int exit code, 0 is success, otherwise failed.
     */
    int operator()() {
        auto tmp_timer(std::chrono::steady_clock::now());
        std::chrono::duration<double, std::milli> d;
        nucleona::proftool::GProfiler profiler("summit-grid-prof.out");
        /*
         *  set logger level
         */
        cv::redirectError(no_cv_error);
        summit::grid::log.set_level(std::min(args_.debug, 6));
        chipimgproc::log.set_level(std::max(args_.debug - 1, 0));
        Model model;
        model.set_executor(args_.thread_num - 1);
        model.set_debug(args_.debug);

        summit::grid::log.info("Program version: {}", summit::grid::version().to_string());

        boost::filesystem::path grid_log_path; 
        std::vector<boost::filesystem::path> task_paths;
        if(is_not_auto_gridding(grid_log_path)) {
            /*
             * Prepare for a manual grid task
             * start updating grid result images
             */
            summit::grid::log.info("input require not to auto gridding");
            summit::grid::log.info("direct use gridding parameter in input grid log");
            model.set_method(args_.method);
            model.set_not_auto_gridding(grid_log_path.string());
            task_paths = Utils::task_paths(
                model.in_grid_log()["chip_dir"].get<std::string>()
            );
            model.add_formats(args_.output_formats);
        } else {
            /*
             * Prepare for an automatic gridding task
             * See @ref doc/algorithm.md for more details.
             */
            summit::grid::log.info("input require auto gridding, process everything");
            /*
             * set basic parameters
             */
            model.set_paths(
                args_.output, args_.input_path,
                args_.shared_dir, args_.secure_dir,
                args_.force_inplace
            );
            model.set_formats(args_.output_formats);
            model.set_marker_append(args_.marker_append);
            model.set_filter(args_.filter);
            model.set_no_bgp(args_.no_bgp);
            model.set_method(args_.method);
            model.set_auto_gridding(true);
            /*
             * collect tasks
             */
            task_paths = Utils::task_paths(model.input());
        }

        /*
         * create task group data model, recognize task RFID, chip id.
         * By implementation, the Summit.Grid actually 
         * support RFID level batch prcoess, 
         * but for the system complexity, 
         * this feature doesn't specify in specification.
         * 
         */
        std::map<
            std::string, // rfid
            std::vector<TaskID>  // task_path
        > task_groups;
        for(auto& task_path : task_paths) {
            auto&& task_id = Utils::to_task_id(task_path);
            task_groups[task_id.rfid()].push_back(task_id) ;
        }
        /*
         * start process pipeline
         */
        std::atomic<int> exit_code = 0;
        task_groups
        | ranges::view::transform([&](auto&& tg_param){
            auto& rfid = tg_param.first;
            auto& task_ids = tg_param.second;
            model::TaskGroup task_group;
            task_group.set_model(model);
            task_group.set_rfid(rfid);
            task_group.set_task_ids(task_ids);
            exit_code |= pipeline::rfid(task_group);
            return 0;
        })
        | nucleona::range::endp
        ;
        summit::grid::log.info("process done"); 
        // d = std::chrono::steady_clock::now() - tmp_timer;
        // std::cout << "Time cost: " << d.count() << " ms\n";
        return exit_code;
    }
};

template<class OPTION_PARSER>
auto make ( OPTION_PARSER&& option_parser )
{
    return Main<OPTION_PARSER> ( 
        std::forward < OPTION_PARSER > ( option_parser )
    );
}}
