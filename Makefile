MAJOR      = 0
MINOR      = 0
PATCH      = 1
BUILD      = 157

CXX     := x86_64-w64-mingw32-g++
WINDRES := x86_64-w64-mingw32-windres

# Flags shared by both the real gateway and the simulator builds.
COMMON_CXXFLAGS := -std=c++23 -O3 -flto=auto -march=x86-64-v3 \
                    -Wl,--no-dynamicbase -Wl,--no-high-entropy-va \
                    -fno-rtti \
                    -mwindows \
                    -static -static-libgcc -static-libstdc++
COMMON_LIBS     := -luser32 -lshell32 -ladvapi32 -lgdi32 -lws2_32 -ldwmapi \
                    -lwinmm -ldbghelp -lwinpthread -lpropsys -lole32 \
                    -lshlwapi -lwininet -lcomctl32 -luxtheme -lriched20 -lgdiplus

.PHONY: all sim clean push MAJOR MINOR PATCH BUILD release

all: bin/Trading-Floor.exe # bin/Trading-Floor-Simulator.exe

bin/Trading-Floor.exe: src/main.cpp lib/Trading-Floor-Assets.res lib/Trading-Floor-Gateway.a
	@echo -n "please wait, building $@.. "
	@rm -f $@
	@$(CXX) $^ \
	    $(COMMON_CXXFLAGS) \
	    $(COMMON_LIBS) \
		-s -o $@
	@echo "OK"
	@ls -la $@

bin/Trading-Floor-Simulator.exe: src/main.cpp lib/Trading-Floor-Assets.res lib/Trading-Floor-Simulator.a
	@echo -n "please wait, building $@.. "
	@rm -f $@
	@$(CXX) $^ \
	    -DGATEWAY_NAME='"Simulator"' \
	    $(COMMON_CXXFLAGS) \
	    $(COMMON_LIBS) \
		-s -o $@
	@echo "OK"
	@ls -la $@

lib/Trading-Floor-Assets.res: res/resources.rc
	@$(WINDRES) $^ -O coff -o $@

clean:
	@rm -f bin/Trading-Floor.exe bin/Trading-Floor-Simulator.exe lib/Trading-Floor-Assets.res
	@echo "Cleaned"

push:
	@date=`date` && (git diff || :) && git status && read -p "MOD: " MOD \
	&& git add . && git commit -S -m "$${MOD}"                           \
	&& (($(MAKE) all release && git push) || git reset HEAD^1)           \
	&& echo "\007" && echo $${date} && date

MAJOR:
	@sed -i "s/^\(MAJOR *=\).*$$/\1 $(shell expr $(MAJOR) + 1)/" Makefile
	@sed -i "s/^\(MINOR *=\).*$$/\1 0/"                          Makefile
	@sed -i "s/^\(PATCH *=\).*$$/\1 0/"                          Makefile
	@sed -i "s/^\(BUILD *=\).*$$/\1 0/"                          Makefile
	@$(MAKE) push

MINOR:
	@sed -i "s/^\(MINOR *=\).*$$/\1 $(shell expr $(MINOR) + 1)/" Makefile
	@sed -i "s/^\(PATCH *=\).*$$/\1 0/"                          Makefile
	@sed -i "s/^\(BUILD *=\).*$$/\1 0/"                          Makefile
	@$(MAKE) push

PATCH:
	@sed -i "s/^\(PATCH *=\).*$$/\1 $(shell expr $(PATCH) + 1)/" Makefile
	@sed -i "s/^\(BUILD *=\).*$$/\1 0/"                          Makefile
	@$(MAKE) push

BUILD:
	@sed -i "s/^\(BUILD *=\).*$$/\1 $(shell expr $(BUILD) + 1)/" Makefile
	@$(MAKE) push

release:
ifndef ZIPFILE
	@$(MAKE) ZIPFILE="Trading-Floor-$(MAJOR).$(MINOR).$(PATCH).$(BUILD)-win32.zip" $@
else
	zip -r $(ZIPFILE) bin lib res src README.md Makefile                \
	&& curl -s -n -H "Content-Type:application/zip" -H "Authorization: token ${TRADINGFLOOR}" \
	--data-binary "@$(PWD)/$(ZIPFILE)" "https://uploads.github.com/repos/ctubio/ibkr-gateway-trading-floor-private/releases/$(shell curl -s        \
	-H "Authorization: token ${TRADINGFLOOR}" https://api.github.com/repos/ctubio/ibkr-gateway-trading-floor-private/releases/latest | grep id | head -n1 | cut -d ' ' -f4 | cut -d ',' -f1 \
	)/assets?name=$(ZIPFILE)" && rm -v $(ZIPFILE) && (cd ../$(shell ls -r1 .. | tail -n1) && make git)
endif