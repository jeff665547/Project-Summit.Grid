
Algorithm
=========

Overview
========

Summit.Grid is a program built from the microarray chip image process library [ChipImgProc](http://gitlab.centrilliontech.com.tw:10080/centrillion/ChipImgProc). To know the image process component algorithm. The users should reference the API documentation of ChipImgProc.

Auto gridding workflow
======================

Summit.Grid image process is a data-driven, multi-level process.
It mainly has 3 levels:

* Chip level
* Channel level
* FOV level

Chip level process
------------------

@startuml
start
if (while channel image process) then (success)
    :consensus rotation and micron to pixel rate;
else (failed)
    :select one probe channel;
    :probe channel image process(parameter estimate);
endif
:parallel process all probe channels;
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
stop
@enduml

White channel image process
---------------------------

@startuml

start
:read all white channel images;
:initialize ArUco maker detector;
:parallel process for each white FOV image;
fork
    while(iterate times < 6 or last rotation degree step > 0.01)
        :ArUco detection;
        :estimate degree;
    endwhile
    :estimate accurate micron to pixel rate;
    :generate marker append images(if needed);
end fork
:collect all FOV process result;
stop

@enduml

Probe channel image process(parameter estimate)
-----------------------------------------------

This step is similar to the white channel image process, except use probe channel marker detection instead of ArUco marker detection.

Channel level process
---------------------

@startuml

start
:generate FOV marker layout;
:parallel process for each FOV image;
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
:generate marker append images(if needed);
stop

@enduml

FOV level process
-----------------

@startuml



@enduml