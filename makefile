imageviewer: imageviewer.c tinyfiledialogs.c
	gcc -Wall -Wextra -g -o imageviewer imageviewer.c tinyfiledialogs.c `sdl2-config --cflags --libs` -lm

ziggersons:
	zig cc imageviewer.c tinyfiledialogs.c -o imageviewer.exe -ID:\vcpkg-master\vcpkg-master\installed\x64-windows\include -LD:\vcpkg-master\vcpkg-master\installed\x64-windows\lib -lSDL2 -luser32 -lgdi32 -lshell32 -lm -lole32 -lcomdlg32 -luuid

run: imageviewer
	./imageviewer

clean:
	rm -f imageviewer