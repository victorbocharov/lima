
########### next target ###############
include_directories(BEFORE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tools ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

SET(limanlpserver_SRCS
   LimaNLPServer.cpp
   main.cpp
   analysisthread.cpp
)

set (limanlpserver_MOC_HEADERS 
    LimaNLPServer.h
    analysisthread.h )


qt5_wrap_cpp (limanlpserver_MOC_OUTFILES ${limanlpserver_MOC_HEADERS})

add_executable(limanlpserver ${limanlpserver_SRCS} ${limanlpserver_MOC_OUTFILES})

target_link_libraries(limanlpserver
    lima-common
    lima-linguisticprocessing
    ${QHTTPSERVER_NAMES}
    ${QT_LIBRARIES}
    ${Boost_LIBRARIES})

install(TARGETS limanlpserver DESTINATION bin)

