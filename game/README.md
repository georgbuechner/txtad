# TXTAD

## Installation 
After cloning this project, run: 
```
git submodule init 
git submodule update 
``` 
to pull the submodules (especially vcpkg)

Build vcpkg: 
```
make setup 
``` 

Build code: 
```
make install 
```

TxtAd can now be run with: `./build/txtad.o`
