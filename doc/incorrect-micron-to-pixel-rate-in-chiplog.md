
Incorrect micron to pixel rate in chip log
===========================================

[TOC]

Before start
============

Before start this step, please read the [basic chip log format](@ref doc/input-spec.md) carefully.

Reason {#reason}
======

For several versions updated, the Summit reader hardware has changed several times. These hardware updatings includes a camera lens change and this cause the image pixel to real-world micron rate changed.

This parameter should store in Summit reader firmware, but for some non-product releases, this parameter is missing updated.

To fix this problem, there are two things need to be done.

1. modify the chip log
2. modify the parameter store in reader firmware

Steps
=====

Modify the chip log {#modify-the-chip-log}
-------------------

Consider the entry ```um_to_px_coef``` in the chip log.
In old design this value is 2.68 but after camera lens specification changed, this value should be updated to 2.4145.

Please change this entry to 2.4145 if it is not.

Modify the parameter store in reader firmware
---------------------------------------------

After doing [modify the chip log](@ref modify-the-chip-log), the chip should be ready to grid,
but as the [reason](@ref reason) says, the root cause is in firmware.

If the user only care about the current chip, this step is not required,
but note that this problem will still happen to other chips scanned from this Summit reader.

To fix this problem once and for all, the user need to update the firmware storage.

Consider the ```system.json``` in Summit.Daemon which locate at ```<reader-pkg>/daemon/etc/private/system.json```.
This file shows the current firmware *system storage* data, for example:

```json
{
  "cali_anchor_points": [
    {
      "x": 4468.75,
      "y": 114884.375,
      "z": 3473.75
    },
    {
      "x": 5053.125,
      "y": 15906.25,
      "z": 3932.1875
    },
    {
      "x": 68053.125,
      "y": 16256.25,
      "z": 4284.0625
    },
    {
      "x": 67471.875,
      "y": 115228.125,
      "z": 3812.1875
    }
  ],
  "camera_height": 1122.38558707807,
  "camera_width": 1401.532408366121,
  "filter_offset": 0,
  "led_cali_point": 70,
  "offset_origin_y": 16450
}
```

Consider the ```camera_height``` and ```camera_width``` entries, these 2 entries shows the FOV height and width in micron.
For production design, it should be 1122.38558707807 and 1401.532408366121, if not, please edit them carefully.

In general, editing system.json is useless, because when daemon bootup, the daemon will read the system data in firmware and overwrite the system.json.

To change this behavior, we should use another way to start the daemon at least one time:

1. Change directory to daemon location:

    ```cd <reader-pkg>\daemon\bin```
2. Start daemon with command:

    ```.\app-summit-daemon -r -c```
3. After several seconds, the daemon should be initialized done.
4. Close the console.
5. Start Magpie normally.
6. Check the system.json in ```<reader-pkg>\daemon\etc\private```, it should show correct ```camera_height``` and ```camera_width```