#!/bin/sh
# $Id: pdf.sh,v 1.8 2006/01/25 18:51:51 garbled Exp $
args=`getopt lng:m:t:c:w:x: $*`
if test $? != 0
then
	echo 'Usage: pdf [-ln] [-g nodegroup] [-m size] [-t type] [-c clusterfile] [-w node1,...,nodeN] [-x node1,...,nodeN] [file | file_system ...]'
	exit 2
fi
set -- $args
for i
do
	case "$i"
	in
		-l|-n)
			flag=`echo $flag $i`; shift;;
		-t)
			flag=`echo $flag $i $2`; shift; shift;;
		-c)
			carg=$2; shift; shift;;
		-w)
			warg=$2; shift; shift;;
		-g)
			garg=$2; shift; shift;;
		-x)
			xarg=$2; shift; shift;;
		-m)
			marg=$2; shift; shift;;
		--)
			shift; break;;
	esac
done

if [ -n "$warg" ]; then 
	dshargs=`echo "-w $warg"`
elif [ -n "$garg" ]; then
	dshargs=`echo "-g $garg"`
fi
if [ -n "$carg" ]; then
	dshargs=`echo "$dshargs -c $carg"`
fi
if [ -n "$xarg" ]; then
	dshargs=`echo "$dshargs -x $xarg"`
fi
if [ -z "$marg" ]; then
	marg=0;
fi

(
dsh $dshargs 'sh -c "if [ `uname` = "AIX" ]; then df -kI '$flag $*'; elif [ `uname` = "HP-UX" ]; then bdf '$flag $*'; elif [ `uname` = "Linux" ]; then df -Pk '$flag $*'; else df -k '$flag $*'; fi"'
)| grep -v Filesystem | sed -e 's/://' | awk '{print $1 " " $2 " " $3 " " $4 " " $5 " " $6 " " $7}' |(
echo 'Node      Filesystem            1K-Blks     Used    Avail  Cap Mounted On'
while read node fs blocks used avail capacity mount; do \
	capacity=`echo "$capacity" | sed -e 's/\%//'`;
	if [ $capacity -gt $marg ]; then \
		printf "%-8s: %-19.19s %9d%9d%9d %3.3s%% %-17.17s\n" $node $fs $blocks $used $avail $capacity $mount; \
	fi \
done
)
