SHELL := /bin/bash

install: 
	export VCPKG_ROOT="vcpkg" && cmake --preset=default && cmake --build build

setup: 
	export VCPKG_DISABLE_METRICS && ./vcpkg/bootstrap-vcpkg.sh -disableMetrics

compilecommands: 
	export VCPKG_ROOT="vcpkg" && cmake --preset=default -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCOMPILE_FOR_SERVER=0 && cmake --build build

server: 
	export VCPKG_ROOT="vcpkg" && cmake --preset=default  -DCOMPILE_FOR_SERVER=1 && cmake --build build

deploy: 
	echo -e "MOVING BUILD FILES"
	mkdir /usr/bin/txtad/
	cp -f -r game/ /usr/bin/txtad/
	cp -f -r builder/ /usr/bin/txtad/
	cp -f -r data/ /usr/bin/txtad/ 
	cp -f txtad-builder.nginx /etc/nginx/sites-availible/
	cp -f txtad-game.nginx /etc/nginx/sites-availible/
	cp -f txtad-builder.service /etc/systemd/system/
	cp -f txtad-game.service /etc/systemd/system/
	echo -e "Restart systemd service"
	systemctl daemon-reload
	systemctl stop txtad-builder.service
	systemctl stop txtad-game.service
	systemctl start txtad-builder.service
	systemctl start txtad-game.service

update: 
	echo -e "MOVING BUILD FILES"
	mkdir -p /usr/bin/txtad/
	cp -f game/txtad.o /usr/bin/txtad/game/
	cp -f -r game/web/ /usr/bin/txtad/game/
	cp -f builder/builder.o /usr/bin/txtad/builder/
	cp -f -r builder/web/ /usr/bin/txtad/builder/
	cp -f txtad-builder.nginx /etc/nginx/sites-availible/
	cp -f txtad-game.nginx /etc/nginx/sites-availible/
	cp -f txtad-builder.service /etc/systemd/system/
	cp -f txtad-game.service /etc/systemd/system/
	echo -e "Restart systemd service"
	systemctl daemon-reload
	systemctl stop txtad-builder.service
	systemctl stop txtad-game.service
	systemctl start txtad-builder.service
	systemctl start txtad-game.service
