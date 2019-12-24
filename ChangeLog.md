# ChangeLog

## 1.1.1 - 2019.12.24

- Update: Make margin method configurable (the default method is "auto_min_cv")

## 1.1.0 - 2019.12.3

- Update: Add sig_est margin method (include local background subtraction)
- Update: Improve BF and FL marker detection process
- Update: Image debug level 2 -> 5
- Bugfix: Numerical type accuracy and chip specification mismatch 
  cause miss inference of grid lines by 1~2 pixel

## 1.0.11 - 2019.10.25

- Bugfix: debug flag effect the image process behavior
  - ArUco marker detection failed when debug level <= 1
- Bugfix: CEN file Channel ID not match specification

## 1.0.10 - 2019.9.17

- Update: light mean application output detail probe informations.

## 1.0.9 - 2019.9.11

- Update: ArUco matching update
- Source code: MSVC build support
  - Currently only support Visual Studio 2017 v15.9

## 1.0.8 - deprecated

## 1.0.7 - 2019.9.11

- Bugfix: no backgroud process workflow cell margin twice
- Feature: debug mode add margin view
- Feature: add raw stitch image output
- Feature: new ArUco matching algorithm import
- Develop update: Linux use new dynamic link solution

## 1.0.6 - 2019.8.16

- Bugfix: Crypto algorithm

## 1.0.5 - 2019.7.27

- Bugfix: None auto grid sometimes crash(marker append ROI bug)

## 1.0.4 - 2019.7.22

- Feature: Add white marker append
- Update: Update run batch script

## 1.0.3 - 2019.7.12

- Bugfix: BANFF chip crash (from 1.0.2)

## 1.0.2 - 2019.7.11

- Bugfix: ZION chip gridding failed (from 1.0.0)

## 1.0.1 - 2019.7.4

- Feature: Use micron distance to record the grid line position in grid log
- Bugfix: marker append FOV add boundary highlight

## 1.0.0 - 2019.7.1

- Feature: Formal grid log, include fov, channel and many parameters
- Feature: None auto grid mode, use modified grid log as input, summit.grid will re-generate all output
- Feature: Raw image generate in secure directory and grid/channels/*/viewable_raw
- Feature: debug mode remake, thread supported log
- Feature: Marker append image from 2x9x(3x3) => 2x9x9
- Feature: Remove FOV image generate in grid/channels/*/viewable_norm, only preserve the stitched image

## 0.9.6 - 2019.6.10

- Support M/pT marker type
- Add grid log into channel directory

## 0.9.5 - 2019.5.30

- Bugfix: Avoid one task fail, abort all tasks in mutli-task mode
- Bugfix: Avoid ArUco detector crash

## 0.9.4 - deprecated

## 0.9.3 - deprecated

## 0.9.2 - 2019.5.27

- ArUco support grdding algorithm: grid location

## 0.9.0 - 2019.5.27

- ArUco support grdding algorithm: rotation estimation, micron to pixel compute

## 0.8.6 - 2019.5.15

- Better viewable image auto contrast adjustor
- Light mean analysis add 3 row analysis
- Bugfix: Avoid multi-task mode consume all memory and crash
