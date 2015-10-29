# - Find EBML library


SET(_ebml_SEARCH_DIRS
  /usr/local
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  "C:/Program Files (x86)" # Windows
)

FIND_PATH(EBML_INCLUDE_DIR
  NAMES
    EbmlHead.h
  HINTS
    ENV CPATH
    ${_ebml_SEARCH_DIRS}
  PATH_SUFFIXES
    include/ebml include ebml ""
)

FIND_LIBRARY( EBML_LIBRARY
    NAMES
      ebml
    HINTS
      ENV LIBRARY_PATH
      ${_ebml_SEARCH_DIRS}
    PATH_SUFFIXES
      lib64 lib ""
    )


# handle the QUIETLY and REQUIRED arguments and set EBML_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EBML  DEFAULT_MSG EBML_LIBRARY  EBML_INCLUDE_DIR)
 	
MARK_AS_ADVANCED(EBML_INCLUDE_DIR)
MARK_AS_ADVANCED(EBML_LIBRARY)
