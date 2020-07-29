
Chip directory spliter specification
====================================

當前狀態與使用者需求
==================

Summit.Daemon 目前產生的資料結構大略為:

```
<RFID>\
    <Chip dir>\
        <row>-<col>-<channel>.tiff
        chip_log.json
```

而 Summit.Grid 則是以 chip directory 為一個單位處理影像，並且要求不得有兩個以上的 channel 使用相同的 filter。

使用者的需求是排程化自動使用同一個 filter 並且不同曝光時間掃描，故按照 Daemon 當前規格
其產生的 chip directory 將有如下可能

```
<RFID>\
    <chip dir>\
        0-0-green-AM1-500ms.tiff
        0-0-green-AM1-1000ms.tiff
        0-0-green-AM1-2000ms.tiff
        ...
        chip_log.json
```

範例中可以觀察到 FOV 0,0 的影像用 filter 2 (green) 掃描了數次

其 chip log 內容將會是

```
{
  "camera_h_size": 1122.38558707807,
  "camera_w_size": 1401.532408366121,
  "channels": [
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 250,
      "filter": 0,
      "gain": 0,
      "marker_type": "none",
      "name": "BF",
      "pixel_format": "Mono8"
    },
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 500000,
      "filter": 2,
      "gain": 0,
      "marker_type": "AM1",
      "name": "green-AM1-500ms",
      "pixel_format": "Mono14"
    },
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 1000000,
      "filter": 2,
      "gain": 0,
      "marker_type": "AM1",
      "name": "green-AM1-1000ms",
      "pixel_format": "Mono14"
    },
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 2000000,
      "filter": 2,
      "gain": 0,
      "marker_type": "AM1",
      "name": "green-AM1-2000ms",
      "pixel_format": "Mono14"
    },
    ...
  ],
  "chip": {
      ...
  }
}
```


Chip directory spliter 需求規格
==============================

基本功能概觀
-----------

使用者輸入
---------

* 一個 chip directory 路徑
* 一個 channel 拆分列表 
    * ex: [[green-AM1-500ms, red-AM3-500ms], [green-AM1-1000ms, red-AM3-1000ms]]
    * 外層描述拆分後各個 chip diretory 所持有之 channel 組
    * 內層描述單個 chip directory 中 channel 組所包涵的 channel 內容
    * 若內層 channel 描述依然有相同的 filter 或者存在本來不存在之 channel，程式應予以阻止
    * 白光(filter = 0) 之 channel, 程式默認複製並記錄到所有產生出來的 chip directory 和 chip log.
    * 實作不強迫使用完全與範例相同之描述，但應使邏輯結構保持一致。

程式行為
-------

Daemon 掃描結果如下範例:

Chip directory:

```
<RFID>\
    <chip dir>\
        0-0-green-AM1-500ms.tiff
        0-0-red-AM3-500ms.tiff
        0-0-green-AM1-2000ms.tiff
        0-0-red-AM3-2000ms.tiff
        0-0-BF.tiff
        chip_log.json
```

chip log:

```
{
  "camera_h_size": 1122.38558707807,
  "camera_w_size": 1401.532408366121,
  "channels": [
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 250,
      "filter": 0,
      "gain": 0,
      "marker_type": "none",
      "name": "BF",
      "pixel_format": "Mono8"
    },
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 500000,
      "filter": 2,
      "gain": 0,
      "marker_type": "AM1",
      "name": "green-AM1-500ms",
      "pixel_format": "Mono14"
    },
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 2000000,
      "filter": 2,
      "gain": 0,
      "marker_type": "AM1",
      "name": "green-AM1-2000ms",
      "pixel_format": "Mono14"
    },
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 500000,
      "filter": 4,
      "gain": 0,
      "marker_type": "AM3",
      "name": "red-AM3-500ms",
      "pixel_format": "Mono14"
    },
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 2000000,
      "filter": 4,
      "gain": 0,
      "marker_type": "AM3",
      "name": "red-AM3-2000ms",
      "pixel_format": "Mono14"
    }
  ],
  "chip": {
      ...
  }
}
```

User 輸入之 chip directory channel list:

```
[[green-AM1-500ms, red-AM3-500ms], [green-AM3-1000ms, red-AM3-1000ms]]
```

經過處理之後應為:

Chip directories:

```
<RFID>\
    <chip dir-gen0>\
        0-0-green-AM1-500ms.tiff
        0-0-red-AM3-500ms.tiff
        0-0-BF.tiff
        chip_log.json
    <chip dir-gen1>\
        0-0-green-AM1-2000ms.tiff
        0-0-red-AM3-2000ms.tiff
        0-0-BF.tiff
        chip_log.json
```

gen0 chip log:

```
{
  "camera_h_size": 1122.38558707807,
  "camera_w_size": 1401.532408366121,
  "channels": [
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 250,
      "filter": 0,
      "gain": 0,
      "marker_type": "none",
      "name": "BF",
      "pixel_format": "Mono8"
    },
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 500000,
      "filter": 2,
      "gain": 0,
      "marker_type": "AM1",
      "name": "green-AM1-500ms",
      "pixel_format": "Mono14"
    },
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 500000,
      "filter": 4,
      "gain": 0,
      "marker_type": "AM3",
      "name": "red-AM3-500ms",
      "pixel_format": "Mono14"
    }
  ],
  "chip": {
      ...
  }
}
```

gen1 chip log:

```
{
  "camera_h_size": 1122.38558707807,
  "camera_w_size": 1401.532408366121,
  "channels": [
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 250,
      "filter": 0,
      "gain": 0,
      "marker_type": "none",
      "name": "BF",
      "pixel_format": "Mono8"
    },
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 2000000,
      "filter": 2,
      "gain": 0,
      "marker_type": "AM1",
      "name": "green-AM1-2000ms",
      "pixel_format": "Mono14"
    },
    {
      "camera_delay_time": 1,
      "exposure_time_abs": 2000000,
      "filter": 4,
      "gain": 0,
      "marker_type": "AM3",
      "name": "red-AM3-2000ms",
      "pixel_format": "Mono14"
    }
  ],
  "chip": {
      ...
  }
}
```

gen0, gen1 為範例使用之區隔標籤，實作可以依真實情況自由執行。