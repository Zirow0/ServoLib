# ─── fetch_or_local(<name> <git_url> <git_tag>) ───────────────────────────────
# Підключає CMake-залежність за пріоритетом:
# 1. Lib/<name>/ — git submodule або symlink (офлайн, пріоритет)
# 2. FetchContent — автозавантаження (fallback, потребує інтернет)

include(FetchContent)

macro(fetch_or_local name git_url git_tag)
    if(EXISTS "${CMAKE_SOURCE_DIR}/Lib/${name}/CMakeLists.txt")
        message(STATUS "[${name}] Lib/${name}")
        if(NOT TARGET ${name})
            add_subdirectory(
                "${CMAKE_SOURCE_DIR}/Lib/${name}"
                "${CMAKE_BINARY_DIR}/_deps/${name}-local"
                EXCLUDE_FROM_ALL
            )
        endif()
    else()
        message(STATUS "[${name}] FetchContent ← ${git_url}")
        FetchContent_Declare(${name}
            GIT_REPOSITORY ${git_url}
            GIT_TAG        ${git_tag}
            GIT_SHALLOW    TRUE
        )
        FetchContent_MakeAvailable(${name})
    endif()
endmacro()
