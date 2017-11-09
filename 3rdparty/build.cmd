@echo off

cd %~dp0
cd ..
SET LIBRARY_DIRECTORY=%cd%/lib
SET BINARY_DIRECTORY=%cd%/bin
cd %~dp0

SET BUILD_ENVIRONMENT="Visual Studio 14 2015 Win64"
SET DEPENDENCIES=

FOR %%A IN (glfw,glm,glbinding,globjects) DO CALL :Build %%A

goto End

:Build
cd %1
rd /S /Q build
md build
cd build
cmake .. -G %BUILD_ENVIRONMENT% -DCMAKE_INSTALL_PREFIX=%LIBRARY_DIRECTORY%/%1 -DCMAKE_PREFIX_PATH=%DEPENDENCIES% -DCMAKE_DEBUG_POSTFIX=d

cmake --build . --config Debug --target install
md %BINARY_DIRECTORY%/Debug
robocopy %LIBRARY_DIRECTORY%/%1 %BINARY_DIRECTORY%/Debug *d.dll

cmake --build . --config Release --target install
md %BINARY_DIRECTORY%/Release
robocopy %LIBRARY_DIRECTORY%/%1 %BINARY_DIRECTORY%/Release *.dll /XF *d.dll

IF "%DEPENDENCIES%" == "" (
  SET DEPENDENCIES=%LIBRARY_DIRECTORY%/%1
) ELSE (
  SET DEPENDENCIES=%DEPENDENCIES%;%LIBRARY_DIRECTORY%/%1
)

cd %~dp0

:End
  
  