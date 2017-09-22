SET(EXECUTABLE_OUTPUT_PATH "${PROJECT_ROOT_PATH}/bin/samples/")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--rpath,../../library/cares/lib:../../library/pcre/lib")
SET(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/library/cares/lib:${CMAKE_INSTALL_PREFIX}/library/pcre/lib)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/file-test.cpp")
ADD_EXECUTABLE(file-test ${SOURCE})
TARGET_LINK_LIBRARIES(file-test core)
install(TARGETS file-test RUNTIME DESTINATION bin/samples)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/hostinfo-test.cpp")
ADD_EXECUTABLE(hostinfo-test ${SOURCE})
TARGET_LINK_LIBRARIES(hostinfo-test core)
install(TARGETS hostinfo-test RUNTIME DESTINATION bin/samples)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/stringlist-test.cpp")
ADD_EXECUTABLE(stringlist-test ${SOURCE})
TARGET_LINK_LIBRARIES(stringlist-test core)
install(TARGETS stringlist-test RUNTIME DESTINATION bin/samples)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/string-test.cpp")
ADD_EXECUTABLE(string-test ${SOURCE})
TARGET_LINK_LIBRARIES(string-test core)
install(TARGETS string-test RUNTIME DESTINATION bin/samples)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/process-test.cpp")
ADD_EXECUTABLE(process-test ${SOURCE})
TARGET_LINK_LIBRARIES(process-test core)
install(TARGETS process-test RUNTIME DESTINATION bin/samples)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/datetime-test.cpp")
ADD_EXECUTABLE(datetime-test ${SOURCE})
TARGET_LINK_LIBRARIES(datetime-test core)
install(TARGETS datetime-test RUNTIME DESTINATION bin/samples)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/elapsedtimer-test.cpp")
ADD_EXECUTABLE(elapsedtimer-test ${SOURCE})
TARGET_LINK_LIBRARIES(elapsedtimer-test core)
install(TARGETS elapsedtimer-test RUNTIME DESTINATION bin/samples)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/md5-test.cpp")
ADD_EXECUTABLE(md5-test ${SOURCE})
TARGET_LINK_LIBRARIES(md5-test core)
install(TARGETS md5-test RUNTIME DESTINATION bin/samples)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/rapidjson-test.cpp")
ADD_EXECUTABLE(rapidjson-test ${SOURCE})
TARGET_LINK_LIBRARIES(rapidjson-test core)
install(TARGETS rapidjson-test RUNTIME DESTINATION bin/samples)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/threads-test.cpp")
ADD_EXECUTABLE(threads-test ${SOURCE})
TARGET_LINK_LIBRARIES(threads-test core)
install(TARGETS threads-test RUNTIME DESTINATION bin/samples)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/sharedptr-test.cpp")
ADD_EXECUTABLE(sharedptr-test ${SOURCE})
TARGET_LINK_LIBRARIES(sharedptr-test core)
install(TARGETS sharedptr-test RUNTIME DESTINATION bin/samples)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/tcpserver-test.cpp")
ADD_EXECUTABLE(tcpserver-test ${SOURCE})
TARGET_LINK_LIBRARIES(tcpserver-test core)
install(TARGETS tcpserver-test RUNTIME DESTINATION bin/samples)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/mempool-test.cpp")
ADD_EXECUTABLE(mempool-test ${SOURCE})
TARGET_LINK_LIBRARIES(mempool-test core)
install(TARGETS mempool-test RUNTIME DESTINATION bin/samples)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/mutex-test.cpp")
ADD_EXECUTABLE(mutex-test ${SOURCE})
TARGET_LINK_LIBRARIES(mutex-test core)
install(TARGETS mutex-test RUNTIME DESTINATION bin/samples)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/regex-test.cpp")
ADD_EXECUTABLE(regex-test ${SOURCE})
TARGET_LINK_LIBRARIES(regex-test core)
install(TARGETS regex-test RUNTIME DESTINATION bin/samples)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/http-test.cpp")
ADD_EXECUTABLE(http-test ${SOURCE})
TARGET_LINK_LIBRARIES(http-test core)
install(TARGETS http-test RUNTIME DESTINATION bin/samples)

FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/samples/gzip-test.cpp")
ADD_EXECUTABLE(gzip-test ${SOURCE})
TARGET_LINK_LIBRARIES(gzip-test core)
install(TARGETS gzip-test RUNTIME DESTINATION bin/samples)
