# This file configures the "build" of this SKSE plugin
# - how to compile the C++ plugin (in addition to settings found in CMakePresets.json)
# - where to output the resulting .dll file

# It's recommended to set a minimum CMake version.
# If you use CMake features from higher versions, update this to match.
cmake_minimum_required(VERSION 3.21)

# [BEGIN set_project_name]
# Set your project name. This will be the name of your SKSE .dll file.
project(EternalBuffsNG VERSION 1.1.0 LANGUAGES CXX)

# [END set_project_name]

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/Plugin.h.in
	${CMAKE_CURRENT_BINARY_DIR}/cmake/Plugin.h
	@ONLY
)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
	${CMAKE_CURRENT_BINARY_DIR}/cmake/version.rc
	@ONLY
)

# If you're not using a mod manager, you probably want the SKSE plugin to go
# inside of your Skyrim "Data" folder.
#
# To do this automatically, set the `SKYRIM_FOLDER` environment variable
# to the path of your Skyrim Special Edition folder
if(DEFINED ENV{SKYRIM_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_FOLDER}/Data")
    set(OUTPUT_FOLDER "$ENV{SKYRIM_FOLDER}/Data")
endif()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
# set(SKYRIM_MODS_FOLDER_LOCAL "F:/MO2/mods/mods")  # replace with your desired path

# If you're using Mod Organizer 2 or Vortex, you might want this to go inside
# of your "mods" folder, inside of a subfolder named "<your mod>".
#
# To do this automatically, set the `SKYRIM_MODS_FOLDER` environment variable
# to the path of your "mods" folder
#if(DEFINED SKYRIM_MODS_FOLDER_LOCAL AND IS_DIRECTORY "$SKYRIM_MODS_FOLDER_LOCAL")
 #   set(OUTPUT_FOLDER "$SKYRIM_MODS_FOLDER_LOCAL/${PROJECT_NAME}")
#endif()

# if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
#     set(OUTPUT_FOLDER "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME}")
# endif()

# Otherwise, you can set OUTPUT_FOLDER to any place you'd like :)
set(OUTPUT_FOLDER "G:/Skyrim/mods/mods/EternalBuffsNG")

find_package(spdlog CONFIG REQUIRED)

# Setup your SKSE plugin as an SKSE plugin!
find_package(CommonLibSSE CONFIG REQUIRED)
add_commonlibsse_plugin(${PROJECT_NAME} SOURCES src/plugin.cpp) # <--- specifies plugin.cpp
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23) # <--- use C++23 standard
target_precompile_headers(${PROJECT_NAME} PRIVATE include/PCH.h) # <--- PCH.h is required!

target_include_directories(
	"${PROJECT_NAME}"
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/cmake
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

# target_include_directories(${PROJECT_NAME} PRIVATE "Deps/simpleini")

target_sources(
	"${PROJECT_NAME}"
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/cmake/Plugin.h
		${CMAKE_CURRENT_BINARY_DIR}/cmake/version.rc
        "include/SpellCastEventHandler.h" "src/SpellCastEventHandler.cpp"
        "include/SpellDataPersistence.h" "src/SpellDataPersistence.cpp"
        "include/SpellLogging.h" "src/SpellLogging.cpp"
        "include/SpellApplication.h" "src/SpellApplication.cpp"
        "include/ConfigLoader.h" "src/ConfigLoader.cpp"
        "include/StringUtilities.h" "src/StringUtilities.cpp"
        "include/ConfigRules.h" "src/ConfigRules.cpp"
        "include/TPPlayerInputEventHandler.h" "src/TPPlayerInputEventHandler.cpp"
        "include/Config.h" "src/Config.cpp"
        "include/ConfigParser.h" "src/ConfigParser.cpp"
        "include/SpellUtilities.h" "src/SpellUtilities.cpp"
        "include/ReserveMagicka.h" "src/ReserveMagicka.cpp"
)

target_precompile_headers(
	"${PROJECT_NAME}"
	PRIVATE
		include/PCH.h
)

# When your SKSE .dll is compiled, this will automatically copy the .dll into your mods folder.
# Only works if you configure DEPLOY_ROOT above (or set the SKYRIM_MODS_FOLDER environment variable)
if(DEFINED OUTPUT_FOLDER)
    # If you specify an <OUTPUT_FOLDER> (including via environment variables)
    # then we'll copy your mod files into Skyrim or a mod manager for you!

    # Copy the SKSE plugin .dll files into the SKSE/Plugins/ folder
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")

    message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM
    )

    # If you perform a "Debug" build, also copy .pdb file (for debug symbols)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(
            TARGET "${PROJECT_NAME}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM
        )
    endif()
endif()
