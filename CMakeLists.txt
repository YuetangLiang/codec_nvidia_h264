project(nvh264 C CXX CUDA)

set(PUBLIC_DOCS
    README.md
)

set(SOURCES
  NvDecoder.cu
)

set(LIBRARIES
    ${LIBRARIES}
    # _nvjpg
    # nvjpeg
    nvcuvid
    cuda
    avcodec
    avutil
    avformat
    nppicc
    nppidei
)

#add_executable(${PROJECT_NAME}_test ${SOURCES} test.cu)
target_link_libraries(${PROJECT_NAME}_test PRIVATE ${LIBRARIES})
add_library(${PROJECT_NAME} SHARED ${SOURCES})

