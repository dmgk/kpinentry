all: debug

debug:
	@mkdir -p build/debug ; \
		cd build/debug && cmake ../.. -DCMAKE_BUILD_TYPE=Debug && make

release:
	@mkdir -p build/release ; \
		cd build/release && cmake ../.. -DCMAKE_BUILD_TYPE=Release && make

clean:
	@rm -rf build/debug/* build/release/*

.PHONY: all debug release clean
