include(${CMAKE_CURRENT_SOURCE_DIR}/../max-sdk-base/script/max-pretarget.cmake)

#############################################################
# MAX EXTERNAL
#############################################################

include_directories( 
	"${MAX_SDK_INCLUDES}"
	"${MAX_SDK_MSP_INCLUDES}"
	"${MAX_SDK_JIT_INCLUDES}"
	"${CMAKE_CURRENT_SOURCE_DIR}/../libfreenect2/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/../libfreenect2/include/internal"
	"${CMAKE_BINARY_DIR}/source/libfreenect2"
)

file(GLOB PROJECT_SRC
     "*.h"
	 "*.c"
     "*.cpp"
)

add_library( 
	${PROJECT_NAME} 
	MODULE
	${PROJECT_SRC}
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../max-sdk-base/script/max-posttarget.cmake)