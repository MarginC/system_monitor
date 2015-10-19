# System Monitor
A multi-platform system monitor, it shows memory usage, cpu usage, file systems, processes and network interfaces through a web interface.

![Screenshot](http://i.imgur.com/vrCCj2n.png, "Screenshot")

## System Support
Tested on the following systems:
- Linux
- FreeBSD
- OSX

## Installing
__Prerequisites__
- GNU autotools/libtool
- pkg-config

To compile run
```
make
```

This will compile all dependencies and sources.

To install
```
make install
```
To handle a daemon web server, the following cmd can be used:
- /etc/init.d/system_monitor start, that will start the server on port 3000.
- /etc/init.d/system_monitor stop
- /etc/init.d/system_monitor restart

## Todos
- Make compilation easier and more portable (maybe use autoconf and automake)
- A way to configure
  - User password
  - HTTP port
  - Refresh timeout
  - What information to display
- User auth
- Provide installers for common systems like debian
- Process detailed information
- Provide historic cpu, network and memory
