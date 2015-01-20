REM "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv" /x86 /debug
REM cl /Zi /Od test.cpp
cl injector.cpp  /link psapi.lib ws2_32.lib
cl /D_USRDLL /D_WINDLL cheat_dll.cpp /link user32.lib ws2_32.lib psapi.lib /DLL /OUT:cheat.dll