cmake_minimum_required( VERSION 2.6 )
project( test_es3 )

include_directories( External/Include )

find_package( OpenCV REQUIRED )
find_package(X11)

find_library( OPENGLES3_LIBRARY GLESv2 "OpenGL ES v3.0 library"
	PATHS /opt/Imagination/PowerVR_Graphics/PowerVR_Tools/PVRVFrame/Library/Linux_x86_64
	NO_DEFAULT_PATH)
find_library( EGL_LIBRARY EGL "EGL 1.4 library" 
	PATHS /opt/Imagination/PowerVR_Graphics/PowerVR_Tools/PVRVFrame/Library/Linux_x86_64
	NO_DEFAULT_PATH)
find_library(M_LIB m)

add_executable( ${PROJECT_NAME}  test_es3.cpp)
target_link_libraries( ${PROJECT_NAME} 
    ${OPENGLES3_LIBRARY} 
    ${EGL_LIBRARY} 
    ${X11_LIBRARIES} 
    ${M_LIB} 
    pthread
    ${OpenCV_LIBS}
    )
