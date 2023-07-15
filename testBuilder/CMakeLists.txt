cmake_minimum_required(VERSION 3.4.1)

macro(testBuilder_get_current_working_directory_of_this_cmake_file variable_name)
    set(${variable_name} ${CMAKE_CURRENT_SOURCE_DIR})
endmacro()

macro(testBuilder_add_source testName sourceFile)
    set(testBuilder_SOURCES_LIST_${testName} ${testBuilder_SOURCES_LIST_${testName}};${sourceFile})
endmacro()

# libraries can be specified as a space seperated list, or as individual calls
macro(testBuilder_add_include testName includeName)
    set(testBuilder_INCLUDE_LIST_${testName} ${testBuilder_INCLUDE_LIST_${testName}};${includeName})
endmacro()

# libraries can be specified as a space seperated list, or as individual calls
macro(testBuilder_add_library testName libraryName)
    set(testBuilder_LIBRARIES_LIST_${testName} ${testBuilder_LIBRARIES_LIST_${testName}};${libraryName})
endmacro()

# libraries can be specified as a space seperated list, or as individual calls
macro(testBuilder_add_dependency testName dependencyName)
    set(testBuilder_DEPENDENCY_LIST_${testName} ${testBuilder_DEPENDENCY_LIST_${testName}};${dependencyName})
endmacro()

macro(testBuilder_add_compile_option testName dependencyName)
    set(testBuilder_COMPILE_OPTIONS_${testName} ${testBuilder_COMPILE_OPTIONS_${testName}};${dependencyName})
endmacro()

macro(testBuilder_set_current_working_directory workingDirectory)
    set(testBuilder_WORKING_DIRECTORY ${workingDirectory})
endmacro()

macro(testBuilder_set_current_working_directory_to_default_binary_directory)
    set(testBuilder_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endmacro()

macro(testBuilder_build testName outDirectory)
    add_executable(${testName} ${testBuilder_SOURCES_LIST_${testName}})
    
    if (testBuilder_INCLUDE_LIST_${testName})
        target_include_directories(${testName} PUBLIC ${testBuilder_INCLUDE_LIST_${testName}})
    endif()
    
    if (testBuilder_DEPENDENCY_LIST_${testName})
        add_dependencies(${testName} ${testBuilder_DEPENDENCY_LIST_${testName}})
    endif()

    if (testBuilder_COMPILE_OPTIONS_${testName})
        target_compile_options(${testName} PUBLIC ${testBuilder_COMPILE_OPTIONS_${testName}})
    endif()
    
    if (testBuilder_LIBRARIES_LIST_${testName})
        target_link_libraries(${testName} ${testBuilder_LIBRARIES_LIST_${testName}})
    endif()

    set(testBuilder_IN_FILE ${testName})

    if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(testBuilder_IN_FILE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${testBuilder_IN_FILE})
    endif()

    set(testBuilder_OUT_FILE ${testBuilder_WORKING_DIRECTORY}/${outDirectory}/${testName})


    add_custom_command(
            TARGET ${testName}
            POST_BUILD
            COMMAND cp -v ${testBuilder_IN_FILE} ${testBuilder_OUT_FILE}
    )
endmacro()

macro(testBuilder_build_shared_library testName)
    add_library(${testName} SHARED ${testBuilder_SOURCES_LIST_${testName}})
    
    if (testBuilder_INCLUDE_LIST_${testName})
        target_include_directories(${testName} PUBLIC ${testBuilder_INCLUDE_LIST_${testName}})
    endif()
    
    if (testBuilder_DEPENDENCY_LIST_${testName})
        add_dependencies(${testName} ${testBuilder_DEPENDENCY_LIST_${testName}})
    endif()

    if (testBuilder_COMPILE_OPTIONS_${testName})
        target_compile_options(${testName} PUBLIC ${testBuilder_COMPILE_OPTIONS_${testName}})
    endif()
    
    if (testBuilder_LIBRARIES_LIST_${testName})
        target_link_libraries(${testName} ${testBuilder_LIBRARIES_LIST_${testName}})
    endif()
endmacro()
