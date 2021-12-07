git clone -b opencilk/v1.0 https://github.com/OpenCilk/infrastructure
infrastructure/tools/get $(pwd)/opencilk
infrastructure/tools/build $(pwd)/opencilk $(pwd)/build
cd $(pwd)/build

cmake -DCMAKE_INSTALL_PREFIX=$(dirname "$0") -P cmake_install.cmake
