include_directories ("${PROJECT_SOURCE_DIR}/src" "${OPENEXR_INCLUDE_DIR}")

add_executable(test_simple_enc
    test_simple_enc.cpp
    ${PROJECT_SOURCE_DIR}/src/exr_interface.cpp
)
add_executable(test_simple_dec
    test_simple_dec.cpp
    ${PROJECT_SOURCE_DIR}/src/exr_interface.cpp
)

target_link_libraries(test_simple_enc luma_encoder ${VPX_LIBRARY} ${EBML_LIBRARY} ${MATROSKA_LIBRARY} ${OPENEXR_LIBRARIES})
target_link_libraries(test_simple_dec luma_decoder ${VPX_LIBRARY} ${EBML_LIBRARY} ${MATROSKA_LIBRARY} ${OPENEXR_LIBRARIES})
