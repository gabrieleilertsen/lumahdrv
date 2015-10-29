include_directories ("./")

add_library(matroska_build
    src/FileKax.cpp 
	src/KaxAttached.cpp 
	src/KaxAttachments.cpp 
	src/KaxBlock.cpp 
	src/KaxBlockData.cpp 
	src/KaxCluster.cpp 
	src/KaxContexts.cpp 
	src/KaxCues.cpp 
	src/KaxCuesData.cpp 
	src/KaxInfoData.cpp 
	src/KaxSeekHead.cpp 
	src/KaxSegment.cpp 
	src/KaxSemantic.cpp 
	src/KaxTracks.cpp 
	src/KaxVersion.cpp
)

target_link_libraries(matroska_build ${EBML_LIBRARY})
