@cd %~dp0
@if exist Build @rd /q /s Build
@mkdir Build
@cd Build
@cmake -G "Visual Studio 15 2017" -A "x64" ..