#pragma once
#include <Nucleona/format/csv_parser.hpp>

namespace summit::app::light_mean {
struct HeatmapEntry {
    std::string task_id;
    int x, y;
    float mean;
    std::string is_mk;
    int mk_x, mk_y;
    int mk_sub_x, mk_sub_y;
};
using HeatmapParserTrait = nucleona::format::CsvEntryTrait<
    HeatmapEntry,
    nucleona::format::PMT<0,  std::string>,
    nucleona::format::PMT<1,  int>,
    nucleona::format::PMT<2,  int>,
    nucleona::format::PMT<4,  float>,
    nucleona::format::PMT<7,  std::string>,
    nucleona::format::PMT<8,  int>,
    nucleona::format::PMT<9,  int>,
    nucleona::format::PMT<10, int>,
    nucleona::format::PMT<11, int>
>;
using HeatmapParser = nucleona::format::CsvParser<HeatmapParserTrait>;

}