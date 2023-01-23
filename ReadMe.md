w801 SDK

install miniterm:
	sudo apt install python3-serial
set permissions:
	chmod 755 tools/w800/mconfig.sh
add user to dialout group (and relogin)
	sudo -E usermod -a -G pulse $LOGNAME
configure port and toolchain path
	make menuconfig

