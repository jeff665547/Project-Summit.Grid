
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
if (while channel image process) then (success)
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

White channel image process {#white-channel-image-process}
---------------------------

@startuml {white-channel-image-process.png}

start
:read all white channel images;
:initialize ArUco maker detector;
fork
    while(iterate times < 6 or last rotation degree step > 0.01)
        :ArUco detection;
        :estimate degree;
    endwhile
    :estimate accurate micron to pixel rate;
    if (marker append option) then (is ON)
        :generate marker append images;
    endif;
end fork
note right
    for each FOV parallel process
end note
:collect all FOV process result;
stop

@enduml

Probe channel image process(parameter estimate)
-----------------------------------------------

This step is similar to the white channel image process, except use probe channel marker detection instead of ArUco marker detection.

Channel level process {@channel-level-process}
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
:generate user specified heatmap format data;
:generate raw-stitched image;
:generate gridline and background data;
if (marker append option) then (is ON)
    :generate marker append images;
endif;
stop

@enduml

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
:generate raw FOV images;
:compute background surface(BSpline algorithm);
if(background process) then (is ON)
    :subtract background value;
endif;
:grid cell margin and do statistic;
:set grid log and processed image;

stop
@enduml
