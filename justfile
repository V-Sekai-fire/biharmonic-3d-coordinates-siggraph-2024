build:
    mkdir -p build
    cd build && cmake -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
    cmake --build build
    build/ex_bhc3d.exe

clean:
    rm -rf build
