TEMPLATE = app
CONFIG += console c++1z link_pkgconfig
CONFIG -= app_bundle
CONFIG -= qt

PKGCONFIG += sfml-all
#PRECOMPILED_HEADER = ../src/stable.hpp
INCLUDEPATH += ../networking-ts-impl/include
LIBS += -lpthread -lclang

# QMAKE_LINK += -fxray-instrument -fxray-instruction-threshold=1
# QMAKE_CXXFLAGS += -fxray-instrument -fxray-instruction-threshold=1
QMAKE_CXXFLAGS += -fmodules-ts


SOURCES += \
        ../src/main.cpp \
    ../src/snake.cpp \
    ../src/network.cpp

HEADERS += \
    ../src/snake.h \
    ../src/network.h \
    ../src/stable_win32.hpp \
    ../src/engine.h \
    ../src/stable.hpp

