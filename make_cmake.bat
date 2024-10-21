@echo off
cmake -S . -B build/ -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=C://clang+llvm//bin//clang++.exe -DCMAKE_C_COMPILER=C://clang+llvm//bin//clang.exe
