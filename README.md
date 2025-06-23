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

## Concept 

### Stack-Structure 

- 15 to  10 -> these should all be completely permeable contexts that only catch
  stuff and f.e. add it to log or history entries 
- 10 to   0 -> Add custom permeable contexts here
-     0     -> Here we can find the mechanics context from here on should be
  non-permeable 
-  0 to -10 -> Add custom non-permeable contexts here
- -15       -> Add Error contexts here printing error messages.

