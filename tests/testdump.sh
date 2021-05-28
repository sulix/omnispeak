#!/bin/sh

EPISODE=4
OMNIDUMP=`mktemp`
KEENDUMP="../tests/demo${1}.dump${EPISODE}"

./omnispeak /EPISODE $EPISODE /PLAYDEMO $1 /DUMPFILE "$OMNIDUMP"

diff "$OMNIDUMP" "$KEENDUMP"
RES=$?

if [ $RES -ne 0 ] ; then
	mkdir -p ../log
	OMNIDUMPTXT="../log/omni${EPISODE}dump${1}.txt"
	KEENDUMPTXT="../log/keen${EPISODE}dump${1}.txt"
	echo "Dump was different for Episode $EPISODE, Demo $1"
	./dumpprinter "$OMNIDUMP" $EPISODE > "$OMNIDUMPTXT"
	./dumpprinter "$KEENDUMP" $EPISODE > "$KEENDUMPTXT"
	diff "$OMNIDUMPTXT" "$KEENDUMPTXT" > "../log/ep${EPISODE}demp${1}.diff"
	diff "$OMNIDUMPTXT" "$KEENDUMPTXT"
else
	echo "Dump matched for Episode $EPISODE, Demo $1"
fi

exit $RES
