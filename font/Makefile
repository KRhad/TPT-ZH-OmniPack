
editor: editor.c
	gcc -oeditor -DSCALE=1 -DFONTEDITOR editor.c fontdata.cpp `sdl2-config --cflags --libs` -lm -O3 -ffast-math -march=k8 -Wall -Wno-unused-result
	
editor.exe: editor.c
	i686-w64-mingw32-gcc -oeditor.exe -DSCALE=1 -DFONTEDITOR editor.c fontdata.cpp -lmingw32 -lm -lSDLmain -lSDL -O3 -ffast-math -march=k8 -Wall -mwindows -Wno-unused-result
	
packer: packer.c
	gcc -opacker -DFONTEDITOR packer.c fontdata.cpp -lm -O3 -ffast-math -march=k8 -Wall -Wno-unused-result
	
packer.exe: packer.c
	i686-w64-mingw32-gcc -opacker.exe -DFONTEDITOR packer.c fontdata.cpp -lm -O3 -ffast-math -march=k8 -Wall -Wno-unused-result
	
unpacker: unpacker.c
	gcc -ounpacker -DFONTEDITOR unpacker.c fontdata.cpp -lm -O3 -ffast-math -march=k8 -Wall -Wno-unused-result

unpacker.exe: unpacker.c
	i686-w64-mingw32-gcc -ounpacker.exe -DFONTEDITOR unpacker.c fontdata.cpp -lm -O3 -ffast-math -march=k8 -Wall -Wno-unused-result
	
clean:
	rm -f editor packer unpacker editor.exe packer.exe unpacker.exe
