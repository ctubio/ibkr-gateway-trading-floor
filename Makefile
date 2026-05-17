PROJECT_ROOT := $(shell pwd)
IBKR_CLIENT  := lib/CppClient/client
BID_SRC      := $(PROJECT_ROOT)/lib/IntelRDFPMathLib20U4/LIBRARY/src
PROTO_BUILD  := $(PROJECT_ROOT)/lib/build/protobuf_win
PROTO_INSTALL:= $(PROJECT_ROOT)/lib/build/protobuf_install
ABSEIL_SRC   := $(PROJECT_ROOT)/lib/protobuf/third_party/abseil-cpp
UTFRANGE_SRC := $(PROJECT_ROOT)/lib/protobuf/third_party/utf8_range
OBJ_DIR_BID  := $(PROJECT_ROOT)/build/obj_bid
OBJ_DIR_IBKR := $(PROJECT_ROOT)/build/obj_ibkr

ABSL_LIBS  := $(wildcard $(PROTO_INSTALL)/lib/libabsl_*.a)
PROTO_LIBS := $(wildcard $(PROTO_INSTALL)/lib/libproto*.a)
UTF8_LIBS  := $(wildcard $(PROTO_INSTALL)/lib/libutf8*.a)

CXX     := x86_64-w64-mingw32-g++
CC      := x86_64-w64-mingw32-gcc
AR      := x86_64-w64-mingw32-ar
RANLIB  := x86_64-w64-mingw32-ranlib
WINDRES := x86_64-w64-mingw32-windres

# ─────────────────────────────────────────────────────────────────────────────

.PHONY: all lib clean

all: build/TNT.exe

# ─── BUILD EXE ───────────────────────────────────────────────────────────────

build/TNT.exe: lib/build/resources.res src/main.cpp \
               lib/build/libibkr.a lib/build/libbid.a
	@echo "please wait, building TNT.exe.."
	@rm -f build/TNT.exe
	@$(CXX) \
	    src/main.cpp \
	    lib/build/resources.res \
	    -I$(IBKR_CLIENT) \
	    -I$(IBKR_CLIENT)/protobuf \
	    -I$(PROTO_INSTALL)/include \
	    -Llib/build \
	    -L$(PROTO_INSTALL)/lib \
	    -Wl,--start-group \
	    -libkr \
	    $(PROTO_LIBS) \
	    $(ABSL_LIBS) \
	    $(UTF8_LIBS) \
	    -lbid \
	    -Wl,--end-group \
	    -std=c++17 \
	    -mwindows \
	    -static -static-libgcc -static-libstdc++ \
	    -luser32 -lshell32 -ladvapi32 -lgdi32 -lws2_32 -ldwmapi \
	    -lwinmm -ldbghelp -lwinpthread -lpropsys -lole32 \
	    -lshlwapi -lcomctl32 \
	    -s \
	    -o build/TNT.exe
	@rm -f lib/build/resources.res
	@echo "Build Complete!"
	@ls -la build/TNT.exe

lib/build/resources.res: resources/resources.rc
	@$(WINDRES) resources/resources.rc -O coff -o lib/build/resources.res

# ─── BUILD LIBS ──────────────────────────────────────────────────────────────

lib: lib/build/libbid.a lib/build/libibkr.a

