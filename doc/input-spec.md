
Input specification
===================

Input directory tree
====================

Summit.Grid considers a chip directory as a **task** each chip directory should at least one chip_log.json and several images which number should match the FOV specification(chip.fov.rows/cols) inside chip_log.json

```txt
<chip directory>/
    chip_log.json
    chip_log.SFFX (not used in summit-app-grid)
    <row>-<col>-<channel>.<tiff/srl> (fov image set)
```

The ```srl``` file is encripted image.

Chip log format
===============

The chip log describe how the chip is scanned and chip type parameters.
The detail description of each entry in chip log is show below.

Note that the marker type of each **channels** entry is very sensitive for Summit.Grid algorithm, it must identical to chip images. In some case, the marker type may not set corrretly during scanning. To solve this problem, please see [this document](@ref doc/modify-markertype-in-exist-chiplog.md)

```json
{
  // camera width & height in micron
  "camera_h_size": 1122.38558707807,
  "camera_w_size": 1401.5324083661214,

  // channels used to done the chip scan task
  "channels": [
    {
      // camera shooting delay in milliseconds, used to prevent motor move damping
      "camera_delay_time": 1,

      // camera exposure time
      "exposure_time_abs": 250,

      // reader filter id, decide the light color
      "filter": 0,

      // camera gain value, usually 0
      "gain": 0,

      // marker type name(AM1/AM3/none)
      "marker_type": "none",

      // channel name
      "name": "BF_250us",

      // pixel format (Mono8/Mono14) related to image 8bits and 16bits
      "pixel_format": "Mono8"
    },
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 1000000,
      "filter": 4,
      "gain": 0,
      "marker_type": "AM3",
      "name": "CY5_1000ms",
      "pixel_format": "Mono14"
    },
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 250000,
      "filter": 2,
      "gain": 0,
      "marker_type": "AM1",
      "name": "CY3_250ms",
      "pixel_format": "Mono14"
    }
  ],

  // chip info specification
  "chip": {

    // autofocus paramters
    "af": {
      "convtol": 1e-06,
      "epsilon": 0.01,
      "kernel_scale": 0.0,
      "learning_rate": 0.1,
      "length_scale": 45.0,
      "margin": 0,
      "maxiter": 10000,
      "momentum": 0.9,
      "noise_level": 0.001,
      "search_range": 120,
      "show_per_n_iters": 100,
      "step_size": 30
    },

    // FOV description
    "fov": {
      // number of rows & columns
      "rows": 3,
      "cols": 3,

      // FOV distance in micron
      "w_d": 810,
      "h_d": 810,

      // shooting marker mask image
      "mask": "resource/banff_fov_mask.tif",
      "mask_src_type": "auto",

      // fov scan order
      "seq":[
          [[0, 0]], [[1, 0]], [[2, 0]],
          [[2, 1]], [[1, 1]], [[0, 1]],
          [[0, 2]], [[1, 2]], [[2, 2]]
      ],

      // the origin of chip in micron
      // defined by origin inference algorithm
      "x_i": -810,
      "y_i": -810
    },

    // chip info name
    "name": "banff33",

    // origin inference algorithm
    "origin_infer": {

      // algorithm tag
      "algo": "aruco_detection",

      // parameters
      "cell_size_px": 5,
      "db_key": "DICT_6X6_250",
      "loc_mk_layout_pt": [3,3],
      "nms_count": 9,
      "pyramid_level": 3
    },

    // chip specification
    "spec": {
      // chip name
      "name": "banff",

      // aruco marker description
      "aruco_marker": {

        // bit width in micron.
        "bit_w": 5.0,

        // boarder bit number
        "border_bits": 1,

        // aruco frame detection template&mask
        "frame_mask": "resource/banff/aruco_frame_mask.tiff",
        "frame_template": "resource/banff/aruco_frame_template.tiff",

        // aruco fringe bit number
        "fringe_bits": 1,

        // aruco marker margin size
        "margin_size": 3.0,

        // aruco marker detection minimum distance
        "nms_radius": 100.0,

        // aruco marker id on chip position mapping
        "id_map": {
          "0" : [0,6],
          "1" : [1,6],
          "10": [2,6],
          // etc
        }
      },

      // a probe cell size in micron
      "cell_h_um": 4,
      "cell_w_um": 4,

      // space between cell in micron
      "space_um": 1,

      // chip size in micron
      "h": 2480,
      "w": 2480,

      // chip size in grid cell count
      "h_cl": 496,
      "w_cl": 496,

      // location marker description, usually used in none aruco marker chip.
      // Not use in aruco marker chip scan
      "location_marker": {
        "w": 56,
        "h": 56,
        "mask": "resource/banff/aruco_frame_mask.tiff",
        "template": "resource/banff/aruco_frame_template.tiff"
      },

      // shooting marker description
      "shooting_marker": {
        // marker pattern images (deprecated, leave for downward compatible)
        "mk_pats": [
          {
            "path": "resource/banff/pat_2_68.tif",
            "um2px_r": 2.68
          },
          {
            "path": "resource/banff/pat_2_41.tif",
            "um2px_r": 2.41
          }
        ],

        // candidate marker pattern in cell level
        "mk_pats_cl": [
          {
            // marker type
            "marker_type": "AM1",

            // pattern file
            "path": "resource/banff/pat_AM1_a.tsv"
          },
          {
            "marker_type": "AM1",
            "path": "resource/banff/pat_AM1_b.tsv"
          },
          {
            "marker_type": "AM1",
            "path": "resource/banff/pat_AM1_c.tsv"
          },
          {
            "marker_type": "AM3",
            "path": "resource/banff/pat_AM3_a.tsv"
          },
          {
            "marker_type": "AM3",
            "path": "resource/banff/pat_AM3_d.tsv"
          },
          {
            "marker_type": "M/pT",
            "path": "resource/banff/pat_MpT_a.tsv"
          },
          {
            "marker_type": "M/pT",
            "path": "resource/banff/pat_MpT_c.tsv"
          }
        ],

        // origin position hint (for human not used in scan flow)
        "origin_desc": "center of the chip",

        // shooting marker position description in micron
        "position": {
          "row": 7,
          "col": 7,
          "w": 50,
          "h": 50,
          "w_d": 405,
          "h_d": 405,
          "x_i": -1240,
          "y_i": -1240
        },


        // shooting marker position description in grid cell level
        "position_cl": {
          "row": 7,
          "col": 7,
          "w": 10,
          "h": 10,
          "w_d": 81,
          "h_d": 81,
          "x_i": 0,
          "y_i": 0
        },

        // marker distribution on chip
        "type": "regular_matrix"
      }
    }
  },

  // chip id on tray
  "chip_id": 89,

  // horizontal overlap of FOV in micron
  "horizontal_overlap": 591.5324083661214,

  // vertical overlap of FOV in micron
  "vertical_overlap": 312.3855870780701,

  // image height in micron
  "image_height": 1122.38558707807,

  // image width in micron
  "image_width": 1401.5324083661214,

  // image prefix, unused
  "image_prefix": "",

  // is image encrypted
  "img_encrypted": false,

  // chip label descripted by user
  "label": "",

  // chip left top position in micron
  "location_marker": {
    "x": 41790.0,
    "y": 36354.0625
  },

  // mainboard version
  "mainboard_version": "0C.5.9",

  // scan start time point
  "scan_start": "2019/08/02 21:02:56",

  // scan end time point
  "scan_end": "2019/08/02 21:06:05",

  // is scan sucess?
  "scan_success": true,

  // reader serial number 
  "scanner_sn": "16000",

  // shooting marker location in physical coordinate micron
  "shoot_locs": [
    {
      "x": 40269.0625,
      "y": 34993.125,
      "z": 3266.5625
    },
    {
      "x": 41079.0625,
      "y": 34982.8125,
      "z": 3268.125
    },
    {
      "x": 41889.0625,
      "y": 34972.8125,
      "z": 3270.3125
    },
    {
      "x": 41899.0625,
      "y": 35782.8125,
      "z": 3274.375
    },
    {
      "x": 41089.375,
      "y": 35792.8125,
      "z": 3271.875
    },
    {
      "x": 40279.375,
      "y": 35803.125,
      "z": 3270.3125
    },
    {
      "x": 40289.375,
      "y": 36613.125,
      "z": 3271.875
    },
    {
      "x": 41099.375,
      "y": 36602.8125,
      "z": 3273.75
    },
    {
      "x": 41909.375,
      "y": 36592.5,
      "z": 3276.25
    }
  ],

  // shooting marker location in logical coordinate micron
  "shoot_locs_logic": [
    {
      "x": 38572.1875,
      "y": 29463.125,
      "z": 702.8125
    },
    {
      "x": 39382.1875,
      "y": 29450.9375,
      "z": 702.5
    },
    {
      "x": 40192.1875,
      "y": 29439.0625,
      "z": 702.8125
    },
    {
      "x": 40203.75,
      "y": 30249.0625,
      "z": 703.75
    },
    {
      "x": 39394.0625,
      "y": 30260.9375,
      "z": 703.125
    },
    {
      "x": 38584.0625,
      "y": 30273.125,
      "z": 703.4375
    },
    {
      "x": 38595.625,
      "y": 31083.125,
      "z": 701.875
    },
    {
      "x": 39405.625,
      "y": 31070.9375,
      "z": 701.875
    },
    {
      "x": 40215.625,
      "y": 31058.75,
      "z": 702.5
    }
  ],

  // shooting position on Z coordinate difference
  "zi_diff": 9.6875,

  // shooting position on Z coordinate max
  "zi_max": 3276.25,

  // shooting position on Z coordinate min
  "zi_min": 3266.5625,

  // daemon version
  "software_version": "1.14.6",

  // chip directory
  "top_level_dir": "D:\\summit_daemon_capture\\B1C-802-TW-6\\89_20190802210256",

  // micron to pixel rate
  "um_to_px_coef": 2.4145

}

```
