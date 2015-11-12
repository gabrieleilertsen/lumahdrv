cmake_minimum_required (VERSION 2.6)
project (LumaHDRv)

option(BUILD_TEST_EXAMPLES
  "Build the simple test applications in ./test" ON)

set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )

if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    set(BUILD_WITH_INSTALL_RPATH 1)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.8")
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wno-long-long -pedantic")
endif()

# === Search for dependencies ==================================================
find_package(OpenGL)
find_package(GLUT)
find_package(GLEW)
find_package(OpenEXR)
find_package(VPX)
find_package(PFS)
find_package(EBML)
find_package(Matroska)


# === Add Luma codec library ===================================================
add_library(luma_encoder SHARED
    ${PROJECT_SOURCE_DIR}/src/luma_encoder.cpp
    ${PROJECT_SOURCE_DIR}/src/luma_quantizer.cpp
    ${PROJECT_SOURCE_DIR}/src/mkv_interface.cpp
)

add_library(luma_decoder SHARED
    ${PROJECT_SOURCE_DIR}/src/luma_decoder.cpp
    ${PROJECT_SOURCE_DIR}/src/luma_quantizer.cpp
    ${PROJECT_SOURCE_DIR}/src/mkv_interface.cpp
)

message( "\n\n==============================================================" )
    message( "Dependencies:\n" )
    
# === PFS ======================================================================
message( " * PFS" )
if( NOT PFS_FOUND )
    message( WARNING "PFS library not found. Integration with pfstools disabled." )
    set( PFS_LIBRARY "" )
    set( HAVE_PFS 0 )
else( NOT PFS_FOUND )
    include_directories ("${PFS_INCLUDE_DIR}")
    message( "\tinclude: ${PFS_INCLUDE_DIR}\n\tlibs: ${PFS_LIBRARY}" )
    set( HAVE_PFS 1 )
endif( NOT PFS_FOUND )


# === OpenEXR ==================================================================
message( " * OpenEXR" )
if( NOT OPENEXR_FOUND )
    message( WARNING "OpenExr not found. lumaenc, lumadec and test examples will not be compiled." )
    set( HAVE_OPENEXR 0 )
else( NOT OPENEXR_FOUND )
    message( "\tinclude: ${OPENEXR_INCLUDE_DIRS}\n\tlibs: ${OPENEXR_LIBRARIES}" )
    set( HAVE_OPENEXR 1 )
endif( NOT OPENEXR_FOUND )


# === OpenGL ===================================================================
message( " * OpenGL" )
if( NOT OPENGL_FOUND )
    message( WARNING "OpenGL not found. lumaplay will not be compiled." )
    set(HAVE_OPENGL NO)
elseif( NOT GLUT_FOUND )
    message( WARNING "GLUT not found. lumaplay will not be compiled." )
    set(HAVE_OPENGL NO)
elseif( NOT GLEW_FOUND )
    message( WARNING "GLEW not found. lumaplay will not be compiled." )
    set(HAVE_OPENGL NO)
else( NOT GLEW_FOUND )
    message( "\tGL include: ${OPENGL_INCLUDE_DIR}\n\tGL libs: ${OPENGL_LIBRARIES}" )
    message( "\tglut include: ${GLUT_INCLUDE_DIR}\n\tglut lib: ${GLUT_glut_LIBRARY}" )
    message( "\tglew include: ${GLEW_INCLUDE_DIR}\n\tglew lib: ${GLEW_LIBRARIES}" )
    set(HAVE_OPENGL YES)  
endif( NOT OPENGL_FOUND )


