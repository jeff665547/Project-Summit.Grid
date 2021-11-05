
Command-line
============

[TOC]

Overview
========

Summit.Grid is an image-processing tool designed for transformimg chip images captured from the SUMMIT scanner into the probe intensity. The executable, summit-app-grid, is the command-line interface of this cross-platform program. The following options table lists various actions the tool can perform:

| Option           | Parameter Type  | Default Value  | Shortcut | Description                                                                                                                                                    |
|------------------|-----------------|----------------|----------|----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| help           | (none)          | (none)         | -h       | Display the help information about the Summit.Grid program.                                                                                                      |
| input_path     | path            | (none)         | -i       | The input path that specifies the directory where the chip_log.json and chip images are stored. |
| output_formats | string[,string] | csv_probe_list | -r       | Identify the output file format. See the [output specification](@ref doc/output-spec.md) for more infomation.                                                                                                                              |
| filter         | string          | all            | -l       | The output feature filter                                                                                                                                      |
| output         | path            | (none)         | -o       | The output path                                                                                                                                                |
| debug          | int [0, 6]      | 0              | -d       | Verbose levels, can be [0,6] and if the level >= 5, the program will generate debug image                                                                      |
| enable_log     | (none)          | (none)         | -g       | Output the terminal debug log to the file (summit-grid.log)                                                                                                             |
| no_bgp         | (none)          | (none)         | -b       | No image background subtraction                                                                                                                                |
| shared_dir     | path            | (none)         | -a       | The shared directory from reader IPC to image server                                                                                                           |
| secure_dir     | path            | (none)         | -e       | The private directory on image server                                                                                                                          |
| thread_num     | int             | 1              | -n       | The thread number used in the image process                                                                                                                    |
| method         | string          | auto_min_cv    | -s       | The signal extraction method                                                                                                                                   |
| marker_append  | (none)          | (none)         | -m       | Generate marker append image                                                                                                                                   |
| version        | (none)          | (none)         | -v       | Show version info                                                                                                                                              |

This information can also be viewed by calling ```summit-app-grid --help``` in the terminal.

input_path details
------------------

If the input path is a directory then the directory will be recognized as a scanned chip directory and Summit.Grid will run the auto gridding mode, while if the input path is a file then the file will be recognized as a grid log, and Summit.Grid run the [manually re-gridding](@ref doc/manual-gridding.md) process.

filter details
--------------

This is a filter to probe feature positions, it has two options: ```all``` and ```marker_only```.

* ```all``` means every probe position is output.
* ```marker_only``` means only the value in marker regions of chip will be output, this option is used in factorial QA/QC process. In general, users should not care this option.
