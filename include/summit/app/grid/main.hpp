#pragma once
#include <string>
#include <Nucleona/app/cli/option_parser.hpp>
#include <ChipImgProc/utils.h>
#include <regex>
#include <Nucleona/sys/executable_dir.hpp>
#include <boost/optional.hpp>
#include <Nucleona/algo/split.hpp>
#include <ChipImgProc/comb/single_general.hpp>
#include <ChipImgProc/multi_tiled_mat.hpp>
#include <Nucleona/proftool/timer.hpp>
#include <summit/grid/version.hpp>
#include "model.hpp"
#include "utils.hpp"
#include "pipeline.hpp"

namespace summit::app::grid{

struct Parameters
{
    std::string input_path;
    std::vector<std::string> output_formats;
    std::string filter;
    std::string output;
    std::string method;
    int debug;
    bool  no_bgp;
    std::string shared_dir;
    std::string secure_dir;
    int thread_num;
    bool marker_append;
};

class OptionParser : public Parameters, public nucleona::app::cli::OptionParser
{
    using Base = nucleona::app::cli::OptionParser;
public:
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
            ("no_bgp,b"         ,       "No background process.")
            ("shared_dir,a"     ,       po::value<std::string>()->default_value(""),           "The share directory from reader IPC to image server")
            ("secure_dir,e"     ,       po::value<std::string>()->default_value(""),           "The private directory on image server")
            ("thread_num,n"     ,       po::value<int>()->default_value(1),                    "The thread number used in the image process")
            ("method,s"         ,       po::value<std::string>()->required()
                                                        ->default_value("auto_min_cv"),        "The signal extraction method")
            ("marker_append,m"  ,       "Show marker append")
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
        Base::get_parameter( "no_bgp"            , no_bgp            );
        Base::get_parameter( "shared_dir"        , shared_dir        );
        Base::get_parameter( "secure_dir"        , secure_dir        );
        Base::get_parameter( "thread_num"        , thread_num        );
        Base::get_parameter( "marker_append"     , marker_append     );
        Base::get_parameter( "method"            , method            );
        auto input_path_  = boost::filesystem::absolute(input_path);
        auto output_      = boost::filesystem::absolute(output);
        input_path = input_path_.make_preferred().string();
        output     = output_.make_preferred().string();

        if(debug < 0 || debug > 7) {
            throw std::runtime_error("debug level should be [0,7], but " + std::to_string(debug));
        }
    }

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
};

template<class OPTION_PARSER>
class Main
{
    // using OPTION_PARSER = OptionParser;


    OPTION_PARSER args_;

  public:
    Main( OPTION_PARSER&& args )
    : args_( std::forward<OPTION_PARSER>( args ) )
    {}

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
    
    int operator()() {
        summit::grid::log.set_level(std::min(args_.debug, 6));
        chipimgproc::log.set_level(std::max(args_.debug - 1, 0));

        Model model;
        model.set_executor(args_.thread_num - 1);
        model.set_debug(args_.debug);

        boost::filesystem::path grid_log_path; 
        std::vector<boost::filesystem::path> task_paths;

        if(is_not_auto_gridding(grid_log_path)) {
            summit::grid::log.info("input require not to auto gridding");
            summit::grid::log.info("direct use gridding parameter in input grid log");
            model.set_method(args_.method);
            model.set_not_auto_gridding(grid_log_path.string());
            task_paths = Utils::task_paths(
                model.in_grid_log()["chip_dir"].get<std::string>()
            );
            model.add_formats(args_.output_formats);
        } else {
            summit::grid::log.info("input require auto gridding, process everything");
            model.set_paths(
                args_.output, args_.input_path,
                args_.shared_dir, args_.secure_dir
            );
            model.set_formats(args_.output_formats);
            model.set_marker_append(args_.marker_append);
            model.set_filter(args_.filter);
            model.set_no_bgp(args_.no_bgp);
            model.set_method(args_.method);
            model.set_auto_gridding(true);
            task_paths = Utils::task_paths(model.input());
        }

        std::map<
            std::string, // rfid
            std::vector<TaskID>  // task_path
        > task_groups;
        for(auto& task_path : task_paths) {
            auto&& task_id = Utils::to_task_id(task_path);
            task_groups[task_id.rfid()].push_back(task_id) ;
        }

        task_groups
        | ranges::view::transform([&](auto&& tg_param){
            auto& rfid = tg_param.first;
            auto& task_ids = tg_param.second;
            model::TaskGroup task_group;
            task_group.set_model(model);
            task_group.set_rfid(rfid);
            task_group.set_task_ids(task_ids);
            pipeline::rfid(task_group);
            return 0;
        })
        | nucleona::range::endp
        ;
        summit::grid::log.info("process done");
        return 0;
    }
};

template<class OPTION_PARSER>
auto make ( OPTION_PARSER&& option_parser )
{
    return Main<OPTION_PARSER> ( 
        std::forward < OPTION_PARSER > ( option_parser )
    );
}}
