cmake_minimum_required(VERSION 2.8.12)

# Make sure we have C++11 enabled.
if(NOT ITK_IGNORE_CMAKE_CXX11_CHECKS)
  # Needed to make sure libraries and executables not built by the
  # itkModuleMacros still have the C++11 compiler flags enabled
  # Wrap this in an escape hatch for unknown compilers
  if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11) # Supported values are ``11``, ``14``, and ``17``.
  endif()
  if(NOT CMAKE_CXX_STANDARD_REQUIRED)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
  endif()
  if(NOT CMAKE_CXX_EXTENSIONS)
    set(CMAKE_CXX_EXTENSIONS OFF)
  endif()
endif()

project(Montage)

#set(Montage_LIBRARIES Montage)

if(NOT ITK_SOURCE_DIR)
  find_package(ITK REQUIRED)
  list(APPEND CMAKE_MODULE_PATH ${ITK_CMAKE_DIR})
  include(ITKModuleExternal)
else()
  itk_module_impl()
endif()

# itk_module_examples() # no examples currently
