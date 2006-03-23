#
# Copyright 2003, 2004 Andrew De Ponte.
#
# This file is part of zsrep.
#
# zsrep is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# any later version.
#
# zsrep is distributed in the hopes that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Generel Public License for details.
#
# You should have received a copy of the GNU General Public License
# along with zsrep; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307 USA
#

TODOPLUGIN_OBJ = KOrgTodoPlugin.o
TODOPLUGIN_SRC = KOrgTodoPlugin.cc

KDE3_INC = /opt/kde3/include
QT3_INC = /usr/lib/qt3/include
KDE3_LIB = /opt/kde3/lib
QT3_LIB = /usr/lib/qt3/lib

# This is the plugins output file name.
TODOPLUGIN_OUT_FILENAME = KOrgTodoPlugin.so
# A series of all the object files used to create the ZMSG library.
TODOPLUGIN_OBJS = $(TODOPLUGIN_OBJ)

TODOPLUGIN_LIB_FLAG = -L$(KDE3_LIB) -L$(QT3_LIB) -lzdata -lconfmgr -lkcal -lkdecore
TODOPLUGIN_INC_FLAG = -I$(KDE3_INC) -I$(QT3_INC)

# Remove command
RM = rm -rf

# The compiler and its options.
COMPILER = g++
# The compile only (do not link) flag.
COMPILE_FLAG = -c
# Position Independent Code Flag.
PIC_FLAG = -fPIC
# Enable debug code within compile.
DEBUG_FLAG =
# The warnings control flag.
WARNING_FLAG = -Wall
# The flag used to specify the file name to use for output.
OUTPUT_FLAG = -o
# The flag used to specify the SONAME of a library when creating
# a shared library.
SONAME_FLAG = -shared -Wl,-soname,

#####################################################################
# No user configuration should occur below this line.
#####################################################################

all : $(TODOPLUGIN_OUT_FILENAME)

# Create the shared library.
$(TODOPLUGIN_OUT_FILENAME) : $(TODOPLUGIN_OBJS)
	$(COMPILER) $(DEBUG_FLAG) $(SONAME_FLAG)$(TODOPLUGIN_OUT_FILENAME).0 $(OUTPUT_FLAG) $(TODOPLUGIN_OUT_FILENAME) $(TODOPLUGIN_LIB_FLAG) $(TODOPLUGIN_OBJS)

# Here we create the zdata shared object file.
$(TODOPLUGIN_OBJ) : $(TODOPLUGIN_SRC)
	$(COMPILER) $(PIC_FLAG) $(WARNING_FLAG) $(DEBUG_FLAG) $(COMPILE_FLAG) $(TODOPLUGIN_INC_FLAG) $(TODOPLUGIN_SRC)

install :
	mkdir -p /usr/local/lib/zync/plugins/todo/
	cp $(TODOPLUGIN_OUT_FILENAME) /usr/local/lib/zync/plugins/todo/

# Here we get rid of the files that we created.
clean :
	$(RM) $(TODOPLUGIN_OUT_FILENAME) $(TODOPLUGIN_OBJS)
