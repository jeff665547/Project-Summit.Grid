SET ROOT=%~dp0
%root%\..\bin\summit-app-light_mean.exe -i %* -o light_mean_AM5B.csv -c yz01 -m AM5B -e light_mean_AM5B_probes.csv

pause