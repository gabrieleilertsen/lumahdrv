# - Find PFS library


SET(_pfs_SEARCH_DIRS
  /usr/local
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
#  "C:/Program Files (x86)/OpenEXR" # Windows
)

FIND_PATH(PFS_INCLUDE_DIR
  NAMES
    pfs.h
  HINTS
    ENV CPATH
    ${_pfs_SEARCH_DIRS}
  PATH_SUFFIXES
    include/pfs-1.2 include/pfs include pfs-1.2 pfs ""
)

FIND_LIBRARY( PFS_LIBRARY
    NAMES
      pfs-1.2 pfs 
    HINTS
      ENV LIBRARY_PATH
      ${_pfs_SEARCH_DIRS}
    PATH_SUFFIXES
      lib64 lib ""
    )


# handle the QUIETLY and REQUIRED arguments and set OPENEXR_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PFS  DEFAULT_MSG PFS_LIBRARY  PFS_INCLUDE_DIR)
 	
MARK_AS_ADVANCED(PFS_INCLUDE_DIR)
MARK_AS_ADVANCED(PFS_LIBRARY)
