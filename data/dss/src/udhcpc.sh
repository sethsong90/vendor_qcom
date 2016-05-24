#!/bin/ash

# udhcpc script edited by Tim Riker <Tim@Rikers.org>

# ---------------------------------------------------------------------------
# Copyright (c) 2007 Qualcomm Technologies, Inc.
# All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
# ---------------------------------------------------------------------------

[ -z "$1" ] && echo "Error: should be called from udhcpc" && exit 1

RESOLV_CONF="/etc/resolv.conf"
[ -n "$broadcast" ] && BROADCAST="broadcast $broadcast"
[ -n "$subnet" ] && NETMASK="netmask $subnet"

case "$1" in
	deconfig)
		#/sbin/ifconfig $interface 0.0.0.0
		#echo "/sbin/ifconfig $interface 0.0.0.0"
		# echo "ip addr del $ip dev $interface"
		# ip addr del $ip dev $interface
		ip addr flush dev $interface
		echo "ip addr flush dev $interface"
		;;

	renew|bound)
#	/sbin/ifconfig $interface $ip $BROADCAST $NETMASK
		ip addr add ${ip}/${mask} dev $interface
		echo "ip addr add ${ip}/${mask} dev $interface"
		echo "ip route add default via $router"
		ip route add default via $router

#		if [ -n "$router" ] ; then
#			echo "deleting routers"
#			while route del default gw 0.0.0.0 dev $interface ; do
#				:
#			done
#
#			for i in $router ; do
#				route add default gw $i dev $interface
#			done
#		fi

		echo -n > $RESOLV_CONF
		[ -n "$domain" ] && echo search $domain >> $RESOLV_CONF
		for i in $dns ; do
			echo adding dns $i
			echo nameserver $i >> $RESOLV_CONF
		done
		;;
esac

exit 0
