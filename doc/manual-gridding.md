
Manual gridding
===============

[TOC]

Overview
========

Due to the Summit.Grid auto-gridding algorithm is not perfect gridding algorithm, the user may want to fine-tune the gridding result. The Summit.Grid provide the manual gridding function for user to manually **optimize** the gridding result.

In this case, the user needs to run the auto-gridding algorithm first and use the grid_log.json to run the manual gridding.

Features
========

All features of manual gridding is based on editing the grid_log.json, the user should read the [specification of grid_log.json](@ref grid-log) carefully before start using these features.

Grid line position
------------------

The grid line position of the gridding result is mainly composed by 3 subjects:

* grid line interval
* grid origin
* micron-to-pixel rate

Modify the micron value in ```channels[i].fovs[j].du_<x/y>[k]``` can change the grid line interval of **a specific channel and FOV**.

While modify the pixel value ```channels[i].fovs[j].<x/y>0_px``` can change the first grid line position of **a specific channel and FOV**.

The modification of micron to pixel rate can be found [here](@ref modify-um-to-px-rate)

Rotation
--------

Modify ```rotation_degree``` in grid log can change the **chip** rotation degree.

Note that this update is chip-wide, which means all FOVs, channels will be effected.

Micron-to-pixel rate {#modify-um-to-px-rate}
--------------------------------------------

Modify ```um_to_pixel_rate``` in grid log can change the **chip** micron-to-pixel rate.

Note that this update is chip-wide, which means all FOVs, channels will be effected.
