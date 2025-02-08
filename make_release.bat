@echo off
cmake -S . -B build_release/ -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=C://clang+llvm//bin//clang++.exe -DCMAKE_C_COMPILER=C://clang+llvm//bin//clang.exe
