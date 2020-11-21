@echo off

pushd build

set clang_warnings=-Wno-missing-field-initializers -Wno-deprecated-declarations -Wno-reserved-id-macro -Wno-global-constructors -Wno-cast-align -Wno-unused-variable -Wno-unused-macros -Wno-newline-eof -Wno-writable-strings -Wno-unused-parameter -Wno-c++98-compat -Wno-old-style-cast -Wno-c++20-designator -Wno-reorder-init-list -Wno-c++98-compat-pedantic -Wno-missing-prototypes -Wno-gnu-anonymous-struct -Wno-missing-braces

clang-cl -march=native /Od -I..\IACA /Z7 -Wall -Werror %clang_warnings% ..\code\main.cpp /link Winmm.lib user32.lib Gdi32.lib

popd