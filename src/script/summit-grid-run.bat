SET ROOT=%~dp0
%root%\..\bin\summit-app-grid.exe -o output -i %* -r "tsv,html" -d 0 --marker_append --no_bgp -n 2

pause