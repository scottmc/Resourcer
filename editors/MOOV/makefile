## makefile for the MOOV (movie) editor add-on ##
## Uses the Haiku makefile-engine; normally run from the top level Makefile. ##
## When run on its own the editor ends up in the objects.* directory. ##

# specify the name of the binary
NAME = MOOV

# specify the type of binary
#	APP:	Application
#	SHARED:	Shared library or add-on
#	STATIC:	Static library archive
#	DRIVER: Kernel Driver
TYPE = SHARED

#	specify the source files to use
SRCS = MOOV.cpp \
	AudioOutput.cpp \
	DrawingTidbits.cpp \
	MediaSlider.cpp \
	MediaView.cpp \
	TransportButton.cpp

#	specify the resource definition files and resource files to use
RDEFS =
RSRCS =

#	specify additional libraries to link against
#	$(STDCPPLIBS) is "stdc++.r4" with gcc2 and "stdc++ supc++" with newer gcc
LIBS = be media root tracker $(STDCPPLIBS)

#	specify additional paths to directories following the standard
#	libXXX.so or libXXX.a naming scheme
LIBPATHS =

#	additional paths to look for system headers and local headers
SYSTEM_INCLUDE_PATHS =
LOCAL_INCLUDE_PATHS =

#	specify the level of optimization that you desire
#	NONE, SOME, FULL
OPTIMIZE =

#	specify any preprocessor symbols to be defined
DEFINES =

#	specify special warning levels (NONE, ALL)
WARNINGS =

#	specify whether image symbols will be created
SYMBOLS =

#	specify debug settings
DEBUGGER =

#	specify additional compiler and linker flags for all files
COMPILER_FLAGS =
LINKER_FLAGS =

## include the makefile-engine
include $(BUILDHOME)/etc/makefile-engine
