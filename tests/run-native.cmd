@echo off
setlocal
cd /d "%~dp0.."
for /f "usebackq delims=" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find VC\Auxiliary\Build\vcvars64.bat`) do call "%%i" >nul
if not exist .validation\screens mkdir .validation\screens
cl /nologo /std:c++17 /EHsc /W4 /DARDUINOJSON_ENABLE_ARDUINO_STRING=1 /I tests\stubs /I AIRQ /I "%USERPROFILE%\Documents\Arduino\libraries\Adafruit_GFX_Library" /I "%USERPROFILE%\Documents\Arduino\libraries\ArduinoJson\src" tests\regression.cpp AIRQ\StateMachine.cpp AIRQ\SensorManager.cpp AIRQ\DisplayManager.cpp AIRQ\PayloadBuilder.cpp AIRQ\Estados.cpp AIRQ\Telemetry.cpp AIRQ\DevWebOTA.cpp /Fe:.validation\regression.exe /Fo:.validation\ > .validation\native-build.log 2>&1
if errorlevel 1 (type .validation\native-build.log & exit /b 1)
.validation\regression.exe
