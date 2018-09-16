TEMPLATE = app
CONFIG += console c++1z link_pkgconfig
CONFIG -= app_bundle
CONFIG -= qt

PKGCONFIG += sfml-all
#PRECOMPILED_HEADER = stable.hpp
INCLUDEPATH += networking-ts-impl/include
LIBS += -lpthread -lclang

#QMAKE_LINK += -fxray-instrument -fxray-instruction-threshold=1
#QMAKE_CXXFLAGS += -fxray-instrument -fxray-instruction-threshold=1


SOURCES += \
        main.cpp \
    snake.cpp \
    network.cpp

HEADERS += \
    main.h \
    snake.h \
    network.h\
    stable.hpp

