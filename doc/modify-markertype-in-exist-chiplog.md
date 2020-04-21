
Modify marker type in exist chip log
====================================

[TOC]

Before start
============

Before start this step, please read the [basic chip log format](@ref doc/input-spec.md) carefully.

Reason
======

The image process algorithm in Summit.Grid is marker type sensitive, but the marker type may be ignored by the user, because it doesn't cause problems in scanning.

Therefore, a post-processing to chip log for marker type correctness may required for some cases.

Steps
=====

Consider the channels in a chip log:

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

In this case, we have marker type and filter with following mapping

* filter = 0, marker type = none
* filter = 4, marker type = AM3
* filter = 2, marker type = AM1

We assume the marker type of filter 4 is not AM3 but Metal poly T, then we should find the channel with filter 4 and modify the marker type.

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

Now we can re-run the Summit.Grid for this chip.
