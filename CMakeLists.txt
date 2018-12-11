cmake_minimum_required(VERSION 3.3)
project(IAR)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

if (MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if (BUILD_COMPAT)
	if (NOT MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
		#	add_subdirectory(compat)
	endif()
endif()


#add_subdirectory(PacMan/src/Engine)
#add_subdirectory(PacMan/src/Game)

include_directories(
  .
  PacMan/src/Engine
  PacMan/src/Game
)

#set(CMAKE_SHARED_LIBRARY_PREFIX "")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(pacmanNovelty

	main.cpp  			 organism.cpp
	innovation.cpp  	 network.cpp     population.cpp
	gene.cpp             link.cpp        nnode.cpp       	species.cpp
	genome.cpp           maze.cpp        trait.cpp
	simulation.cpp		 neat.cpp        noveltyset.cpp 	posix.cpp
	PacMan/src/		 PacMan/src/Engine/
    )

#target_link_libraries(IASUModule BWAPILIB)
