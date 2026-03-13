@echo off
setlocal

set "ROOT=%~dp0"
set "MODE=%~1"
set "GPP="
set "EXIT_CODE=0"

pushd "%ROOT%" >nul

if /I "%MODE%"=="build" (
    call :build
    set "EXIT_CODE=%ERRORLEVEL%"
    goto :end
)

if /I "%MODE%"=="run" (
    call :run
    set "EXIT_CODE=%ERRORLEVEL%"
    goto :end
)

if not "%MODE%"=="" (
    echo Usage: run.bat [build^|run]
    set "EXIT_CODE=1"
    goto :end
)

call :build
if errorlevel 1 (
    set "EXIT_CODE=%ERRORLEVEL%"
    goto :end
)

call :run
set "EXIT_CODE=%ERRORLEVEL%"
goto :end

:detect_gpp
if defined GPP exit /b 0

if exist "C:\Users\gp68\mingw64\bin\g++.exe" (
    set "GPP=C:\Users\gp68\mingw64\bin\g++.exe"
    exit /b 0
)

where /q g++.exe
if not errorlevel 1 (
    set "GPP=g++"
    exit /b 0
)

echo [ERROR] g++ not found. Install MinGW-w64 and add g++ to PATH.
echo [ERROR] Or edit run.bat and set a full path in GPP.
exit /b 1

:build
call :detect_gpp
if errorlevel 1 exit /b 1

if not exist "bin" mkdir "bin"

echo [Build] Compiling OpenGLRenderer...
"%GPP%" -fdiagnostics-color=always -g ^
    -DIMGUI_IMPL_OPENGL_LOADER_GLAD ^
    -DIMGUI_IMPL_GLFW_DISABLE_NATIVE_WINDOWS ^
    "%ROOT%src\Main.cpp" ^
    "%ROOT%src\Rendering\Shader.cpp" ^
    "%ROOT%src\Rendering\Texture.cpp" ^
    "%ROOT%src\Core\Camera.cpp" ^
    "%ROOT%src\Core\Window.cpp" ^
    "%ROOT%src\Lighting\Light.cpp" ^
    "%ROOT%src\Rendering\Framebuffer.cpp" ^
    "%ROOT%src\Rendering\DeferredRenderer.cpp" ^
    "%ROOT%src\Rendering\ShadowMap.cpp" ^
    "%ROOT%src\UI\ImGuiLayer.cpp" ^
    "%ROOT%src\Other\glad.c" ^
    "%ROOT%include\imgui\imgui.cpp" ^
    "%ROOT%include\imgui\imgui_draw.cpp" ^
    "%ROOT%include\imgui\imgui_tables.cpp" ^
    "%ROOT%include\imgui\imgui_widgets.cpp" ^
    "%ROOT%include\imgui\backends\imgui_impl_glfw.cpp" ^
    "%ROOT%include\imgui\backends\imgui_impl_opengl3.cpp" ^
    -I "%ROOT%include" ^
    -I "%ROOT%include\imgui" ^
    -L "%ROOT%lib" ^
    -lglfw3 ^
    -lgdi32 ^
    -lopengl32 ^
    -o "%ROOT%bin\main.exe"

if errorlevel 1 (
    echo [Build] Failed.
    exit /b 1
)

echo [Build] Success. Output: bin\main.exe
exit /b 0

:run
if not exist "%ROOT%bin\main.exe" (
    echo [Run] bin\main.exe not found. Run "run.bat build" first.
    exit /b 1
)

echo [Run] Launching OpenGLRenderer...
"%ROOT%bin\main.exe"
exit /b %ERRORLEVEL%

:end
popd >nul
endlocal & exit /b %EXIT_CODE%
