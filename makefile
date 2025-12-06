imageviewer: imageviewer.c tinyfiledialogs.c
	gcc -Wall -Wextra -g -o imageviewer imageviewer.c tinyfiledialogs.c `sdl2-config --cflags --libs` -lm

run: imageviewer
	./imageviewer

clean:
	rm -f imageviewer