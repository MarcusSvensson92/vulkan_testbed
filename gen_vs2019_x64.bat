@cd %~dp0
@if exist Build @rd /q /s Build
@mkdir Build
@cd Build
@cmake -G "Visual Studio 16 2019" -A "x64" ..