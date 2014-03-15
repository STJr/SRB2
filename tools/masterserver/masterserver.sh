#!/bin/sh -e
#
# SRB2 MasterServer - start up the masterserver
#

# Get LSB functions
. /lib/lsb/init-functions
. /etc/default/rcS

SRB2MS=/usr/local/bin/masterserver
SRB2MS_PORT=28900

# Check that the package is still installed
[ -x $SRB2MS ] || exit 0;

case "$1" in
	start)
		log_begin_msg "Starting SRB2MS..."
		umask 002
		if start-stop-daemon --start \
		--exec $SRB2MS \
		-- $SRB2MS_PORT; then
			log_end_msg 0
		else
			log_end_msg $?
		fi
	;;

	stop)
		log_begin_msg "Stopping SRB2MS..."
		if start-stop-daemon --stop --exec $SRB2MS; then
		log_end_msg 0
		else
		log_end_msg $?
		fi
	;;

	restart|force-reload)
		"$0" stop && "$0" start
	;;

	*)
	e	cho "Usage: /etc/init.d/masterserver {start|stop|restart|force-reload}"
		exit 1
	;;
esac

exit 0
