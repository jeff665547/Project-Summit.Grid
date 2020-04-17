
Command-line
============

Overview
========

Summit.Grid is a command-line tool which includes the options:

```sh
Summit image gridding tool, version: 1.1.4, allowed options:
  -h [ --help ]                         show help message
  -i [ --input_path ] arg               The input path, can be directory or
                                        file.If the path is directory, a 
                                        chip_log.json file must in the
                                        directory.If the path is a file, it 
                                        must be grid_log.json
  -r [ --output_formats ] arg (=csv_probe_list)
                                        Identify the output file format
  -l [ --filter ] arg (=all)            The output feature filter
  -o [ --output ] arg                   The output path.
  -d [ --debug ] arg (=0)               Verbose levels, can be [0,6] and if the
                                        level >= 2, the program will generate
                                        debug image.
  -b [ --no_bgp ]                       No background process.
  -a [ --shared_dir ] arg               The share directory from reader IPC to
                                        image server
  -e [ --secure_dir ] arg               The private directory on image server
  -n [ --thread_num ] arg (=1)          The thread number used in the image
                                        process
  -s [ --method ] arg (=auto_min_cv)    The signal extraction method
  -m [ --marker_append ]                Show marker append
  -v [ --version ]                      Show version info
```

This description can viewed by call ```summit-app-grid --help```
