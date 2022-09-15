TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        lexer.cpp \
        lexer_test_open.cpp \
        main.cpp \
        parse.cpp \
        parse_test.cpp \
        runtime.cpp \
        runtime_test.cpp \
        statement.cpp \
        statement_test.cpp

HEADERS += \
  lexer.h \
  parse.h \
  runtime.h \
  statement.h \
  test_runner_p.h
