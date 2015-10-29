# - Find Matroska library


SET(_matroska_SEARCH_DIRS
  /usr/local
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  "C:/Program Files (x86)" # Windows
)

FIND_PATH(MATROSKA_INCLUDE_DIR
  NAMES
    FileKax.h
  HINTS
    ENV CPATH
    ${_matroska_SEARCH_DIRS}
  PATH_SUFFIXES
    include/matroska include matroska ""
)

FIND_LIBRARY( MATROSKA_LIBRARY
    NAMES
      matroska
    HINTS
      ENV LIBRARY_PATH
      ${_matroska_SEARCH_DIRS}
    PATH_SUFFIXES
      lib64 lib ""
    )


# handle the QUIETLY and REQUIRED arguments and set MATROSKA_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MATROSKA  DEFAULT_MSG MATROSKA_LIBRARY  MATROSKA_INCLUDE_DIR)
 	
MARK_AS_ADVANCED(MATROSKA_INCLUDE_DIR)
MARK_AS_ADVANCED(MATROSKA_LIBRARY)
