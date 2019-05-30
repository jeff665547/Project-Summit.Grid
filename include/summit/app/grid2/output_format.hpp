#pragma once
namespace summit::app::grid2 {

struct OutputFormat {
    enum Labels{
        tsv_probe_list, 
        csv_probe_list, 
        html_probe_list,
        cen_file,
        csv_matrix
    };
};

}