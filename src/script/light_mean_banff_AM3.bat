SET ROOT=%~dp0
%root%\..\bin\summit-app-light_mean.exe -i %* -o light_mean_AM3.csv -c banff -m AM3 -e light_mean_AM3_probes.csv

pause