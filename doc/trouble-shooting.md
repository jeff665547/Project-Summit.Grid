
Trouble shooting
================

[TOC]

Gridding failed
===============

Summit.Grid image process parameter is mainly based on Summit.Daemon scanning software, the Summit.Daemon should provide correct chip log and images to guarantee gridding algorithm is workable. However, there are several reasons that may cause Summit.Daemon generate wrong or non-optimized chip log and make gridding failed.

This section introduce the most frequent issues and solutions for user to diagnosis and fix the problem.

Known issues
------------

* [incorrect marker type in chip log](@ref doc/modify-markertype-in-exist-chiplog.md)
* [incorrect micron to pixel rate in chip log](@ref doc/incorrect-micron-to-pixel-rate-in-chiplog.md)
