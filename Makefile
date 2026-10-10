# Resourcer - top level Makefile
#
# Running plain `make` here builds everything into ./build:
#
#   build/Resourcer         the application
#   build/editors/<TYPE>    one add-on per resource type (TEXT, ICON, WIND, ...)
#   build/ResourceLib       helper library for apps that read their own resources
#
# Each component keeps its own makefile (they all use the Haiku
# makefile-engine); this file only decides what gets built and where the
# results go.  Resourcer looks for its editors in an "editors" folder next to
# the application, so build/Resourcer can be run straight from the build folder.
#
# Handy targets:
#   make                 build everything
#   make app             just the application
#   make editors         all the editors
#   make editor-TEXT     a single editor
#   make reslib          just ResourceLib
#   make clean           remove all build output
#   make help            print this list
#
# Only basic GNU make features are used so this works with the make that
# ships with the gcc2 (x86_gcc2) builds of Haiku as well as newer ones.
# The compiler itself is picked by the makefile-engine, so the usual
# setgcc / hybrid gcc2 setup decides what gets used.

# The makefile-engine lives in $(BUILDHOME)/etc.  Haiku normally sets
# BUILDHOME for us; fall back to asking the system if it is missing.
ifeq ($(strip $(BUILDHOME)),)
BUILDHOME := $(shell finddir B_SYSTEM_DEVELOP_DIRECTORY 2>/dev/null)
endif
ifeq ($(strip $(BUILDHOME)),)
$(error BUILDHOME is not set and could not be found with finddir. \
This build needs the Haiku development tools)
endif
export BUILDHOME

BUILD_DIR   := $(CURDIR)/build
EDITORS_OUT := $(BUILD_DIR)/editors

# Editors built from a single source file, editors/<TYPE>.cpp,
# using the shared editors/makefile.
SINGLE_FILE_EDITORS := \
	ALGN AMTX APPF APPV BOOL BPNT BYTE CHAR CSTR CURS DBLE FLOT ICON LLNG \
	LONG MICN MIME MIMS MSGG OFFT PATN RECT RGBC SHRT SIZE SIZT SSZT TEXT \
	TIME UBYT ULLG ULNG USHT bits nois unknown

# Editors that have a directory and makefile of their own: editors/<TYPE>/
MULTI_FILE_EDITORS := MOOV WIND

SINGLE_EDITOR_TARGETS := $(addprefix editor-,$(SINGLE_FILE_EDITORS))
MULTI_EDITOR_TARGETS  := $(addprefix editor-,$(MULTI_FILE_EDITORS))

# Every directory that has a makefile, used for cleaning.
MAKEFILE_DIRS := main reslib editors \
	$(addprefix editors/,$(MULTI_FILE_EDITORS))

.PHONY: all app reslib editors test clean help \
	$(SINGLE_EDITOR_TARGETS) $(MULTI_EDITOR_TARGETS)

all: app editors reslib

app:
	@mkdir -p "$(BUILD_DIR)"
	$(MAKE) -C main TARGET_DIR="$(BUILD_DIR)"

reslib:
	@mkdir -p "$(BUILD_DIR)"
	$(MAKE) -C reslib TARGET_DIR="$(BUILD_DIR)"

editors: $(SINGLE_EDITOR_TARGETS) $(MULTI_EDITOR_TARGETS)

$(SINGLE_EDITOR_TARGETS): editor-%:
	@mkdir -p "$(EDITORS_OUT)"
	$(MAKE) -C editors EDITOR_NAME=$* TARGET_DIR="$(EDITORS_OUT)"

$(MULTI_EDITOR_TARGETS): editor-%:
	@mkdir -p "$(EDITORS_OUT)"
	$(MAKE) -C editors/$* TARGET_DIR="$(EDITORS_OUT)"

# Compiles the sample resource file used for testing. It holds one resource
# of most of the types Resourcer has editors for. Not part of "make all".
test: $(BUILD_DIR)/test.rsrc

$(BUILD_DIR)/test.rsrc: tests/test.rdef
	@mkdir -p "$(BUILD_DIR)"
	rc -o "$@" tests/test.rdef

clean:
	@for d in $(MAKEFILE_DIRS); do \
		$(MAKE) -C $$d clean || exit 1; \
	done
	rm -rf "$(BUILD_DIR)"

help:
	@echo "Targets:"
	@echo "  all (default)   build the application, all editors and ResourceLib"
	@echo "  app             build the application only"
	@echo "  editors         build all editors"
	@echo "  editor-<TYPE>   build one editor, e.g. editor-TEXT"
	@echo "  reslib          build ResourceLib only"
	@echo "  test            compile tests/test.rdef into $(BUILD_DIR)/test.rsrc"
	@echo "  clean           remove all build output"
	@echo "Results end up in $(BUILD_DIR)"
