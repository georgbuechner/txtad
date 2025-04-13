install: 
	export VCPKG_ROOT="vcpkg" && cmake --preset=default && cmake --build build

setup: 
	export VCPKG_DISABLE_METRICS && ./vcpkg/bootstrap-vcpkg.sh -disableMetrics

compilecommands: 
	export VCPKG_ROOT="vcpkg" && cmake --preset=default -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCOMPILE_FOR_SERVER=0 -DDEVELOPMENT=0 && cmake --build build

server: 
	export VCPKG_ROOT="vcpkg" && cmake --preset=default  -DCOMPILE_FOR_SERVER=1 && cmake --build build

development: 
	export VCPKG_ROOT="vcpkg" && cmake --preset=default -DCOMPILE_FOR_SERVER=1 -DDEVELOPMENT=1 && cmake --build build
