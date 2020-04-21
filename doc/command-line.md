
Command-line
============

[TOC]

Overview
========

Summit.Grid is a command-line tool which includes the options:

| option           | parameter type  | default        | shortcut | function                                                                                                                                                       |
|------------------|-----------------|----------------|----------|----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| help           | (none)          | (none)         | -h       | Show help message                                                                                                                                              |
| input_path     | path            | (none)         | -i       | The input path, can be directory or file. If the path is a directory, a chip_log.json file must in the directory. If the path is a file, it must be grid_log.json |
| output_formats | string[,string] | csv_probe_list | -r       | Identify the output file format see [output specification](@ref doc/output-spec.md) for more details                                                                                                                              |
| filter         | string          | all            | -l       | The output feature filter                                                                                                                                      |
| output         | path            | (none)         | -o       | The output path                                                                                                                                                |
| debug          | int [0, 6]      | 0              | -d       | Verbose levels, can be [0,6] and if the level >= 5, the program will generate debug image                                                                      |
| no_bgp         | (none)          | (none)         | -b       | No image background subtraction                                                                                                                                |
| shared_dir     | path            | (none)         | -a       | The shared directory from reader IPC to image server                                                                                                           |
| secure_dir     | path            | (none)         | -e       | The private directory on image server                                                                                                                          |
| thread_num     | int             | 1              | -n       | The thread number used in the image process                                                                                                                    |
| method         | string          | auto_min_cv    | -s       | The signal extraction method                                                                                                                                   |
| marker_append  | (none)          | (none)         | -m       | Generate marker append image                                                                                                                                   |
| version        | (none)          | (none)         | -v       | Show version info                                                                                                                                              |

This description can viewed by call ```summit-app-grid --help```

input_path details
------------------

If the input path is a directory then the directory will be recognized as a scanned chip directory and Summit.Grid will run the auto gridding mode, while if the input path is a file then the file will be recognized as a grid log, and Summit.Grid run the manually re-gridding process.

filter details
--------------

This is a filter to probe feature positions, it has two options: ```all``` and ```marker_only```.

* ```all``` means every probe position is output.
* ```marker_only``` means only the value in marker regions of chip will be output, this option is used in factorial QA/QC process. In general, users should not care this option.
