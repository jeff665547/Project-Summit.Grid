
# Summit.Grid gridding result specification

## grid log

### location

For Summit Grid run on Bamboolake, the grid log is in:

```sh
<chip directory>/grid/grid_log.json
```

### files

```json
{
  // Is the current result use auto gridding or manual?
  "auto_gridding": true,

  // Each channel result list
  "channels": [
    {

      // Each fov result in channel
      // The number of "fovs" is the shooting fov numbers of channel
      // For Banff, it is 9
      "fovs": [
        {

          // The distance in micron between each grid line on x coordinate
          // The number of elements is same as FOV contained grid lines.
          // For banff, the number is 171.
          "du_x": [
            // the micron level grid line distance
            4.992,
            4.992,
            4.992
            // etc.
          ],

          // Same concept as "du_x" but y coordinate
          "du_y": [
            4.992,
            4.992,
            4.992
            // etc.
          ],

          // A roughly gridding quality hint.
          // If the "grid_bad" is true, the result should nearly unusable
          // It means the program unable to detect enough markers on both white and probe channel
          "grid_bad": false,

          // This flag is false when the FOV gridding is interrupted by critical error. Otherwise, this flag should always be true.
          // If the this flag is false, the FOV result may missing some property,
          // please always check this flag before access the property
          "grid_done": true,

          // The FOV index of channel
          "id": [0, 0],

          // Specify the channel markers [probe_channel/white_channel] use to generate grid line
          "marker_region_source": "prboe_channel",

          // grid line x origin in pixel
          "x0_px": 641.0,

          // grid line y origin in pixel
          "y0_px": 326.0
        },
        {
            //...
        }
        // etc.
      ],

      // grid line position on full stitched image
      // The viewable stiched image is in:
      // <chip dir>/grid/channels/<channel name>/viewable_norm/stitch-<chname>.png
      // The raw stitched image is in:
      // <chip dir>/grid/channels/<channel name>/viewable_raw/stitch-<chname>.tiff
      // raw stitched image generation is provided after 1.0.7(included)
      "gl_x": [
        0,
        11,
        23,
        // ...
      ],
      "gl_y": [
        // ...
      ],

      // If one "grid_bad" flag of FOV in channel is true, then this flag is true
      "grid_bad": false,

      // If one "grid_done" flag of FOV in channel is true, then this flag is true
      "grid_done": true,

      // channel name
      "name": "CY5_1000ms"
    },

    // another channel of scan result
    {
        // ...
    }
    // etc.
  ],

  // If one "grid_done" flag of channel in chip is true, then this flag is true
  "grid_done": true,

  // The raw image input chip directory
  "input": "E:\\20190816-npcall-data\\B1C-802-TW-6\\90_20190802210607_mincv",

  // The raw image input chip directory, same as input
  "chip_dir": "E:\\20190816-npcall-data\\B1C-802-TW-6\\90_20190802210607_mincv",

  // Marker append image result is generate or not.
  // If true, the result should generated in <output>/marker_append/
  "marker_append": false,

  // The gridding result heatmap is including background process or not
  // If false, the background process is inlucded
  "no_bgp": true,

  // Output directory of gridding
  "output": "E:\\20190816-npcall-data\\B1C-802-TW-6\\90_20190802210607_mincv",

  // Output heatmap formats
  "output_formats": [
    "csv_probe_list"
  ],

  // Gridding process time in second
  "proc_time": 69.44400024414063,

  // Chip rotation degree
  "rotate_degree": -0.43091893196105957,

  // The secure directory path (used by bamboo lake, not necessary for image process)
  "secure_dir": "",

  // The shared directory path (used by bamboo lake, not necessary for image process)
  "shared_dir": "",

  // micron to pixel rate
  // pixel value = micron value * <um_to_pixel_rate>
  "um_to_pixel_rate": 2.4015774726867676,
  
  // The white channel is detected and processed in gridding flow
  "white_channel_proc": true

}

```
