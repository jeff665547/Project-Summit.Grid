
Incorrect micron to pixel rate in chip log
===========================================

Before start
============

Before start this step, please read [basic chip log format](@ref input-spec.md) carefully.

Reason
======

For several versions updated, the Summit reader hardware has changed several times. These hardware updatings includes a camera lens change and this cause the image pixel to real-world micron rate changed.

This parameter should store in Summit reader firmware, but for some non-product releases, this parameter is missing updated.

To fix this problem, there are two things need to be done.

1. modify the chip log
2. modify the parameter store in reader firmware

Steps
=====

Modify the chip log
-------------------

Consider the entry ```um_to_px_coef``` in the chip log.
In old design this value is 2.68 but after camera lens specification changed, this value should be updated to 2.4145.

Please change this entry to 2.4145 if it is not.

Modify the parameter store in reader firmware
--------------------------------------------

To fix this problem once and for all,