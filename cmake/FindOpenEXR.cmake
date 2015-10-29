# - Find OpenEXR library
# Find the native OpenEXR includes and library
# This module defines
#  OPENEXR_INCLUDE_DIRS, where to find ImfXdr.h, etc. Set when
#                        OPENEXR_INCLUDE_DIR is found.
#  OPENEXR_LIBRARIES, libraries to link against to use OpenEXR.
#  OPENEXR_ROOT_DIR, The base directory to search for OpenEXR.
#                    This can also be an environment variable.
#  OPENEXR_FOUND, If false, do not try to use OpenEXR.
#
# For individual library access these advanced settings are available
#  OPENEXR_HALF_LIBRARY, Path to Half library
#  OPENEXR_IEX_LIBRARY, Path to Half library
#  OPENEXR_ILMIMF_LIBRARY, Path to Ilmimf library
#  OPENEXR_ILMTHREAD_LIBRARY, Path to IlmThread library
#  OPENEXR_IMATH_LIBRARY, Path to Imath library
#
# also defined, but not for general use are
#  OPENEXR_LIBRARY, where to find the OpenEXR library.

#=============================================================================
# Copyright 2011 Blender Foundation.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

# If OPENEXR_ROOT_DIR was defined in the environment, use it.
IF(NOT OPENEXR_ROOT_DIR AND NOT $ENV{OPENEXR_ROOT_DIR} STREQUAL "")
  SET(OPENEXR_ROOT_DIR $ENV{OPENEXR_ROOT_DIR})
ENDIF()

SET(_openexr_FIND_COMPONENTS
  Half
  Iex
  IlmImf
  IlmThread
  Imath
)


SET(_openexr_SEARCH_DIRS
  ${OPENEXR_ROOT_DIR}  
  /usr/local
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  "C:/Program Files (x86)/OpenEXR" # Windows
  "C:/Program Files (x86)/IlmBase" # Windows
)

FIND_PATH(OPENEXR_INCLUDE_DIR
  NAMES
    ImfXdr.h
  HINTS
    ENV CPATH
    ${_openexr_SEARCH_DIRS}
  PATH_SUFFIXES
    include/OpenEXR include OpenEXR
)

#message( "Include dir: ${OPENEXR_INCLUDE_DIR}" )

SET(_openexr_LIBRARIES)
FOREACH(COMPONENT ${_openexr_FIND_COMPONENTS})
  STRING(TOUPPER ${COMPONENT} UPPERCOMPONENT)

  FIND_LIBRARY(OPENEXR_${UPPERCOMPONENT}_LIBRARY
    NAMES
      ${COMPONENT}-2_2 ${COMPONENT}-2.2 ${COMPONENT}-2.1 ${COMPONENT}-2_1 ${COMPONENT}  # The newest version first
    HINTS
      ENV LIBRARY_PATH
      ${_openexr_SEARCH_DIRS}
    PATH_SUFFIXES
      lib64 lib ""
    )

#  message( "Lib: ${OPENEXR_${UPPERCOMPONENT}_LIBRARY}" )

#  IF( NOT "${OPENEXR_${UPPERCOMPONENT}_LIBRARY}" )
#	message( "Library ${COMPONENT} not found" )
#	SET( OPENEXR_FOUND "" )
#	break()
#  ENDIF()
	
  LIST(APPEND _openexr_LIBRARIES OPENEXR_${UPPERCOMPONENT}_LIBRARY)
  
ENDFOREACH()

# OpenEXR needs zlib, which could be tricky to locate on Windows
# You may need to manually do something like this:
#SET(ZLIB_DIR "e:/zlib/")
#SET(CMAKE_INCLUDE_PATH ${ZLIB_DIR}/include ${CMAKE_INCLUDE_PATH})
#SET(CMAKE_LIBRARY_PATH ${ZLIB_DIR}/ ${ZLIB_DIR}/lib ${CMAKE_LIBRARY_PATH})

FIND_PACKAGE(ZLIB REQUIRED)
LIST(APPEND _openexr_LIBRARIES ZLIB_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set OPENEXR_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)

#FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenEXR  DEFAULT_MSG  
#    ${_openexr_LIBRARIES} OPENEXR_INCLUDE_DIR)	

#  message( "${_openexr_LIBRARIES}" )
  
# This is a work-around as passing the list does not work in cmake 2.8.11.2 (cygwin)  
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenEXR  DEFAULT_MSG  
    OPENEXR_HALF_LIBRARY OPENEXR_IEX_LIBRARY OPENEXR_ILMIMF_LIBRARY OPENEXR_ILMTHREAD_LIBRARY OPENEXR_IMATH_LIBRARY ZLIB_LIBRARY OPENEXR_INCLUDE_DIR)
 	
IF(OPENEXR_FOUND)
  SET(OPENEXR_LIBRARIES ${OPENEXR_ILMTHREAD_LIBRARY} ${OPENEXR_IEX_LIBRARY} ${OPENEXR_ILMIMF_LIBRARY} ${OPENEXR_IMATH_LIBRARY} ${OPENEXR_HALF_LIBRARY} ${ZLIB_LIBRARY})
  # Both include paths are needed because of dummy OSL headers mixing #include <OpenEXR/foo.h> and #include <foo.h> :(
  SET(OPENEXR_INCLUDE_DIRS ${OPENEXR_INCLUDE_DIR} ${OPENEXR_INCLUDE_DIR}/OpenEXR ${ZLIB_INCLUDE_DIR})
ENDIF()

MARK_AS_ADVANCED(OPENEXR_INCLUDE_DIR)
FOREACH(COMPONENT ${_openexr_FIND_COMPONENTS})
  STRING(TOUPPER ${COMPONENT} UPPERCOMPONENT)
  MARK_AS_ADVANCED(OPENEXR_${UPPERCOMPONENT}_LIBRARY)
ENDFOREACH()
