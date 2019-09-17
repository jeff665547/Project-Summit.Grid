#pragma once
#include <string>
#include <Nucleona/app/cli/option_parser.hpp>
#include <boost/filesystem.hpp>
#include "core.hpp"
#include "heatmap_parser.hpp"
#include <Nucleona/range.hpp>
#include <Nucleona/algo/segment.hpp>
#include <summit/format/csv_out.hpp>
namespace summit::app::light_mean {
struct Parameter {
    std::string input;
    std::string output;
    std::string details;
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
            ("details,e"         ,      po::value<std::string>(),                   "detail probe value output")
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
        Base::get_parameter("details"     , details);
        Base::get_parameter("chip_spec"   , chip_spec);
        Base::get_parameter("marker_type" , marker_type);
        auto i_tmp = boost::filesystem::absolute(input);
        input = i_tmp.make_preferred().string();
        auto o_tmp = boost::filesystem::absolute(output);
        output = o_tmp.make_preferred().string();
        if(!details.empty()) {
            auto d_tmp = boost::filesystem::absolute(details);
            details = d_tmp.make_preferred().string();
        }
    }

};
struct Main {
    Main(OptionParser&& args)
    : args_(std::move(args))
    {}
    auto row_3_idx(int row_num) {
        std::cout << VDUMP(row_num) << std::endl;
        std::vector<std::size_t> res;
        res.push_back(0);
        res.push_back(row_num / 2);
        res.push_back(row_num - 1);
        return res;
    }
    template<class Rng>
    auto extract_row(
        // const std::vector<cv::Mat_<float>>& markers,
        Rng&& buffer,
        int row,
        int col_num
    ) {
        // std::vector<cv::Mat_<float>> res;
        // for(int i = 0; i < col_num; i ++ ) {
        //     res.push_back(markers.at(col_num * row + i));
        // }
        // return res;
        std::vector<HeatmapEntry> res;
        for(auto&& ent : buffer) {
            if(ent.mk_y == row) {
                res.push_back(ent);
            }
        }
        return res;
    }
    void count_and_write(
        std::ostream& out, 
        std::ostream& detail_out,
        const std::vector<HeatmapEntry>& buffer
    ) {
        const auto& detector = buffer.back();

        // std::vector<cv::Mat_<std::uint32_t>> markers;
        Core core;
        int mk_idx_row = detector.mk_y + 1;
        int mk_idx_col = detector.mk_x + 1;
        // markers.resize(
        //     mk_idx_col * mk_idx_row
        // );
        int mk_row = detector.mk_sub_y + 1;
        int mk_col = detector.mk_sub_x + 1;
        // for(auto&& mk : markers) {
        //     mk = cv::Mat_<std::uint32_t>(mk_row, mk_col);
        // }
        // for(std::size_t i = 0; i < buffer.size(); i ++ ) {
        //     auto& entry = buffer.at(i);
        //     auto& mk = markers.at(mk_idx_col * entry.mk_y + entry.mk_x);
        //     mk(entry.mk_sub_y, entry.mk_sub_x) = i;
        // }
        auto light_probe_idx = core.select_light_probe(buffer, args_.chip_spec, args_.marker_type);
        auto light_probes = buffer 
            | nucleona::range::at(light_probe_idx)
        ;
        summit::format::csv_out(
            detail_out, ",", 
            "task_id", "x", "y", "mean", "mk_x", "mk_y", 
            "mk_sub_x", "mk_sub_y"
        );
        for(auto&& lp : light_probes) {
            summit::format::csv_out(
                detail_out, ",",
                lp.task_id, lp.x, lp.y, lp.mean, lp.mk_x, lp.mk_y,
                lp.mk_sub_x, lp.mk_sub_y
            );
        }
        auto light_probe_means = light_probes
            | nucleona::range::transformed([](auto&& ent){
                return ent.mean;
            })
        ;
        auto res = core.statistic(light_probe_means);
        summit::format::csv_out(
            out, ",", 
            "id", "mean", "cv"
        );
        summit::format::csv_out(
            out, ",",
            detector.task_id, res.mean, res.cv
        );
        
        // show upper, mid, and button row
        auto row_3 = row_3_idx(mk_idx_row);
        for(auto&& [i, ridx] : nucleona::range::indexed(row_3, 0)) {
            auto curr_row = extract_row(light_probes, ridx, mk_idx_col);
            auto curr_row_res = core(curr_row, args_.chip_spec, args_.marker_type);
            summit::format::csv_out(
                out, ",",
                detector.task_id + "-row-" + std::to_string(i), 
                curr_row_res.mean, 
                curr_row_res.cv
            );
        }

    }
    void consume_header(std::istream& is) {
        std::string header;
        std::getline(is, header);
    }

    auto operator()() {
        std::ifstream heatmap_file(args_.input);
        std::ofstream output_file(args_.output);
        std::ofstream details_file_impl;
        std::ostream* p_details_file;
        if(args_.details.empty()) {
            p_details_file = &nucleona::stream::null_out;
        } else {
            details_file_impl.open(args_.details);
            p_details_file = &details_file_impl;
        }
        auto& details_file = *p_details_file;

        HeatmapParser parser(",");
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
                    count_and_write(output_file, details_file, buffer);
                }
                buffer.clear();
                last_task_id = entry.task_id;
            }
            if(entry.is_mk == "true") {
                buffer.push_back(entry);
            }
        }
        if(!buffer.empty()) {
            count_and_write(output_file, details_file, buffer);
        }
        return 0;
    }
private:
    OptionParser args_;
};
}