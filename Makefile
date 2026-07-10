MAJOR      = 0
MINOR      = 0
PATCH      = 0
BUILD      = 18

CXX     := x86_64-w64-mingw32-g++
WINDRES := x86_64-w64-mingw32-windres

.PHONY: all clean push MAJOR MINOR PATCH BUILD release 

all: bin/Trading-Floor.exe

bin/Trading-Floor.exe: lib/Trading-Floor-Assets.res src/main.cpp
	@echo -n "please wait, building Trading-Floor.exe.. "
	@rm -f bin/Trading-Floor.exe
	@$(CXX) \
	    src/main.cpp \
	    lib/Trading-Floor-Assets.res \
	    lib/Trading-Floor-Gateway.a \
	    -std=c++23 -Wl,--no-dynamicbase -Wl,--no-high-entropy-va \
	    -mwindows \
	    -static -static-libgcc -static-libstdc++ \
	    -luser32 -lshell32 -ladvapi32 -lgdi32 -lws2_32 -ldwmapi \
	    -lwinmm -ldbghelp -lwinpthread -lpropsys -lole32 \
	    -lshlwapi -lwininet -lcomctl32 -luxtheme -lriched20 -lgdiplus \
		-s -o bin/Trading-Floor.exe
	@echo "OK"
	@ls -la bin/Trading-Floor.exe

lib/Trading-Floor-Assets.res: res/resources.rc
	@$(WINDRES) res/resources.rc -O coff -o lib/Trading-Floor-Assets.res

clean:
	@rm -f bin/Trading-Floor.exe lib/Trading-Floor-Assets.res
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
	--data-binary "@$(PWD)/$(ZIPFILE)" "https://uploads.github.com/repos/ctubio/ibkr-gateway-trading-floor/releases/$(shell curl -s        \
	https://api.github.com/repos/ctubio/ibkr-gateway-trading-floor/releases/latest | grep id | head -n1 | cut -d ' ' -f4 | cut -d ',' -f1 \
	)/assets?name=$(ZIPFILE)" && rm -v $(ZIPFILE)
endif
