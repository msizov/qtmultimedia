CONFIG += testcase
TARGET = tst_qmediaplayer

QT += network multimedia-private testlib
CONFIG += no_private_qt_headers_warning

HEADERS += tst_qmediaplayer.h
SOURCES += main.cpp tst_qmediaplayer.cpp

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockplayer.pri)