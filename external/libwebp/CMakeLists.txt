cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

set(WEBP_LINK_STATIC ON CACHE INTERNAL "")

add_subdirectory(./libwebp EXCLUDE_FROM_ALL)

