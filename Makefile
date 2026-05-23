CXX     := x86_64-w64-mingw32-g++
WINDRES := x86_64-w64-mingw32-windres

.PHONY: all clean

all: build/Trading-Floor.exe

build/Trading-Floor.exe: lib/Trading-Floor-Assets.res src/main.cpp
	@echo "please wait, building Trading-Floor.exe.."
	@rm -f build/Trading-Floor.exe
	@$(CXX) \
	    src/main.cpp \
	    lib/Trading-Floor-Assets.res \
	    lib/Trading-Floor-Gateway.a \
	    -std=c++17 \
	    -mwindows \
	    -static -static-libgcc -static-libstdc++ \
	    -luser32 -lshell32 -ladvapi32 -lgdi32 -lws2_32 -ldwmapi \
	    -lwinmm -ldbghelp -lwinpthread -lpropsys -lole32 \
	    -lshlwapi -lcomctl32 \
		-s -o build/Trading-Floor.exe
	@echo "Build Complete!"
	@ls -la build/Trading-Floor.exe

lib/Trading-Floor-Assets.res: resources/resources.rc
	@$(WINDRES) resources/resources.rc -O coff -o lib/Trading-Floor-Assets.res

clean:
	@rm -f build/Trading-Floor.exe lib/Trading-Floor-Assets.res
	@echo "Cleaned"
