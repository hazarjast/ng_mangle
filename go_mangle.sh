#!/bin/sh

case "$2" in
start)
	/sbin/kldload ./ng_mangle.ko 			2> /dev/null
	/sbin/kldload /boot/kernel/ng_ether.ko  	2> /dev/null
	/usr/sbin/ngctl mkpeer $1: mangle lower lower 	2> /dev/null
	/usr/sbin/ngctl name $1:lower mangle0 		2> /dev/null
	/usr/sbin/ngctl connect $1: mangle0: upper upper 2> /dev/null
	echo "Mangle node loaded!"
	;;
stop)
	/usr/sbin/ngctl shutdown mangle0: 		2> /dev/null
	/sbin/kldunload ng_mangle.ko 			2> /dev/null
	/sbin/kldunload ng_ether.ko 			2> /dev/null
	echo "Mangle node unloaded!"
	;;
ttl)
	/usr/sbin/ngctl msg mangle0: set_ttl_lower $3
	echo "TTL set to $3"
	;;
tos)
	/usr/sbin/ngctl msg mangle0: set_tos_lower $3
	echo "TOS set to $3"
	;;
*)
	echo "Usage: ${0##*/} interface { start | stop | ttl num | tos num }" >&2
	exit 64
	;;
esac

exit 0
