#pragma once
#include <string>
#include <Nucleona/app/cli/option_parser.hpp>
#include <boost/filesystem.hpp>
#include "core.hpp"
#include "heatmap_parser.hpp"
namespace summit::app::light_mean {
struct Parameter {
    std::string input;
    std::string output;
    std::string chip_spec;
    std::string marker_type;
};
struct OptionParser 
: public Parameter
, public nucleona::app::cli::OptionParser 
{
    using Base = nucleona::app::cli::OptionParser;
public:
    OptionParser(int argc, char* argv[])
    {
        namespace po = boost::program_options;
        po::options_description desc("Summit marker statistic tool");
        desc.add_options()
            ("help,h"           ,       "show help message")
            ("input,i"          ,       po::value<std::string>()->required(),       "input heatmap table")
            ("output,o"         ,       po::value<std::string>()->required(),       "output heatmap table")
            ("chip_spec,c"      ,       po::value<std::string>()->required()
                                            ->default_value("banff")        ,       "chip spec name(zion/banff/yz01)")
            ("marker_type,m"    ,       po::value<std::string>()->required()
                                            ->default_value("AM1")          ,       "marker type name(AM1/AM3)")
        ;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if(argc == 1 or vm.count("help"))
        {
            std::cout << desc << std::endl;
            std::exit(1);
        }
        po::notify(vm);
        Base::get_parameter("input"       , input);
        Base::get_parameter("output"      , output);
        Base::get_parameter("chip_spec"   , chip_spec);
        Base::get_parameter("marker_type" , marker_type);
        auto i_tmp = boost::filesystem::absolute(input);
        input = i_tmp.make_preferred().string();
        auto o_tmp = boost::filesystem::absolute(output);
        output = o_tmp.make_preferred().string();
    }

};
struct Main {
    Main(OptionParser&& args)
    : args_(std::move(args))
    {}
    void count_and_write(
        std::ostream& out, 
        const std::vector<HeatmapEntry>& buffer
    ) {
        const auto& detector = buffer.back();

        std::vector<cv::Mat_<float>> markers;
        Core core;
        int mk_idx_row = detector.mk_y + 1;
        int mk_idx_col = detector.mk_x + 1;
        markers.resize(
            mk_idx_col * mk_idx_row
        );
        int mk_row = detector.mk_sub_y + 1;
        int mk_col = detector.mk_sub_x + 1;
        for(auto&& mk : markers) {
            mk = cv::Mat_<float>(mk_row, mk_col);
        }
        for(auto&& entry : buffer) {
            auto& mk = markers.at(mk_idx_col * entry.mk_y + entry.mk_x);
            mk(entry.mk_sub_y, entry.mk_sub_x) = entry.mean;
            // std::cout << '(' << entry.mk_sub_x 
            //     << ',' << entry.mk_sub_y << ')' << std::endl;
        }
        auto res = core(markers, args_.chip_spec, args_.marker_type);
        out << detector.task_id << '\t' << res.mean << '\t' << res.cv << '\n';
    }
    void consume_header(std::istream& is) {
        std::string header;
        std::getline(is, header);
    }
    auto operator()() {
        std::ifstream heatmap_file(args_.input);
        std::ofstream output_file(args_.output);

        HeatmapParser parser("\t");
        HeatmapEntry entry;

        std::vector<HeatmapEntry> buffer;
        std::string last_task_id;
        consume_header(heatmap_file);
        std::string line;
        while(std::getline(heatmap_file, line)) {
            if(line.empty()) continue;
            entry = parser(line);
            if(last_task_id != entry.task_id) {
                if(!last_task_id.empty()) {
                    count_and_write(output_file, buffer);
                }
                buffer.clear();
                last_task_id = entry.task_id;
            }
            if(entry.is_mk == "true") {
                buffer.push_back(entry);
            }
        }
        if(!buffer.empty()) {
            count_and_write(output_file, buffer);
        }
        return 0;
    }
private:
    OptionParser args_;
};
}