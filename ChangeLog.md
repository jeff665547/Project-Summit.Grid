# ChangeLog

## 1.1.22 - 2020.7.14

- Bugfix: light mean crash and naming.

## 1.1.21 - 2020.7.8

- Bugfix: L1B ArUco detection failed.

## 1.1.20 - 2020.6.30

- Bugfix: Fix runtime crash.

## 1.1.19 - 2020.6.22

- Bugfix: Fix COMPLETE file not generated after image gridding problem in several cases.

## 1.1.18 - 2020.6.17

- Bugfix: Fix COMPLETE file not generated after image gridding problem in several cases.
- Feature: Logger add flush.
- Dev: Show whether the log is enabled in build message.

## 1.1.17 - 2020.6.16

- Bugfix: Chip log fixer unable to process L1B SP.
- Feature: Convenience batch runner for chip log fixer.

## 1.1.16 - 2020.6.10

- Bugfix: Chip log fixer not work in several cases.

## 1.1.15 - 2020.6.9

- Feature: All chip types use the formal marker.

## 1.1.14 - 2020.6.4

- Bugfix: K1B marker size error, grid result doesn't fit the spec.

## 1.1.13 - 2020.6.3

- Bugfix: White channel general marker detection failed.

## 1.1.12 - 2020.5.28

- Bugfix: db_key move.

## 1.1.11 - 2020.5.27

- Bugfix: Image gridding failed when no probe image.

## 1.1.10 - 2020.5.22

- Feature: L2C support.

## 1.1.9 - 2020.5.21

- Feature: L1B full support.

## 1.1.8 - 2020.5.21

- Bugfix: Lassen spec missing parameter.

## 1.1.7 - 2020.5.20

- Feature: Lassen support (BF still not work)
- Feature: yz01 1/4 support

## 1.1.6 - 2020.4.23

- Bugfix: Give channel merge failed case an error message.

## 1.1.5 - 2020.4.23

- Feature: Kenai able to use white channel gridding.
- Bugfix: Image processing failed exit without error message.

## 1.1.4 - 2020.4.14

- Feature: More stitched image.

## 1.1.3 - 2020.4.14

- Feature: Add debug grid stiched image. All channels stacked into a multi-channel image(png).
  - Use option "-d 5" and the result will generate in "grid" directory.
  - The color mapping is follow the channel filter:
    - 0 -> blue
    - 2 -> green
    - 4 -> red

## 1.1.2 - 2020.3.19

- Update: Use raw channel name as CEN file channel name, instead of channel-0/1.
- Feature: Add Kenai support.
- Feature: Background subtraction use Spline algorithm.

## 1.1.1.1 - 2020.1.22

- Update: Add marker region to grid log

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
- Bugfix: CEN file Channel ID not match the specification

## 1.0.10 - 2019.9.17

- Update: light mean application output detail probe information.

## 1.0.9 - 2019.9.11

- Update: ArUco matching update
- Source code: MSVC build support
  - Currently only support Visual Studio 2017 v15.9

## 1.0.8 - deprecated

## 1.0.7 - 2019.9.11

- Bugfix: no background process workflow cell margin twice
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

- Feature: Formal grid log, include FOV, channel and many parameters
- Feature: None auto grid mode, use modified grid log as input, summit.grid will re-generate all output
- Feature: Raw image generate in the secure directory and grid/channels/*/viewable_raw
- Feature: debug mode remake, thread supported log
- Feature: Marker append image from 2x9x(3x3) => 2x9x9
- Feature: Remove FOV image generate in grid/channels/*/viewable_norm, only preserve the stitched image

## 0.9.6 - 2019.6.10

- Support M/pT marker type
- Add grid log into channel directory

## 0.9.5 - 2019.5.30

- Bugfix: Avoid one task fail, abort all tasks in multi-task mode
- Bugfix: Avoid ArUco detector crash

## 0.9.4 - deprecated

## 0.9.3 - deprecated

## 0.9.2 - 2019.5.27

- ArUco support gridding algorithm: grid location

## 0.9.0 - 2019.5.27

- ArUco support gridding algorithm: rotation estimation, micron to pixel compute

## 0.8.6 - 2019.5.15

- Better viewable image auto contrast adjustor
- Light mean analysis add 3 row analysis
- Bugfix: Avoid multi-task mode consume all memory and crash
