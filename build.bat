mkdir out
cd out
rm main.exe
cl ..\main.cc /I ..\include /link ..\lib\x86\SDL2.lib ..\lib\x86\SDL2main.lib ..\lib\x86\SDL2_ttf.lib /subsystem:windows
cp ..\lib\x86\SDL2.dll ..\lib\x86\SDL2_ttf.dll ..\lib\x86\libfreetype-6.dll ..\lib\x86\zlib1.dll .
cp ..\assets\consola.ttf .
cd ..
