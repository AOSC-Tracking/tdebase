#Macro to find xscreensaver directory

# Copyright (c) 2006, Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
# got from kde4

if (NOT XSCREENSAVER_FOUND)
  set(xscreensaver_alldirs)
  set(xscreensaver_checkdirs
    /usr/
    /usr/local/
    /opt/local/
    /usr/X11R6/
    /opt/kde/
    /opt/kde3/
    /usr/kde/
    /usr/share/xscreensaver/
    /usr/local/kde/
    /usr/local/xscreensaver/
    /usr/local/share/xscreensaver/
    /usr/openwin/lib/xscreensaver/
    /etc/
  )
  foreach(suffix lib${LIB_SUFFIX}/xscreensaver lib${LIB_SUFFIX}/misc/xscreensaver
                 lib/xscreensaver lib64/xscreensaver libexec/xscreensaver
                 bin/xscreensaver-hacks hacks)
     foreach(xscreensaver_path ${xscreensaver_checkdirs} )
        set(xscreensaver_alldirs ${xscreensaver_alldirs} ${xscreensaver_path}/${suffix})
     endforeach(xscreensaver_path ${xscreensaver_checkdirs} )
  endforeach()

  set(XSCREENSAVER_DIRS)
  FIND_PATH(XSCREENSAVER_DIR_DECO deco ${xscreensaver_alldirs})
  FIND_PATH(XSCREENSAVER_DIR_FLUX flux ${xscreensaver_alldirs})
  if(XSCREENSAVER_DIR_DECO)
    list(APPEND XSCREENSAVER_DIRS ${XSCREENSAVER_DIR_DECO})
  endif(XSCREENSAVER_DIR_DECO)
  if(XSCREENSAVER_DIR_FLUX)
    list(APPEND XSCREENSAVER_DIRS ${XSCREENSAVER_DIR_FLUX})
  endif(XSCREENSAVER_DIR_FLUX)
  list( REMOVE_DUPLICATES XSCREENSAVER_DIRS )
  string( REPLACE ";" ":" XSCREENSAVER_DIRS "${XSCREENSAVER_DIRS}" )

  set(XSCREENSAVER_CONFIG_DIRS)
  FIND_PATH(XSCREENSAVER_CONFIG_DECO config/deco.xml ${xscreensaver_checkdirs} )
  FIND_PATH(XSCREENSAVER_CONFIG_FLUX config/flux.xml ${xscreensaver_checkdirs} )
  if(XSCREENSAVER_CONFIG_DECO)
    list(APPEND XSCREENSAVER_CONFIG_DIRS "${XSCREENSAVER_CONFIG_DECO}/config/")
  endif(XSCREENSAVER_CONFIG_DECO)
  if(XSCREENSAVER_CONFIG_FLUX)
    list(APPEND XSCREENSAVER_CONFIG_DIRS "${XSCREENSAVER_CONFIG_FLUX}/config/")
  endif(XSCREENSAVER_CONFIG_FLUX)

  # Try and locate XScreenSaver config when path doesn't include config
  if(NOT XSCREENSAVER_CONFIG_DIRS)
    FIND_PATH(XSCREENSAVER_CONFIG_DIRS deco.xml
      /etc/xscreensaver
      )
  endif(NOT XSCREENSAVER_CONFIG_DIRS)

  list( REMOVE_DUPLICATES XSCREENSAVER_CONFIG_DIRS )
  string( REPLACE ";" ":" XSCREENSAVER_CONFIG_DIRS "${XSCREENSAVER_CONFIG_DIRS}" )
endif(NOT XSCREENSAVER_FOUND)

#MESSAGE(STATUS "XSCREENSAVER_CONFIG_DIR :<${XSCREENSAVER_CONFIG_DIR}>")
#MESSAGE(STATUS "XSCREENSAVER_DIR :<${XSCREENSAVER_DIR}>")

# Need to fix hack
if(XSCREENSAVER_DIRS AND XSCREENSAVER_CONFIG_DIRS)
	set(XSCREENSAVER_FOUND TRUE)
endif(XSCREENSAVER_DIRS AND XSCREENSAVER_CONFIG_DIRS)

if (XSCREENSAVER_FOUND)
  if (NOT Xscreensaver_FIND_QUIETLY)
    message(STATUS "Found SCREENSAVER_CONFIG_DIRS <${XSCREENSAVER_CONFIG_DIRS}>")
  endif (NOT Xscreensaver_FIND_QUIETLY)
else (XSCREENSAVER_FOUND)
  if (Xscreensaver_FIND_REQUIRED)
    message(FATAL_ERROR "XScreenSaver not found")
  endif (Xscreensaver_FIND_REQUIRED)
endif (XSCREENSAVER_FOUND)

MARK_AS_ADVANCED(XSCREENSAVER_DIRS XSCREENSAVER_CONFIG_DIRS)