lib/build/libbid.a:
	@echo "Compiling Intel Decimal Floating Point (libbid)..."
	@mkdir -p $(OBJ_DIR_BID)/fenv_override
	@printf '%s\n' \
	    '#ifndef _BID_FENV_OVERRIDE_H' \
	    '#define _BID_FENV_OVERRIDE_H' \
	    'typedef unsigned long fexcept_t;' \
	    'typedef unsigned long fenv_t;' \
	    '#define FE_INVALID    1' \
	    '#define FE_DIVBYZERO  4' \
	    '#define FE_OVERFLOW   8' \
	    '#define FE_UNDERFLOW  16' \
	    '#define FE_INEXACT    32' \
	    '#define FE_ALL_EXCEPT (FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT)' \
	    '#define FE_TONEAREST  0' \
	    '#define FE_DOWNWARD   1024' \
	    '#define FE_UPWARD     2048' \
	    '#define FE_TOWARDZERO 3072' \
	    'int fegetround(void);' \
	    'int fesetround(int);' \
	    'int feclearexcept(int);' \
	    'int feraiseexcept(int);' \
	    'int fetestexcept(int);' \
	    'int fegetexceptflag(fexcept_t*,int);' \
	    'int fesetexceptflag(const fexcept_t*,int);' \
	    'int fegetenv(fenv_t*);' \
	    'int fesetenv(const fenv_t*);' \
	    'int feupdateenv(const fenv_t*);' \
	    'int feholdexcept(fenv_t*);' \
	    '#endif' \
	    > $(OBJ_DIR_BID)/fenv_override/fenv.h
	@cd $(OBJ_DIR_BID) && \
	    $(CC) -c \
	    $(shell find $(BID_SRC) -maxdepth 1 -name "*.c" ! -name "wcstod*" ! -name "strtod*") \
	    -I$(OBJ_DIR_BID)/fenv_override \
	    -I$(BID_SRC) \
	    -DWINDOWS -DCALL_BY_REF=0 -DGLOBAL_RND=0 -DGLOBAL_FLAGS=0 \
	    -DUNCHANGED_BINARY_FLAGS=0 -DUSE_COMPILER_F128_TYPE=0 \
	    -DUSE_COMPILER_F80_TYPE=0 -O2
	@$(AR) rcs $@ $(OBJ_DIR_BID)/*.o
	@$(RANLIB) $@
	@rm -rf $(OBJ_DIR_BID)
	@echo "libbid.a done"

$(PROTO_INSTALL)/lib/libprotobuf.a:
	@echo "Building Protobuf + Abseil via CMake..."
	@rm -rf $(PROTO_BUILD) $(PROTO_INSTALL)
	@mkdir -p $(PROTO_BUILD)
	@printf '%s\n' \
	    'set(CMAKE_SYSTEM_NAME Windows)' \
	    'set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)' \
	    'set(CMAKE_C_COMPILER $${TOOLCHAIN_PREFIX}-gcc)' \
	    'set(CMAKE_CXX_COMPILER $${TOOLCHAIN_PREFIX}-g++)' \
	    'set(CMAKE_RC_COMPILER $${TOOLCHAIN_PREFIX}-windres)' \
	    'set(CMAKE_FIND_ROOT_PATH /usr/$${TOOLCHAIN_PREFIX})' \
	    'set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)' \
	    'set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)' \
	    'set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)' \
	    'set(BUILD_SHARED_LIBS OFF)' \
	    > $(PROJECT_ROOT)/mingw-toolchain.cmake
	@cd $(PROTO_BUILD) && cmake $(PROJECT_ROOT)/lib/protobuf \
	    -DCMAKE_TOOLCHAIN_FILE=$(PROJECT_ROOT)/mingw-toolchain.cmake \
	    -DCMAKE_BUILD_TYPE=Release \
	    -DCMAKE_INSTALL_PREFIX=$(PROTO_INSTALL) \
	    -Dprotobuf_BUILD_TESTS=OFF \
	    -Dprotobuf_BUILD_SHARED_LIBS=OFF \
	    -Dprotobuf_ABSL_PROVIDER=module \
	    -Dprotobuf_BUILD_PROTOC_BINARIES=OFF \
	    -DCMAKE_CXX_STANDARD=17 && \
	    cmake --build . -j$(shell nproc) && \
	    cmake --install .
	@rm -f $(PROJECT_ROOT)/mingw-toolchain.cmake
	@echo "Protobuf + Abseil done"

lib/build/libibkr.a: $(PROTO_INSTALL)/lib/libprotobuf.a
	@echo "Compiling IBKR SDK objects..."
	@mkdir -p $(OBJ_DIR_IBKR)
	@find $(PROJECT_ROOT)/$(IBKR_CLIENT) \( -name "*.cpp" -o -name "*.cc" \) | \
	    while IFS= read -r f; do \
	        $(CXX) -c "$$f" \
	            -I$(PROJECT_ROOT)/$(IBKR_CLIENT) \
	            -I$(PROJECT_ROOT)/$(IBKR_CLIENT)/protobuf \
	            -I$(PROTO_INSTALL)/include \
	            -I$(ABSEIL_SRC) \
	            -I$(UTFRANGE_SRC) \
	            -std=c++17 -O2; \
	    done
	@$(AR) rcs $@ $(OBJ_DIR_IBKR)/*.o
	@$(RANLIB) $@
	@rm -rf $(OBJ_DIR_IBKR)
	@echo "libibkr.a done"

# ─── CLEAN ───────────────────────────────────────────────────────────────────

clean:
	@rm -f build/TNT.exe lib/build/resources.res
	@echo "Cleaned"

clean_lib:
	@rm -f lib/build/libbid.a lib/build/libibkr.a
	@rm -rf lib/build/protobuf_win lib/build/protobuf_install
	@echo "Cleaned libs"
