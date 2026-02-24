run-clang-tidy -p . \
  -extra-arg=-std=c++17 \
  -extra-arg=--gcc-toolchain=/usr \
  source/*.cpp

