cmake -H. -Bcmake-build -A x64 -DHPX_DIR="C:/Repos/hpx/cmake-build-debug/lib/cmake/HPX"

pause
::cmake --build cmake-build --config Debug

cmake --open cmake-build
