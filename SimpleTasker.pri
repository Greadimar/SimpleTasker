HEADERS += \
    $$PWD/simpletasker.h \
    $$PWD/simpletasks.h


contains(DEFINES, BASENET_EXISTS){
HEADERS +=
}

SOURCES += \
    $$PWD/simpletasker.cpp \
    $$PWD/simpletasks.cpp

