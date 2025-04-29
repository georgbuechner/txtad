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


## Concept 

A basic room Context might look like this: 

```
Context: 
 id: "rooms.pool" 
 name: "Swimming Pool"
 entry_logic: "match = {this.id}"

 handlers: 
 - "exits": 
   - "swim out" -> "#ct contexts += rooms.meddow", "#contexts -= rooms.pool") 
   - "swin down" -> "print-text: occtopus-attack" 

 - "items": 
   - "pick (.*)" -> items.water_ballon -> "#contexts contexts += 

 - "other":
    - "go to" -> "print: you can't walk in water"
    - "show exits" -> "print: this.exists"
    - "show items" -> "print: this.items"

Context: 
 id: "rooms.meddow" 
 name: "Meddow"
 entry_logic: "match ~ meddow || match ~ lawn"

 handlers: 
 - "exits": 
   - "go (.*)" -> rooms.pool

 - "other":
    - "show exits" -> "print: this.exits"
    - "show exits" -> "print: this.items"

Context: 
  id: "items.water_ballon" 
  name: "Water Ballon" 
  entry_logic: "match ~ watter ballon " 

  handlers: 
  - "use": 
    - "use (.*)" 
```
