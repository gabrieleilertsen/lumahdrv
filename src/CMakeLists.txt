include_directories ("${PROJECT_SOURCE_DIR}/include")

add_library(luma_encoder SHARED
    luma_encoder.cpp
    luma_quantizer.cpp
    mkv_interface.cpp
)

add_library(luma_decoder SHARED
    luma_decoder.cpp
    luma_quantizer.cpp
    mkv_interface.cpp
)

target_link_libraries(luma_encoder ${VPX_LIBRARY} ${EBML_LIBRARY} ${MATROSKA_LIBRARY})
target_link_libraries(luma_decoder ${VPX_LIBRARY} ${EBML_LIBRARY} ${MATROSKA_LIBRARY})

install(TARGETS luma_encoder luma_decoder
        LIBRARY DESTINATION lib)
