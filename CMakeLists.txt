include_directories(
  .
)

set(CMAKE_SHARED_LIBRARY_PREFIX "")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_library(IASUModule SHARED 

	main.cpp  organism.cpp
	innovation.cpp  	 network.cpp     population.cpp
	gene.cpp             link.cpp        nnode.cpp       species.cpp
	genome.cpp           maze.cpp        trait.cpp
	simulation.cpp  neat.cpp        noveltyset.cpp posix.cpp
    )

target_link_libraries(IASUModule BWAPILIB)
