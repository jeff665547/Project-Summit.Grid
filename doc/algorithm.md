
Algorithm
=========

[TOC]

Overview
========

Summit.Grid is a program built on a library, [ChipImgProc](http://gitlab.centrilliontech.com.tw:10088/centrillion/ChipImgProc), combining some image processing and computer vision algorithms to deal with microarray images scanned from the SUMMIT. The discription below shows a brief introduction for the concept behind this project.

Image Gridding Process

1. Load chip images, parameters and configs.
2. Take the bright-field (BF, white channel) images or some fluorescent (probe) channel images as the reference channel for the entire gridding process.
3. Downsample the referenced-channel images and identify positions of each alignment marker (ArUco markers surrounded by rectangle markers).
4. Filter the unreasonable positions of the recognized markers and estimated the warp matrix for getting the relationship between the referenced-channel images and the theoretically rescaled GDS spec for the corresponding chip type.
5. For each remaining fluorescent channel, estimate the sub-pixel bias from the marker positions between that probe channel and the referenced channel via the referenced-channel warp matrix.ddd
6. Add the estimated bias to the referenced-channel warp matrix to get the true positions of fluorescent markers and probes under that probe channel due to the invariant of the rotation degree among referenced-channel images and remaining probe channel images.
7. By using the information from the updated warp matrix and the theoretical GDS spec, grid line can be known and can be mapped into any pixel and subpixel domain.
8. For each cell (feature/probe) in every FOV, find a fixed size rectangle whose coefficient of variation is the smallest to calculate the heatmap intensity (mean value of that rectangle) for the corresponding probe from the originally raw image.
9. Rotate and crop the tilt images. Use markers as anchers, and stitch all FOVs together to get the stitched chip image for downstream debug.
10. Output heatmap for downstream analysis, and all other images (marker-append images, stitched images, gridline images, etc.) for debug and shown only.

This project can be viewed as an application of the [ChipImgProc](http://gitlab.centrilliontech.com.tw:10088/centrillion/ChipImgProc). To get more details on the technical support of the algorithms, please refer to the API documentation of the [ChipImgProc](http://gitlab.centrilliontech.com.tw:10088/centrillion/ChipImgProc). The following section introduces how the automatically gridding feature actually works.

Automatically Gridding Workflow
===============================

The automatically gridding process of Summit.Grid is a data-driven, multi-level image processing.
It can be categorized into three levels further:

* Chip level process
* Channel level process
* FOV level process

Chip images will first 
Each level of processing has their own job to 

Chip level process {#chip-level-process}
------------------

The main goal of this process is to evaluate the *chip level* parameters: the warp matrix and some other parameters for each FOV.

This algorithm will try to estimate parameters from the white (Bright-Field (BF)) channel images with alignment (ArUco) markers. However, in some cases, if the BF channel images are not available, Summit.Grid program will estimate the parameters through the probe (fluorescent) channel images.

Several input data are required for this process, including:

* A complete chip directory:
  * Well-scanned chip images. (The alignment markers of images must be clear to view.)
  * chip log for the corresponding chip images.

It's worth noting that the chip log generated from the SUMMIT provides several important chip parameters. However, to get more accurate results, Summit.Grid will re-estimate some parameters based on the input images.

@startuml Chip Level Process
start
if (BF channel image process) then (success)
    :store the warp matrix.
else (failed)
    :currently not work, throw exception;
    stop
endif
:chip level parameter estimation done;
fork
    :Fluorescent Channel NO.1;
fork again
    :Fluorescent Channel NO.2;
fork again
    :...Fluorescent Channel NO.n;
end fork
note right
    Channel level process
end note
:collect grid log data;
:write the heatmap for the entire chip;
if (debug level) then ( >= 5 )
    :generate debug image data;
endif
stop
@enduml
  
References:

* [BF channel image process](@ref bf-channel-image-process)
* [channel level process](@ref channel-level-process)

BF channel image process {#bf-channel-image-process}
---------------------------

@startuml BF Channel Image Process

start
:read all BF images;
if(no BF images) then
    :return failed;
    stop
endif;
if(is ArUco marker images) then(do ArUco marker detecting process)
    :initialize ArUco marker detector;
    fork
        :detect ArUco markers that are randomly distributed;
        :estimate the warp matrix;
        :compute some statistics from the warp matrix;
        :use those statistics to secure bad FOVs;
        if (marker append option) then (is ON)
            :generate marker append images;
        endif;
    end fork
    :collect all FOV process results;
    :stitch white channel images;
    :return success;
    stop
else (do general marker detecting process)
    :initialize general marker detector;
    fork
        :detect general markers that are randomly distributed;
        :estimate the warp matrix;
        :compute some statistics from the warp matrix;
        if (marker append option) then (is ON)
            :generate marker append images;
        endif;
    end fork
    :collect all FOV process results;
    :stitch white channel image;
    :return success;
    stop
endif;

@enduml

See ChipImgProc:

* Bright-Field Marker Detection

Channel level process {#channel-level-process}
---------------------

In this level of process, the marker layout of each FOV is assumed to be the same, and all the FOV images will undergo the same process.

@startuml Channel Level Process

start
:parameters preparation;
fork
    :FOV image gridding;
    note right
        FOV level process
    end note
endfork
:collect FOV parameters;
:generate the multiple warped matrix;
:generate user-specified heatmap format data;
:generate the raw stitched image;
:generate gridline information;
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

@startuml Fov Level Process
start

while (each candidate fluorescent marker)
    :get the marker pattern image from txt file;
    :estimate the bias and compute the score of the fluorescent marker pattern recognition
     from the white channel marker position;
endwhile
:update the warp matrix with the estimated bias having the best score;
:generate the inverse warp matrix;
if (debug level) then ( >= 5 )
    :generate images used for debug;
endif
if (marker append option) then (is ON)
    :generate marker append images;
endif;
:make margin, mincv integrated warped matrix;
:set grid log and processed image;
stop
@enduml

See ChipImgProc for more information:

* Fluorescence Marker Detection