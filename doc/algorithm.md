
Algorithm
=========

[TOC]

Overview
========

Summit.Grid is a program built from the microarray chip image process library [ChipImgProc](http://gitlab.centrilliontech.com.tw:10080/centrillion/ChipImgProc).
To know the image process component algorithm. The users should reference the API documentation of ChipImgProc.

Auto gridding workflow
======================

Summit.Grid image process is a data-driven, multi-level process.
It mainly has 3 levels:

* Chip level
* Channel level
* FOV level

Chip level process {#chip-level-process}
------------------

The main target of this level is to evaluate the *chip wide* parameter: micron to pixel rate and chip rotation degree.

This algorithm will try to compute the parameter from the white channel with ArUco marker image, but in some case the chip doesn't include ArUco marker or scan task doesn't provide the white channel images.
In such cases, Summit.Grid will try to compute the chip wide parameters from the probe channel.

Before this step, several data should be prepared:

* complete chip directory
  * well-scanned chip images.
  * chip log

Note that, the chip log also provides several chip wide parameters, but these parameters are not accurate. This algorithm optimize these parameters.

@startuml {chip-level-process.png}
start
if (BF channel image process) then (success)
    :store warp matrix, rotate degree and white channel stitched image;
else (failed)
    :currently not work, throw exception;
    stop
endif
:chip-wide parameter estimate done;
fork
    :channel-1;
fork again
    :channel-2;
fork again
    :...channel-n;
end fork
note right
    channel level process
end note
:collect grid log data;
:write chip level heatmap;
if (debug level) then ( >= 5 )
    :generate debug data;
endif
stop
@enduml

References:

* [BF channel image process](@ref bf-channel-image-process)
* [channel level process](@ref channel-level-process)

BF channel image process {#bf-channel-image-process}
---------------------------

@startuml {bf-channel-image-process.png}

start
:read all BF channel images;
if(no BF channel images) then
    :return failed;
    stop
endif;
if(is ArUco marker images) then(do ArUco decoding)
    :initialize ArUco maker detector;
    fork
        :detect ArUco marker with random position;
        :estimate warp matrix;
        :estimate rotation degree;
        if (marker append option) then (is ON)
            :generate marker append images;
        endif;
    end fork
    :collect all FOV process result;
    :stitch white channel image;
    :return success;
    stop
else (do general marker decoding)
    :currently not work;
    :return failed;
    stop
endif;

@enduml

See ChipImgProc:

* Bright-Field Marker Detection

Channel level process {#channel-level-process}
---------------------

In this level process, Summit.Grid will assume each FOV has the same marker layout and doing the exactly same process to all FOVs.

@startuml {channel-level-process.png}

start
:generate FOV marker layout;
fork
    :FOV image gridding;
    note right
        FOV level process
    end note
endfork
:collect FOV result;
:generate multiple warped matrix;
:generate user-specified heatmap format data;
:generate the raw stitched image;
:generate gridline and background data;
if (marker append option) then (is ON)
    :generate marker append images;
endif;
stop

@enduml

Reference:

* [FOV image gridding](@ref fov-level-process)

See ChipImgProc:

* chipimgproc::marker::Layout
* Image Stitching

FOV level process {#fov-level-process}
-----------------

@startuml {fov-level-process.png}
start

while (each candidate probe marker)
    :read marker pattern into image;
    :estimate marker position bias and score from white channel marker position;
endwhile
:warp matrix add the bias with the best score;
:make margin, mincv integrated warped matrix;
:generate invert warp matrix;
:make debug related view;
if (marker append option) then (is ON)
    :generate marker append images;
endif;
:set grid log and processed image;
stop
@enduml

See ChipImgProc:

* Fluorescence Marker Detection