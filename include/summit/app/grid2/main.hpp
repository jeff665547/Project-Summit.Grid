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

namespace summit::app::grid2{

struct Parameters
{
    std::string input_path;
    std::vector<std::string> output_formats;
    std::string filter;
    std::string output;
    int debug;
    float um2px_r;
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
            )
            ("output_formats,r" ,       po::value<std::string>()->required()
                                                       ->default_value("cen"),                 "Identify the output file format"
            )
            ("filter,l"         ,       po::value<std::string>()->required()
                                                       ->default_value("all"),                 "The output feature filter"
            )
            ("output,o"         ,       po::value<std::string>()->required(),                  "The output path."
            )
            ("debug,d"          ,       po::value<int>()->required()
                                                        ->default_value(0),                    "Show debug images."
            )
            ("um2px_r,u"        ,       po::value<float>()->required()
                                                        ->default_value(-1.0),                 "Specify the um to pixel rate."
            )
            ("no_bgp,b"         ,       "No background process.")
            ("shared_dir,a"     ,       po::value<std::string>()->default_value(""),           "The share directory from reader IPC to image server")
            ("secure_dir,e"     ,       po::value<std::string>()->default_value(""),           "The private directory on image server")
            ("thread_num,n"     ,       po::value<int>()->default_value(1),                    "The thread number used in the image process")
            ("marker_append"    ,       "Show marker append")
            ("version,v"        ,       "Show version info")
        ;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if(argc == 1 || vm.count("help"))
        {
            std::cout << desc << std::endl;
            std::exit(1);
        }
        if(vm.count("version")) {
            std::cout << summit::grid::version().to_string() << std::endl;
            std::exit(1);
        }
        po::notify(vm);
        Base::get_parameter( "input_path"        , input_path        );
        get_parameter(       "output_formats"    , output_formats    );
        Base::get_parameter( "filter"            , filter            );
        Base::get_parameter( "output"            , output            );
        Base::get_parameter( "debug"             , debug             );
        Base::get_parameter( "um2px_r"           , um2px_r           );
        Base::get_parameter( "no_bgp"            , no_bgp            );
        Base::get_parameter( "shared_dir"        , shared_dir        );
        Base::get_parameter( "secure_dir"        , secure_dir        );
        Base::get_parameter( "thread_num"        , thread_num        );
        Base::get_parameter( "marker_append"     , marker_append     );
        auto input_path_  = boost::filesystem::absolute(input_path);
        auto output_      = boost::filesystem::absolute(output);
        input_path = input_path_.make_preferred().string();
        output     = output_.make_preferred().string();
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

    int operator()() {
        auto timer = nucleona::proftool::make_timer([](auto&& du){
            std::cout << std::chrono::duration_cast<std::chrono::seconds>(du).count() 
                << " sec." << std::endl;
        });
        Model model;
        model.set_paths(
            args_.output, args_.input_path,
            args_.shared_dir, args_.secure_dir
        );
        model.set_formats(args_.output_formats);
        model.set_executor(args_.thread_num - 1);

        auto&& task_paths = Utils::task_paths(args_.input_path);
        for(auto& task_path : task_paths) {
            auto&& task_id = Utils::to_task_id(task_path);
            auto& task = model.create_task(task_id);
            task.set_chip_dir(task_path);
        }

        model.get_task_groups()
        | ranges::view::transform(pipeline::rfid)
        | nucleona::range::endp
        ;
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