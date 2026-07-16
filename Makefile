debug:
	cmake -B build/debug -DCMAKE_BUILD_TYPE=Debug
	cmake --build build/debug

release:
	cmake -B build/release -DCMAKE_BUILD_TYPE=Release
	cmake --build build/release

clean:
	rm -rf build bin