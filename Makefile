all: make

make:
	cd build ; cmake --build . -- -j 64 ; cd ..

normal:
	rm -rf build ; mkdir -p build ; cd build ; ../cmakung -DCMAKE_BUILD_TYPE=RelWithDebInfo .. ; cmake --build . -- -j 64 ; cd ..

release:
	rm -rf build ; mkdir -p build ; cd build ; ../cmakung -DCMAKE_BUILD_TYPE=Release .. ; cmake --build . -- -j 64 ; cd ..

debug:
	rm -rf build ; mkdir -p build ; cd build ; ../cmakung -DCMAKE_BUILD_TYPE=Debug .. ; cmake --build . -- -j 64 ; cd ..

clean:
	rm -rf build

