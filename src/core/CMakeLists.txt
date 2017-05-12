FILE(GLOB SOURCE "${PROJECT_ROOT_PATH}/src/core/*.cpp")
ADD_LIBRARY(core STATIC ${SOURCE})  
TARGET_LINK_LIBRARIES(core ssl crypto cares pcre http_parser)
