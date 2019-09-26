@echo off

where cl 2> NUL 1>NUL
if %ERRORLEVEL% neq 0 call dev

set INCLUDE_DIRS=
set LIBRARIES=

set CPP_FLAGS_DEBUG=/Od /DDEBUG /Zi
set CPP_FLAGS_REL=/O2 /Oi

set CPP_FLAGS=/nologo /MD /diagnostics:column /D_CRT_SECURE_NO_WARNINGS /WL /GR- /EHa- /W4 /wd4996 /wd4706 %INCLUDE_DIRS% 
set LD_FLAGS=%LIBRARIES%

mkdir build 2> NUL

pushd build\

copy ..\src\*.png 2>NUL 1>NUL
copy ..\src\*.jpeg 2>NUL 1>NUL
copy ..\src\*.jpg 2>NUL 1>NUL

cl %CPP_FLAGS% %CPP_FLAGS_REL% ..\src\main.cpp /FeDeepFry /link %LD_FLAGS%

popd

call run
