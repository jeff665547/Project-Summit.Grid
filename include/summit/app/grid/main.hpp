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
#include <summit/app/grid/single.hpp>
#include <summit/app/grid/chipscan.hpp>
#include <summit/app/grid/reg_mat_mk_index.hpp>

namespace summit::app::grid{

struct Parameters
{
    std::string input_path;
    std::string chip_type;
    int fov_ec_id;
    std::vector<std::string> channel_names;
    std::vector<std::string> spectrum_names;
    std::vector<std::string> output_formats;
    std::string filter;
    std::string output;
    int debug;
    float um2px_r;
};

class OptionParser : public Parameters, public nucleona::app::cli::OptionParser
{
    using Base = nucleona::app::cli::OptionParser;
public:
    OptionParser(int argc, char* argv[])
    {
        namespace po = boost::program_options;
        po::options_description desc("Summit marker probe QA, allowed options");
        desc.add_options()
            ("help,h"           ,       "show help message")
            ("input_path,i"     ,       po::value<std::string>()->required(),                  "The input path, can be directory or file."
                                                                                               "If the path is directory, a chip_log.json file must in the directory."
            )
            ("chip_type,t"      ,       po::value<std::string>()->required()                      
                                                            ->default_value(
                                                                "zion22"
                                                            ),                                 "Select a chip type in cell_fov.json."
            )
            ("fov_ec_id,f"      ,       po::value<int>()->required()
                                                    ->default_value(0),                        "The <n>th FOV related to chip type."
            )
            ("channel_names,c"  ,       po::value<std::string>()->required()
                                    ->default_value("CY3,CY5,CY3_500K,CY3_1M,CY3_2M,CY3_4M"),  "The channel going to process. (Centrillion version favor)"
            )
            ("spectrum_names,s" ,       po::value<std::string>()->required()
                                                       ->default_value("2,3"),                 "The spectrum going to process. (TIRC version favor)"
            )
            ("output_formats,r" ,       po::value<std::string>()->required()
                                                       ->default_value("csv,html"),            "Identify the output file format"
            )
            ("filter,l"         ,       po::value<std::string>()->required()
                                                       ->default_value("marker_only"),         "The output feature filter"
            )
            ("output,o"         ,       po::value<std::string>()->required(),                  "The output path."
            )
            ("debug,d"          ,       po::value<int>()->required()
                                                        ->default_value(0),                    "Show debug images."
            )
            ("um2px_r,u"        ,       po::value<float>()->required()
                                                        ->default_value(-1.0),                 "Specify the um to pixel rate."
            )
        ;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if(argc == 1 or vm.count("help"))
        {
            std::cout << desc << std::endl;
            std::exit(1);
        }
        po::notify(vm);
        Base::get_parameter( "input_path"        , input_path        );
        Base::get_parameter( "chip_type"         , chip_type         );
        Base::get_parameter( "fov_ec_id"         , fov_ec_id         );
        get_parameter(       "channel_names"     , channel_names     );
        get_parameter(       "spectrum_names"    , spectrum_names    );
        get_parameter(       "output_formats"    , output_formats    );
        Base::get_parameter( "filter"            , filter            );
        Base::get_parameter( "output"            , output            );
        Base::get_parameter( "debug"             , debug             );
        Base::get_parameter( "um2px_r"           , um2px_r           );
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

    using Float = float;
    using GridLineID = std::uint16_t;


    OPTION_PARSER args_;

  public:
    Main( OPTION_PARSER&& args )
    : args_( std::forward<OPTION_PARSER>( args ) )
    {}

    std::vector<Task> get_tasks() {
        namespace bfs = boost::filesystem;
        std::cout << "input_path: " << args_.input_path << std::endl;
        return Task::search(args_.input_path);
    }
    void task_proc(
        const Task& tk,
        const output::DataPaths& output_paths,
        output::HeatmapWriter<Float, GridLineID>& heatmap_writer
    ) {
        switch(tk.type) {
            case Task::single:
                single_type_proc(
                    tk.path, args_.chip_type, 
                    args_.fov_ec_id, args_.um2px_r, 
                    args_.output,
                    args_.output_formats, tk.id(),
                    args_.filter, args_.debug,
                    output_paths, heatmap_writer
                );
                break;
            case Task::chipscan:
                chipscan_type_proc(
                    tk.path, args_.chip_type, 
                    args_.channel_names,
                    args_.spectrum_names,
                    args_.um2px_r, args_.output,
                    args_.output_formats, tk.id(),
                    args_.filter, args_.debug,
                    output_paths, heatmap_writer
                );
                break;
            default:
                break;
        }
    }
    int operator()() {
        output::DataPaths output_paths(
            args_.output, args_.input_path
        );
        output::HeatmapWriter<Float, GridLineID> heatmap_writer(
            output_paths, args_.output_formats
        );
        for ( auto&& tk : get_tasks() ) {
            try{
                task_proc(tk, output_paths, heatmap_writer);
            } catch ( const std::exception& e ) {
                std::cerr << "error when process task: " << tk.path << std::endl;
                std::cerr << e.what() << std::endl;
                std::cout << tk.path << "skip" << std::endl;
            }
        }
        return 0;
    }
    Single      single_type_proc        ;
    ChipScan    chipscan_type_proc      ;
};

template<class OPTION_PARSER>
auto make ( OPTION_PARSER&& option_parser )
{
    return Main<OPTION_PARSER> ( 
        std::forward < OPTION_PARSER > ( option_parser )
    );
}
}