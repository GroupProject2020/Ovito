# Function for setting up precompiled headers. Usage:
#
#   add_library/executable(target
#       pchheader.c pchheader.cpp pchheader.h)
#
#   add_precompiled_header(target pchheader.h
#       [FORCEINCLUDE]
#       [SOURCE_C pchheader.c]
#       [SOURCE_CXX pchheader.cpp])
#
# Options:
#
#   FORCEINCLUDE: Add compiler flags to automatically include the
#   pchheader.h from every source file. Works with both GCC and
#   MSVC. This is recommended.
#
#   SOURCE_C/CXX: Specifies the .c/.cpp source file that includes
#   pchheader.h for generating the pre-compiled header
#   output. Defaults to pchheader.c. Only required for MSVC.
#
# Caveats:
#
#   * Its not currently possible to use the same precompiled-header in
#     more than a single target in the same directory (No way to set
#     the source file properties differently for each target).
#
#   * MSVC: A source file with the same name as the header must exist
#     and be included in the target (E.g. header.cpp). Name of file
#     can be changed using the SOURCE_CXX/SOURCE_C options.
#
# License:
#
# Copyright (C) 2009-2013 Lars Christensen <larsch@belunktum.dk>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the 'Software') deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

include(CMakeParseArguments)

macro(combine_arguments _variable)
  set(_result "")
  foreach(_element ${${_variable}})
    set(_result "${_result} \"${_element}\"")
  endforeach()
  string(STRIP "${_result}" _result)
  set(${_variable} "${_result}")
endmacro()

