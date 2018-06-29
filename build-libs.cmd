rem @echo off

SET LIBRARY_DIRECTORY=%cd%/lib
SET BINARY_DIRECTORY=%cd%/bin

SET BUILD_ENVIRONMENT="Visual Studio 15 2017 Win64"
SET DEPENDENCIES=

cd ext

FOR %%A IN (glfw,glm,glbinding,globjects) DO CALL :Build %%A

goto End

:Build
cd "%1"
rd /S /Q build
md build
cd build
cmake .. -G %BUILD_ENVIRONMENT% -DCMAKE_CXX_FLAGS=/D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING -DCMAKE_INSTALL_PREFIX="%LIBRARY_DIRECTORY%/%1" -DCMAKE_PREFIX_PATH="%DEPENDENCIES%" -DCMAKE_DEBUG_POSTFIX=d

cmake --build . --config Debug --target install
md "%BINARY_DIRECTORY%/Debug"
robocopy "%LIBRARY_DIRECTORY%/%1" "%BINARY_DIRECTORY%/Debug" *d.dll

cmake --build . --config Release --target install
md "%BINARY_DIRECTORY%/Release"
robocopy "%LIBRARY_DIRECTORY%/%1" "%BINARY_DIRECTORY%/Release" *.dll /XF *d.dll

IF "%DEPENDENCIES%" == "" (
  SET DEPENDENCIES=%LIBRARY_DIRECTORY%/%1
) ELSE (
  SET DEPENDENCIES=%DEPENDENCIES%;%LIBRARY_DIRECTORY%/%1
)

cd ..
cd ..

GOTO :eof

:End
cd %~dp0
rem rd /S /Q ext