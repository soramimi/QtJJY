
TARGET = QtJJY
TEMPLATE = app
QT += core gui multimedia widgets
CONFIG += c++11

SOURCES += \
	main.cpp \
	MainWindow.cpp \
	TimeCodeWidget.cpp

HEADERS += \
	MainWindow.h \
	TimeCodeWidget.h

FORMS += \
	MainWindow.ui