function(add_precompiled_header _target _input)
  cmake_parse_arguments(_PCH "FORCEINCLUDE" "SOURCE_CXX:SOURCE_C" "" ${ARGN})

  get_filename_component(_input_we ${_input} NAME_WE)
  if(NOT _PCH_SOURCE_CXX)
    set(_PCH_SOURCE_CXX "${_input_we}.cpp")
  endif()
  if(NOT _PCH_SOURCE_C)
    set(_PCH_SOURCE_C "${_input_we}.c")
  endif()

  if(MSVC)
    set(_cxx_path "${CMAKE_CURRENT_BINARY_DIR}/${_target}_cxx_pch")
    set(_c_path "${CMAKE_CURRENT_BINARY_DIR}/${_target}_c_pch")
    file(MAKE_DIRECTORY "${_cxx_path}")
    file(MAKE_DIRECTORY "${_c_path}")
    set(_pch_cxx_header "${_cxx_path}/${_input}")
    set(_pch_cxx_pch "${_cxx_path}/${_input_we}.pch")
    set(_pch_c_header "${_c_path}/${_input}")
    set(_pch_c_pch "${_c_path}/${_input_we}.pch")

    get_target_property(sources ${_target} SOURCES)
    foreach(_source ${sources})
      set(_pch_compile_flags "")
	    if(_source MATCHES \\.\(cpp\)$)

	      get_source_file_property(_object_generated "${_source}" GENERATED)
	      if(NOT _object_generated)
	        set(_pch_header "${_input}")
	        set(_pch "${_pch_cxx_pch}")

	        if(_source STREQUAL "${_PCH_SOURCE_CXX}")
	          set(_pch_compile_flags "${_pch_compile_flags} \"/Fp${_pch_cxx_pch}\" /Yc${_input}")
	          set(_pch_source_cxx_found TRUE)
	        elseif(_source STREQUAL "${_PCH_SOURCE_C}")
	          set(_pch_compile_flags "${_pch_compile_flags} \"/Fp${_pch_c_pch}\" /Yc${_input}")
	          set(_pch_source_c_found TRUE)
	        else()
	          if(_source MATCHES \\.\(cpp|cxx|cc\)$)
	            set(_pch_compile_flags "${_pch_compile_flags} \"/Fp${_pch_cxx_pch}\" /Yu${_input}")
	            set(_pch_source_cxx_needed TRUE)
	          else()
	            set(_pch_compile_flags "${_pch_compile_flags} \"/Fp${_pch_c_pch}\" /Yu${_input}")
	            set(_pch_source_c_needed TRUE)
	          endif()
	          if(_PCH_FORCEINCLUDE)
	            set(_pch_compile_flags "${_pch_compile_flags} /FI${_input}")
	          endif(_PCH_FORCEINCLUDE)
	        endif()

	        get_source_file_property(_object_depends "${_source}" OBJECT_DEPENDS)
	        if(NOT _object_depends)
	          set(_object_depends)
	        endif()
	        if(_PCH_FORCEINCLUDE)
	          if(_source MATCHES \\.\(cc|cxx|cpp\)$)
	            list(APPEND _object_depends "${_pch_header}")
	          else()
	            list(APPEND _object_depends "${_pch_header}")
	          endif()
	        endif()

	        set_source_files_properties(${_source} PROPERTIES
	          COMPILE_FLAGS "${_pch_compile_flags}"
	          OBJECT_DEPENDS "${_object_depends}")
        endif()
	    endif()
	  endforeach()

    if(_pch_source_cxx_needed AND NOT _pch_source_cxx_found)
      message(FATAL_ERROR "A source file ${_PCH_SOURCE_CXX} for ${_input} is required for MSVC builds. Can be set with the SOURCE_CXX option.")
    endif()
    if(_pch_source_c_needed AND NOT _pch_source_c_found)
      message(FATAL_ERROR "A source file ${_PCH_SOURCE_C} for ${_input} is required for MSVC builds. Can be set with the SOURCE_C option.")
    endif()
  endif(MSVC)

  if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    GET_FILENAME_COMPONENT(_name ${_input} NAME)
    GET_FILENAME_COMPONENT(_dir ${_input} DIRECTORY)
    SET_SOURCE_FILES_PROPERTIES(${_name} PROPERTIES LANGUAGE CXX)
    
    # Create a target that build the .pch/.gch precompiled header file from the .h file.
    # It inherits all compile options from the parent target.
    ADD_LIBRARY(${_target}_PCH OBJECT ${_name})
    TARGET_INCLUDE_DIRECTORIES(${_target}_PCH PUBLIC "$<TARGET_PROPERTY:${_target},INCLUDE_DIRECTORIES>")
    TARGET_COMPILE_DEFINITIONS(${_target}_PCH PUBLIC "$<TARGET_PROPERTY:${_target},COMPILE_DEFINITIONS>")
    TARGET_COMPILE_OPTIONS(${_target}_PCH PUBLIC "$<TARGET_PROPERTY:${_target},COMPILE_OPTIONS>")
    GET_TARGET_PROPERTY(_target_type ${_target} TYPE)
    IF(_target_type STREQUAL SHARED_LIBRARY)
      TARGET_COMPILE_DEFINITIONS(${_target}_PCH PUBLIC "${_target}_EXPORTS")
    ENDIF()
    TARGET_COMPILE_OPTIONS(${_target}_PCH PRIVATE "-x" "c++-header")
    GET_TARGET_PROPERTY(_CXX_VISIBILITY_PRESET ${_target} CXX_VISIBILITY_PRESET)
    GET_TARGET_PROPERTY(_VISIBILITY_INLINES_HIDDEN ${_target} VISIBILITY_INLINES_HIDDEN)
    SET_TARGET_PROPERTIES(${_target}_PCH PROPERTIES CXX_VISIBILITY_PRESET "${_CXX_VISIBILITY_PRESET}")
    SET_TARGET_PROPERTIES(${_target}_PCH PROPERTIES VISIBILITY_INLINES_HIDDEN "${_VISIBILITY_INLINES_HIDDEN}")
        
    # The parent target depends on the PCH build target.
    ADD_DEPENDENCIES(${_target} ${_target}_PCH)

    if(CMAKE_COMPILER_IS_GNUCXX)
      set(_pch_extension "gch")
    else()
      set(_pch_extension "pch")
    endif()
    
    # Create a new directory containing a file named .gch, which is a symlink to the precompiled header.
    # CMake always produces a precompile header file with a ".h.o" extension, but GCC requires the .gch extension.
    SET(_pch_dir "${CMAKE_CURRENT_BINARY_DIR}/${_target}.${_pch_extension}")
    FILE(MAKE_DIRECTORY "${_pch_dir}/${_dir}") 
    EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E create_symlink "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_target}_PCH.dir/${_name}.o" "${_pch_dir}/${_input}_pch.h.${_pch_extension}")

    # Force include the precompiled header in all .cpp files.
    get_property(_sources TARGET ${_target} PROPERTY SOURCES)
    foreach(_source ${_sources})
      if(_source MATCHES \\.\(cc|cxx|cpp\)$)
        get_source_file_property(_pch_compile_flags "${_source}" COMPILE_FLAGS)
        if(NOT _pch_compile_flags)
          set(_pch_compile_flags)
        endif()
        separate_arguments(_pch_compile_flags)
        list(APPEND _pch_compile_flags -Winvalid-pch)
        list(APPEND _pch_compile_flags -include "${_pch_dir}/${_input}_pch.h")
        combine_arguments(_pch_compile_flags)
        set_source_files_properties(${_source} PROPERTIES COMPILE_FLAGS "${_pch_compile_flags}")
      endif()
    endforeach()
  endif()

endfunction()
