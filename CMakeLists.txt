cmake_minimum_required(VERSION 3.10)
project(seed)
set(CMAKE_C_FLAGS "${CMAKE_XX_FLAGS} -Wall")

add_executable(main layers.c generator.c finders.c find_quadhuts.c)
add_executable(mainFinder layers.c generator.c finders.c main.c)