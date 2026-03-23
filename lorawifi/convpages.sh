#!/bin/bash
#
# ------------------------------------------------------------------------
# Re run if changed any M4 macro / page.
# ------------------------------------------------------------------------
#
# This should be a Makefile, but missed the boat ... as ...
# the python script will compare file dates for us. The advantage is that
# this script autoscans new files for us.
#

# ------------------------------------------------------------------------
# Process file and dependencies

got=0       # Flag if we did anything

function xprocess {
    aa=$1; shift;   bb=$1; shift
    #echo xprocess: $aa to: $bb;
    #echo $*
    if [ $(isnewer.py -e $aa $bb $*) == 1 ]; then
        echo Convert: $bb to: $aa
        m4 $bb > $aa
        got=1
    fi
}

# Save old dir
pushd $(pwd) >/dev/null
cd main/m4

# Gather thoughts (really .. files)
DEPS=`find mac -maxdepth 1`
SRCS=`find . -maxdepth 1 -name "*.m4"`
#FILES=$(ls ../html)

# Testing below
#echo DEPS: $DEPS
#echo SRCS: $SRCS
#popd        > /dev/null; exit

for aa in $SRCS; do
    #echo aa: $aa
    NNN+="`basename  $aa .m4` "
done

# Testing below
#echo NNN: $NNN
#popd        > /dev/null; exit

for bb in $NNN; do
    #echo bb: $bb
    xprocess "../html/$bb.html" "$bb.m4" $DEPS
done

#if [ $got == 1 ]; then
#    echo        # If we filled up lines, add a return
#fi

# Restore old dir
popd        > /dev/null

#echo Done.
