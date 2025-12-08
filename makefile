imageviewer: imageviewer.c tinyfiledialogs.c
	gcc -Wall -Wextra -g -o imageviewer imageviewer.c tinyfiledialogs.c `sdl2-config --cflags --libs` -lm

windows: imageviewer.c tinyfiledialogs.c
	x86_64-w64-mingw32-gcc imageviewer.c tinyfiledialogs.c -o imageviewer.exe \
        -I/usr/local/mingw/SDL2/x86_64-w64-mingw32/include/SDL2 \
        -L/usr/local/mingw/SDL2/x86_64-w64-mingw32/lib \
        -lmingw32 -lSDL2main -lSDL2 -luser32 -lgdi32 -lshell32 -lm -lole32 -lcomdlg32 -luuid -mwindows

ziggersons: imageviewer.c tinyfiledialogs.c
	zig cc imageviewer.c tinyfiledialogs.c -o imageviewer.exe -ID:\vcpkg-master\vcpkg-master\installed\x64-windows\include -LD:\vcpkg-master\vcpkg-master\installed\x64-windows\lib -lSDL2 -luser32 -lgdi32 -lshell32 -lm -lole32 -lcomdlg32 -luuid

run: imageviewer
	./imageviewer

clean:
	rm -f imageviewer imageviewer.exe