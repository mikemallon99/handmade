@echo off

set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DHANDMADE_WIN32=1 -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 -FC -Z7 
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib Gdi32.lib winmm.lib

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

REM 32 bit build
REM cl %CommonCompilerFlags% ..\game\code\win32_handmade.cpp -Fmwin32_handmade.map /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64 bit build
del *.pdb > NUL 2> NUL
cl %CommonCompilerFlags% ..\game\code\handmade.cpp -Fmhandmade.map -LD /link -PDB:handmade_%random%.pdb -EXPORT:GameUpdateAndRender -EXPORT:GameGetSoundSamples
cl %CommonCompilerFlags% ..\game\code\win32_handmade.cpp -Fmwin32_handmade.map /link %CommonLinkerFlags%
popd
