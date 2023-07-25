cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

set(SDL_SHARED OFF CACHE INTERNAL "")
set(SDL_STATIC ON CACHE INTERNAL "")
#TODO: pic ?

add_subdirectory(./SDL EXCLUDE_FROM_ALL)
#add_subdirectory(./SDL)

