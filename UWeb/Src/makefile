#=============================================================================
# UWeb makefile for Linux.
#
# Revision history:
# 	* Created by Brandon Reinhart 
#=============================================================================

# Unreal directory. (Required by makefile-header.)
UNREAL_DIR	= ../..

# Include global definitions.
include $(UNREAL_DIR)/makefile-header

#-----------------------------------------------------------------------------
# Directories.
#-----------------------------------------------------------------------------

SRC_DIRS	= $(UWEB_SRC)
INC_DIRS    = $(UWEB_INC)
BUILD_DIR	= $(UWEB_BUILD)

#-----------------------------------------------------------------------------
# Compiler/linker options.
#-----------------------------------------------------------------------------

CXX		= $(STD_CXX)
CXXFLAGS	= $(STD_CXXFLAGS) $(EXT_CXXFLAGS) -I$(UWEB_INC) \
			-I$(IPDRV_INC) -I$(ENGINE_INC) -I$(CORE_INC) \
			-DGPackage=GPackageWeb
LDFLAGS		= $(STD_LDFLAGS) $(EXT_LDFLAGS)$(UWEB_SONAME)
LIBS		= $(STD_LIBS) $(CORE)

#-----------------------------------------------------------------------------
# Files.
#-----------------------------------------------------------------------------

OBJS =			\
	UWeb.o		\
	WebServer.o	\
	UWebNative.o

OUT = $(UWEB)

#-----------------------------------------------------------------------------
# Rules.
#-----------------------------------------------------------------------------

# Use general rules.
include $(UNREAL_DIR)/makefile-general

#-----------------------------------------------------------------------------
# The End.
#-----------------------------------------------------------------------------
