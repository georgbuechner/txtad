# TxtAd 
Context and Eventmanager Bases Textadventure Framework

Note: This repository has submodules. When cloning for the first time, run: 
```
git submodule init
git submodule update 
```

## Structure 
The repro consists of two parts, the *Textadventure* and the *Builder*.

The source code for the Textadventure is located at `game/` for the Builder at
`builder/`. Both share the same data-structures, stored at `shared/`. The games
itself are stored at `data/`.

## Game 

### Installation (Linux/Mac)
Go to the game-directory: 
```
cd game/ 
``` 

Setup vcpkg: 
``` 
make setup
make compilecommands 
``` 

Compile code:
``` 
make install 
```

Then the code can be run as:
``` 
./build/txtad.o
```
