cmake_minimum_required(VERSION 3.12)
project(clibparser)

set(CMAKE_CXX_STANDARD 14)

link_libraries(freeglut opengl32 glu32)

add_executable(clibparser main.cpp
        cast.h cast.cpp
        cexception.h cexception.cpp
        clexer.h clexer.cpp
        cparser.h cparser.cpp
        memory.h
        types.h types.cpp
        cunit.h cunit.cpp
        cgen.h cgen.cpp
        cvm.h cvm.cpp
        cgui.h cgui.cpp
        cmem.h cmem.cpp
        cvfs.h cvfs.cpp)

add_executable(clibparser-test test.cpp
        cast.h cast.cpp
        cexception.h cexception.cpp
        clexer.h clexer.cpp
        cparser.h cparser.cpp
        memory.h
        types.h types.cpp
        cunit.h cunit.cpp
        cgen.h cgen.cpp
        cvm.h cvm.cpp
        cgui.h cgui.cpp
        cmem.h cmem.cpp
        cvfs.h cvfs.cpp)