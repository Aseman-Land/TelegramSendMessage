QT += core gui widgets telegram network
ios|osx|clang: QMAKE_CXXFLAGS += -Wno-narrowing

TARGET = TelegramSendMessage
TEMPLATE = app

CONFIG += c++11

SOURCES += \
    main.cpp \
    sharedialog.cpp

HEADERS += \
    sharedialog.h

FORMS += \
    sharedialog.ui \
    proxy.ui \
    about.ui \
    dialog.ui

RESOURCES += \
    resource.qrc
