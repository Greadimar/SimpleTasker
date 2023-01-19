!contains(DEFINES, __SIMPLETASKER__) {
DEFINES += __SIMPLETASKER__

CONFIG += c++17

HEADERS += \
    $$PWD/pausetask.h \
    $$PWD/simplereplytask.h \
    $$PWD/simpletask.h \
    $$PWD/simpletasker.h \
    $$PWD/taskcluster.h \
    $$PWD/tasks.h \
    $$PWD/tasktools.h \
    $$PWD/taskplanner.h


SOURCES += \
    $$PWD/pausetask.cpp \
    $$PWD/simplereplytask.cpp \
    $$PWD/simpletask.cpp \
    $$PWD/simpletasker.cpp \
    $$PWD/taskcluster.cpp \
    $$PWD/taskplanner.cpp
}
