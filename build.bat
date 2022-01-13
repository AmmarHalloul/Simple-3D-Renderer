@echo off 

set compiler= clang-cl

set flags= /W0 /Od /Fo./obj/ /Fe./bin/RendererTest.exe /Fd./bin/ /Idep/include/
set files=  dep/src/glad.c  dep/src/lodepng.cpp dep/lib/glfw3dll.lib src/renderer.cpp src/main.cpp
set link_flags=  /incremental:no /opt:ref

%compiler% %flags% %files% /link %link_flags% && start bin/RendererTest.exe 