# === EBML =====================================================================
# If EBML cannot be found, it will be built with the including EBML sources
message( " * EBML" )
if( NOT EBML_FOUND )
    message( WARNING "EBML library not found. Static EBML library will be built...")
    set (CMAKE_POSITION_INDEPENDENT_CODE TRUE)
    set(EBML_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/ebml")
    set(EBML_LIBRARY "ebml_build")
    link_directories("${PROJECT_BINARY_DIR}/lib/ebml")
    add_dependencies(luma_encoder ebml_build)
    add_dependencies(luma_decoder ebml_build)
    add_subdirectory(lib/ebml)
else( NOT EBML_FOUND )
    message( "\tinclude: ${EBML_INCLUDE_DIR}\n\tlib: ${EBML_LIBRARY}" )
endif( NOT EBML_FOUND )


# === Matroska =================================================================
# If Matroska cannot be found, it will be built with the including Matroska sources
message( " * Matroska" )
if( NOT MATROSKA_FOUND OR NOT EBML_FOUND )
    message( WARNING "Matroska library not found. Static Matroska library will be built...")
    set (CMAKE_POSITION_INDEPENDENT_CODE TRUE)
    include_directories ("${EBML_INCLUDE_DIR}")
    set(MATROSKA_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/matroska")
    set(MATROSKA_LIBRARY "matroska_build")
    add_dependencies(luma_encoder matroska_build)
    add_dependencies(luma_decoder matroska_build)
    add_subdirectory (lib/matroska)
else( NOT MATROSKA_FOUND OR NOT EBML_FOUND )
    message( "\tinclude: ${MATROSKA_INCLUDE_DIR}\n\tlib: ${MATROSKA_LIBRARY}" )
endif( NOT MATROSKA_FOUND OR NOT EBML_FOUND )


# === VPX ======================================================================
# If VPX cannot be found, it will be built as an external project
message( " * VPX" )
if ( NOT VPX_FOUND )
    if (WIN32 OR WIN64)
        message( FATAL_ERROR "Unable to find VPX library. Please see README for info on how to include this in Windows." )
    else ()
        message( WARNING "VPX library not found. Static VPX library will be built...")
        include(ExternalProject)

        find_program (YASM_EXECUTABLE yasm)
        if( YASM_EXECUTABLE )
	  message( "Using yasm: ${YASM_EXECUTABLE}" )
        else( YASM_EXECUTABLE )
	  message( SEND_ERROR "yasm not found. You need to install yasm to compile the VPX library." )
        endif( YASM_EXECUTABLE )
        
        
        ExternalProject_Add(
            libvpx
            SOURCE_DIR ${PROJECT_SOURCE_DIR}/lib/libvpx
            URL ${PROJECT_SOURCE_DIR}/lib/libvpx.tar.gz
            CONFIGURE_COMMAND ${PROJECT_SOURCE_DIR}/lib/libvpx/configure --enable-vp9-highbitdepth --enable-pic --disable-shared --enable-static --enable-vp9 --enable-multithread --as=yasm --disable-avx2 --disable-libyuv --disable-webm_io --disable-unit_tests --disable-install-docs --disable-install-bins --disable-install-srcs --disable-examples --disable-docs --disable-vp8-encoder --disable-vp8-decoder --prefix=${PROJECT_BINARY_DIR}/lib
            PREFIX ${PROJECT_BINARY_DIR}/lib
            BUILD_COMMAND make
            BUILD_IN_SOURCE 1
        )

        ExternalProject_Get_Property(libvpx install_dir)
        #include_directories(${install_dir}/include/vpx)
        set(VPX_INCLUDE_DIR "${install_dir}/include/vpx")
        set(VPX_LIBRARY "${install_dir}/lib/libvpx.a")
        list(APPEND VPX_LIBRARY "pthread")

        add_dependencies(luma_encoder libvpx)
        add_dependencies(luma_decoder libvpx)
        #target_link_libraries(libvpx)
    endif ()
else ( NOT VPX_FOUND )
    message( "\tinclude: ${VPX_INCLUDE_DIR}\n\tlib: ${VPX_LIBRARY}" )
endif ( NOT VPX_FOUND )


# === Targets ==================================================================
include_directories ("${PROJECT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/include/luma" "${PROJECT_BINARY_DIR}" "${EBML_INCLUDE_DIR}" "${MATROSKA_INCLUDE_DIR}" "${VPX_INCLUDE_DIR}")
#add_subdirectory (src)
#set(SOURCES lumaenc.cpp ${PROJECT_SOURCE_DIR}/src/exr_interface)

target_link_libraries(luma_encoder ${VPX_LIBRARY} ${EBML_LIBRARY} ${MATROSKA_LIBRARY})
target_link_libraries(luma_decoder ${VPX_LIBRARY} ${EBML_LIBRARY} ${MATROSKA_LIBRARY})

# lumaenc, lumadec and test examples can only be built if OpenExr is found
if ( HAVE_OPENEXR )
    if ( HAVE_PFS )
        add_executable(lumaenc 
            lumaenc.cpp
            ${PROJECT_SOURCE_DIR}/src/exr_interface.cpp
            ${PROJECT_SOURCE_DIR}/src/pfs_interface.cpp
            ${PROJECT_SOURCE_DIR}/src/arg_parser.cpp
        )

        add_executable(lumadec
            lumadec.cpp
            ${PROJECT_SOURCE_DIR}/src/exr_interface.cpp
            ${PROJECT_SOURCE_DIR}/src/pfs_interface.cpp
            ${PROJECT_SOURCE_DIR}/src/arg_parser.cpp
        )
    else ( HAVE_PFS )
        add_executable(lumaenc 
            lumaenc.cpp
            ${PROJECT_SOURCE_DIR}/src/exr_interface.cpp
            ${PROJECT_SOURCE_DIR}/src/arg_parser.cpp
        )

        add_executable(lumadec
            lumadec.cpp
            ${PROJECT_SOURCE_DIR}/src/exr_interface.cpp
            ${PROJECT_SOURCE_DIR}/src/arg_parser.cpp
        )
    endif ( HAVE_PFS )

    include_directories ("${OPENEXR_INCLUDE_DIRS}")
    target_link_libraries(lumaenc luma_encoder ${VPX_LIBRARY} ${EBML_LIBRARY} ${MATROSKA_LIBRARY} ${OPENEXR_LIBRARIES} ${PFS_LIBRARY})
    target_link_libraries(lumadec luma_decoder ${VPX_LIBRARY} ${EBML_LIBRARY} ${MATROSKA_LIBRARY} ${OPENEXR_LIBRARIES} ${PFS_LIBRARY})

    if (BUILD_TEST_EXAMPLES)
        include_directories ("${PROJECT_SOURCE_DIR}/test")
        add_subdirectory (test)
    endif (BUILD_TEST_EXAMPLES)
endif ( HAVE_OPENEXR )

# lumaplay can only be built if OpenGL is found
if( HAVE_OPENGL )
    add_executable(lumaplay
        lumaplay.cpp
        ${PROJECT_SOURCE_DIR}/src/arg_parser.cpp
    )
    
    # Wrap shader code in to string file that can be loaded on compile time in lumaplay
    set (py_cmd "fi = open('${PROJECT_SOURCE_DIR}/src/lumaplay_dequantizer.frag', 'r+')\; fo = open('${PROJECT_BINARY_DIR}/lumaplay_dequantizer_glsl.h', 'w')\;\n")
    set (py_cmd "${py_cmd}for line in fi: fo.write( '\"{0:80s}\\\\n\"\\n'.format(line.rstrip()) )\;\n")
    set (py_cmd "${py_cmd}fi.close()\; fo.close()\;")
    
    set (py_cmd "${py_cmd}fi = open('${PROJECT_SOURCE_DIR}/src/lumaplay_gui.frag', 'r+')\; fo = open('${PROJECT_BINARY_DIR}/lumaplay_gui_glsl.h', 'w')\;\n")
    set (py_cmd "${py_cmd}for line in fi: fo.write( '\"{0:80s}\\\\n\"\\n'.format(line.rstrip()) )\;\n")
    set (py_cmd "${py_cmd}fi.close()\; fo.close()\;")

    execute_process(COMMAND python -c ${py_cmd}
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
                  RESULT_VARIABLE py_result)

    if (py_result EQUAL 1)
        message( FATAL_ERROR "Unable to execute python script" )
    elseif (py_result EQUAL 2)
        message( FATAL_ERROR "Unable to find python script" )
    endif (py_result EQUAL 1)

    include_directories("${OPENGL_INCLUDE_DIR}" "${GLUT_INCLUDE_DIR}" "${GLEW_INCLUDE_DIR}")
    target_link_libraries(lumaplay ${OPENGL_LIBRARIES} ${GLUT_glut_LIBRARY} ${GLEW_LIBRARIES} luma_decoder ${VPX_LIBRARY} ${EBML_LIBRARY} ${MATROSKA_LIBRARY})
endif( HAVE_OPENGL )


# === Installation =============================================================
install(TARGETS luma_encoder luma_decoder
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

if ( HAVE_OPENEXR )        
    install(TARGETS lumaenc lumadec
            RUNTIME DESTINATION bin)
endif ( HAVE_OPENEXR )

if ( HAVE_OPENGL )        
    install(TARGETS lumaplay
            RUNTIME DESTINATION bin)
endif ( HAVE_OPENGL )

install(DIRECTORY doc/
        DESTINATION share/man/man1
        COMPONENT doc
        FILES_MATCHING
        PATTERN "*.1"
        PATTERN ".svn" EXCLUDE )

install(DIRECTORY include/luma
        DESTINATION include
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN ".svn" EXCLUDE )


# === Uninstallation target ====================================================
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)


# === Config ===================================================================
configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
  )


# === Build info ===============================================================
if (NOT EBML_FOUND OR NOT MATROSKA_FOUND OR NOT VPX_FOUND)
    message( "\n==============================================================" )
    message( "Dependencies that will be built and statically linked:\n" )
    if( NOT EBML_FOUND )
        message( "\tlibebml" )
    endif( NOT EBML_FOUND )
    if( NOT MATROSKA_FOUND )
        message( "\tlibmatroska" )
    endif( NOT MATROSKA_FOUND )
    if( NOT VPX_FOUND )
        message( "\tlibvpx" )
    endif( NOT VPX_FOUND )
endif (NOT EBML_FOUND OR NOT MATROSKA_FOUND OR NOT VPX_FOUND)
message( "\n==============================================================" )
message( "Components that will be built:\n" )
message( "\tlibluma_encoder" )
message( "\tlibluma_decoder" )
if ( HAVE_OPENEXR )        
    message("\tlumaenc (pfs support: ${HAVE_PFS})" )
    message("\tlumadec (pfs support: ${HAVE_PFS})" )
    if (BUILD_TEST_EXAMPLES)
        message("\ttest_simple_enc" )
        message("\ttest_simple_dec" )
    endif (BUILD_TEST_EXAMPLES)
endif ( HAVE_OPENEXR )
if ( HAVE_OPENGL )        
    message("\tlumaplay" )
endif ( HAVE_OPENGL )

message( "\n==============================================================\n" )

