change Cmakelist -> cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=arm-none-eabi-gcc.cmake 
build -> ninja -C build
