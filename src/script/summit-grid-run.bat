SET ROOT=%~dp0
%root%\..\bin\summit-app-grid.exe -o output -i %* -r "tsv,html" -d 1

pause