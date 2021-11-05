
Troubleshooting
================

[TOC]

Introduction
------------

According to the design of the latest Summit.Grid program, it's very hard to stuck and trigger some unexpected errors during the gridding process. Since this program have several rescuing mechanisms, users should pay more attention on the format of the input data and the final gridding results to check whether the processed image outputs are as expected. However, in this section, we will provide some basic troubleshooting techniques to help users conquer some common obstacles they could have encountered in the absence of instant assistances from the engineers. These techniques are used to exclude the problems that might be generated from the program itself, chip images, and other scanning device issues.

Problems
--------
If you think there is a problem with the Summit.Grid program, first check the following checklist sequentially. If the items below do not solve your problem, contact Centrillion Technologies Taiwan for a futher support.

| Problem           | Workaround      |
|-------------------|-----------------|
| Software version  | - Make sure the gridding program has been updated into the latest version. <br> - Open the terminal and chage the current directory to the one where the executable, summit-app-grid.exe, is located. <br> - Use the command ".\summit-app-grid.exe -v" to get the information of the current operating program. <br> - The showing version information should be the latest.|
| Input file format | - Make sure the chip_log.json and the image filename has the correct format for the corresponding chip type. <br> - The image filename must match the value of the "name" key in the chip_log.json ("channels" >> "name"). <br> - The name of the marker type should match the corresponding chip type. For more information, please refer to the marker_type_table.xlsx. |
| Bright-field (BF) input image quality | - Make sure the alignment markers (ArUco markers) is arranged as expected to the GDS spec and can be clearly viewed. <br> - The BF images can be successfully and correctly gridded. <br> - The captured area should not be out of borders of Field of View (FOV).| 
|Fluorescent input image quality | - Make sure the marker type for the fluorescent images is appropriatly and correctly set. <br> - The fluorescent marker patterns should be clear to view by eyes after the image contrast adjustment by the ImageJ. <br> - The fluorescent marker patterns should match the settings in the chip_log.json ("channels" >> "marker_type"). For modifying the chip_log.json, please refer to the [change the marker type in the chip_log](@ref doc/modify-markertype-in-exist-chiplog.md)|