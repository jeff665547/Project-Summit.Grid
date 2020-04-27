
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
    :consensus rotation and micron to pixel rate;
else (failed)
    :select one probe channel;
    :probe channel image process(parameter estimate);
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
* [probe channel image process](@ref probe-channel-image-process)
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
        :iterative rotation search;
        note right
            use ArUco marker detector
        end note
        :estimate accurate micron to pixel rate;
        if (marker append option) then (is ON)
            :generate marker append images;
        endif;
    end fork
    :collect all FOV process result;
    :return success;
    stop
else (do general marker decoding)
    if(has supported marker?) then (no)
        :return failed;
        stop
    endif;
    :select middle(or near middle) FOV;
    :iterative rotation search;
    note right
        use regular matrix marker detection;
    end note
    :micron to pixel rate auto-scale;
    :collect micron to pixel rate and rotation degree;
    :return success;
    stop
endif;

@enduml

References:

* [iterative rotation search](@ref iterative-rotation-search)
* [micron to pixel auto-scale](@ref micron-to-pixel-auto-scale)

See ChipImgProc:

* Fluorescence Marker Detection
* Bright-Field Marker Detection

Probe channel image process(parameter estimate) {#probe-channel-image-process}
-----------------------------------------------

@startuml {probe-channel-image-process.png}
start
:select middle(or near middle) FOV;
:iterative rotation search;
:micron to pixel rate auto-scale;
:collect micron to pixel rate and rotation degree;
:return success;
stop
@enduml

References:

* [iterative rotation search](@ref iterative-rotation-search)
* [micron to pixel auto-scale](@ref micron-to-pixel-auto-scale)

See ChipImgProc:

* Fluorescence Marker Detection

Iterative rotation search {#iterative-rotation-search}
-------------------------

Iterative rotation search is a utility function wildly used by Summit.Grid to do rotation detection.

@startuml {iterative-rotation-search-algorithm.png}

start
while(iterate times < 6 or last step rotation degree > 0.01)
    :marker detection;
    note right
        This marker detection algorithm is swapable,
        i.e. callback function.
    end note
    :estimate rotation degree from marker position;
    note right
        This rotation degree is "step" rotation degree.
        The real rotation degree is the summation of all step rotation degree.
    end note
    :store current rotation degree to a list;
    note right
        The storing rotation degree is the real rotation degree.
        There are many ways to implement this value.
        ChipImgProc package use 2 variable to implement this algorithm.
    end note
    :rotate the image by current rotation degree;
endwhile
if(last step rotation degree > 0.01) then(not convergence)
    :return the mean of degree list;
    stop
else (convergence)
    :return final rotation degree;
    stop
endif;

@enduml

See ChipImgProc:

* Image Rotation Angle Estimation and Calibration

Micron to pixel auto-scale {#micron-to-pixel-auto-scale}
--------------------------

This is a micron-to-pixel rate optimization algorithm, which require at least one not bad micron-to-pixel rate and this value should provide by Summit reader.

This is an iterative approach, which require some parameters e.g. step size, iteration times etc., here we use step size = 0.002 and iteration time = 7

@startuml {micron-to-pixel-auto-scale}
start
:assume the best score is 0;
while(not reach the iteration time limit)
    :make marker layout from current micron to pixel rate;
    :use marker layout to do the template matching;
    :search the matching point on score matrix;
    if (current score > best score ) then(yes)
        :overwrite the best score by current score;
        :store current micron to pixel rate;
    endif;
    :increase micron to pixel rate by step size;
endwhile
stop
@enduml

Channel level process {#channel-level-process}
---------------------

In this level process, Summit.Grid assume each FOV has the same marker layout and doing the exactly same process to all FOVs.

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
:generate user-specified heatmap format data;
:generate the raw stitched image;
:generate gridline and background data;
if (marker append option) then (is ON)
    :generate marker append images;
endif;
stop

@enduml

Reference:

* [FOV image gridding](#fov-level-process)

See ChipImgProc:

* chipimgproc::marker::Layout
* Image Stitching

FOV level process {#fov-level-process}
-----------------

@startuml {fov-level-process.png}
start
:probe channel marker detection;
note right
    assume regular matrix distribution with rotate
end note
:rotate detection;
partition set-gridding-marker {
    if(abs(probe channel - chip rotation degree) > 0.5) then (probe marker detection failed)
        if(white markers exist) then
            :set white channel;
        else
            :abort, grid failed;
            end
        endif;
    else (probe marker detection success)
        if(white markers exist) then
            :compute pixel per grid cell;
            :use 1/2 cell size as **threshold**;
            if(abs(probe maker - white marker position) > **threshold** ) then (probe marker detection failed)
                :set probe channel;
            else(probe marker detection success)
                :set white channel;
            endif;
        else
            :set probe channel;
        endif;
    endif;
}
:marker based gridding;
:generate the raw FOV images;
:compute background surface(BSpline algorithm);
if(background process) then (is ON)
    :subtract background value;
endif;
:grid cell margin and do statistic;
:set grid log and processed image;

stop
@enduml

See ChipImgProc:

* Fluorescence Marker Detection
* Image Rotation Angle Estimation and Calibrationn
