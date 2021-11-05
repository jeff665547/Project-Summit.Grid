
Modify the marker type in the chip log
======================================

[TOC]

Introduction
------------

For the Summit.Grid program, the setting of the marker type is very pivotal especially when the BF images are not available. Users should double-check the gridding results to prevent getting the wrong intensity values. In this section, we will introduce how to set the marker type correctly if needed. Before we start to adjust the parameters, it's recommended to read the [basic chip log format](@ref doc/input-spec.md) first carefully.

Example
-------

Suppose that we have following parameter settings in some chip_log.json:

```json
{
  ...
  "channels": [
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 250,
      "filter": 0,
      "gain": 0,
      "marker_type": "none",
      "name": "BF_250us",
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
  ...
}
```

From the above configuration, the relationships between the marker type and the filter are as the following:

* filter = 0, marker type = none
* filter = 4, marker type = AM3
* filter = 2, marker type = AM1

We assume the actual marker type of the fluorescnet images captured under the filter four does not meet the parameter settings in the chip_log.json. For example, the actual marker type is not the AM3 but the metal poly T. In this case, we should find the key "marker_type" under the channel with filter four and modify its value (AM3) to the M/pT.

```json
{
  ...
  "channels": [
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 250,
      "filter": 0,
      "gain": 0,
      "marker_type": "none",
      "name": "BF_250us",
      "pixel_format": "Mono8"
    },
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 1000000,
      "filter": 4,
      "gain": 0,
      "marker_type": "M/pT",
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
  ...
}
```

Now we can run the Summit.Grid program again with these new settings to check the new gridding results.
