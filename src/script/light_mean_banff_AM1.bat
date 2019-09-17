SET ROOT=%~dp0
%root%\..\bin\summit-app-light_mean.exe -i %* -o light_mean_AM1.csv -c banff -m AM1 -e light_mean_AM1_probes.csv

pause