SET ROOT=%~dp0
%root%\..\bin\summit-app-grid.exe -o output -i %* -r "csv_probe_list,html_probe_list" -d 0 --marker_append --no_bgp -n 2

pause