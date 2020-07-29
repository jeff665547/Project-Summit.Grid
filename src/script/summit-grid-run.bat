SET ROOT=%~dp0
%root%\..\bin\summit-app-grid.exe -o %* -i %* -r "csv_probe_list,html_probe_list" -d 7 --marker_append --no_bgp -n 4

pause