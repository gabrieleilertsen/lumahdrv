# - Find VPX library


SET(_vpx_SEARCH_DIRS
  /usr/local
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  "C:/Program Files (x86)" # Windows
)

if (WIN32 OR WIN64)
  SET(_vpx_SEARCH_DIRS
    "$_vpx_SEARCH_DIRS"
    "${PROJECT_SOURCE_DIR}/lib/libvpx"
    "${PROJECT_BINARY_DIR}"
  )
endif ()

FIND_PATH(VPX_INCLUDE_DIR
  NAMES
    vpx_codec.h
  HINTS
    ENV CPATH
    ${_vpx_SEARCH_DIRS}
  PATH_SUFFIXES
    include/vpx include vpx ""
)

FIND_LIBRARY( VPX_LIBRARY
    NAMES
      vpx vpxmt vpxmtd
    HINTS
      ENV LIBRARY_PATH
      ${_vpx_SEARCH_DIRS}
    PATH_SUFFIXES
      lib64 lib ""
    )


# handle the QUIETLY and REQUIRED arguments and set VPX_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(VPX  DEFAULT_MSG VPX_LIBRARY  VPX_INCLUDE_DIR)
 	
MARK_AS_ADVANCED(VPX_INCLUDE_DIR)
MARK_AS_ADVANCED(VPX_LIBRARY)
