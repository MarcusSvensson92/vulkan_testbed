@cd %~dp0
@if not exist Assets\Shaders @mkdir Assets\Shaders
@Tools\ShaderCompiler\ShaderCompiler.exe Source\Shaders\ Assets\Shaders\