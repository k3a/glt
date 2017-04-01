cmake_minimum_required(VERSION 2.8)



project(glt C CXX)

if (MSVC)
	set (CMAKE_SKIP_RULE_DEPENDENCY TRUE)
	set (CMAKE_SUPPRESS_REGENERATION TRUE)
	include_directories (windows/include libpng glm glm/detail glm/gtc glm/gtx glm/virtrev glv/include)
	link_directories (../windows/lib)
	add_definitions (/SAFESEH:NO /wd4244 /wd4521 /D "_CRT_SECURE_NO_WARNINGS" /D "_SCL_SECURE_NO_WARNINGS")
	add_library( zlib.lib SHARED IMPORTED )
	set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} /SAFESEH:NO  /DYNAMICBASE zlib.lib")
endif (MSVC)


file(GLOB_RECURSE glt_SRC
	"glt/*.cpp"
	"glt/*.h"
	"glm/*.cpp"
	"libpng/*.c"
	"glv/*.cpp"
	"assimp/*.cpp"
	"assimp/*.c"
)



add_executable( glt ${glt_SRC} )

