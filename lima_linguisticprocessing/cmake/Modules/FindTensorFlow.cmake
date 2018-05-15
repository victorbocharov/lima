# Locates the tensorFlow library and include directories.

include(FindPackageHandleStandardArgs)
unset(TENSORFLOW_FOUND)

find_path(TensorFlow_INCLUDE_DIR
        NAMES
        tensorflow/core
        tensorflow/cc
        third_party
        HINTS
        /usr/local/include/google/tensorflow
        /usr/include/google/tensorflow)

find_library(TensorFlow_LIBRARY 
        NAMES 
#         tensorflow_all 
      tensorflow_cc
        HINTS
        /usr/lib
        /usr/local/lib)

find_library(TensorFlow_LIBRARY2 
        NAMES 
        tensorflow_framework
        HINTS
        /usr/lib
        /usr/local/lib)

# set TensorFlow_FOUND
find_package_handle_standard_args(TensorFlow DEFAULT_MSG TensorFlow_INCLUDE_DIR TensorFlow_LIBRARY)

# set external variables for usage in CMakeLists.txt
if(TENSORFLOW_FOUND)
    set(TensorFlow_INCLUDE_DIRS ${TensorFlow_INCLUDE_DIR})
    set(TensorFlow_LIBRARIES ${TensorFlow_LIBRARY} ${TensorFlow_LIBRARY2})
endif()

# hide locals from GUI
mark_as_advanced(TensorFlow_INCLUDE_DIR TensorFlow_LIBRARY)
