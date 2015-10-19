#!/bin/bash

# Source function library.
. /etc/init.d/functions

prog=system_monitor
lockfile=/var/lock/subsys/$prog

start() {
	[ -e $lockfile ] && exit 2
	[ "$EUID" != "0" ] && exit 4
	[ -x $prog ] || exit 5

    # Start daemons.
    echo -n $"Starting $prog: "
    daemon /usr/local/bin/$prog -d $OPTIONS
	RETVAL=$?
        echo
	[ $RETVAL -eq 0 ] && touch $lockfile
	return $RETVAL
}

stop() {
	[ "$EUID" != "0" ] && exit 4
        echo -n $"Shutting down $prog: "
	killproc $prog
	RETVAL=$?
        echo
	[ $RETVAL -eq 0 ] && rm -f $lockfile
	return $RETVAL
}

# See how we were called.
case "$1" in
  start)
	start
	;;
  stop)
	stop
	;;
  status)
	status $prog
	;;
  restart)
	stop
	start
	;;
  *)
	echo $"Usage: $0 {start|stop|status|restart}"
	exit 2
esac
