#!/usr/bin/env bash

sysctl vfs.write_behind
sysctl vm.pmap.pg_ps_enabled
sysctl vm.disable_swapspace_pageouts
sysctl vfs.zfs.arc_max
sysctl vfs.zfs.arc_min
sysctl hw.ncpu
sysctl machdep.hyperthreading_allowed

zfs get mountpoint test2/yyy_INPUT
zfs get compression test2/yyy_INPUT
zfs get atime test2/yyy_INPUT
zfs get recordsize test2/yyy_INPUT

zfs get mountpoint zroot/test3/yyy_INPUT
zfs get compression zroot/test3/yyy_INPUT
zfs get atime zroot/test3/yyy_INPUT
zfs get recordsize zroot/test3/yyy_INPUT

zfs get mountpoint test2/xxx_HT
zfs get compression test2/xxx_HT
zfs get atime test2/xxx_HT
zfs get recordsize test2/xxx_HT

zfs get mountpoint zroot/test3/xxx_HT
zfs get compression zroot/test3/xxx_HT
zfs get atime zroot/test3/xxx_HT
zfs get recordsize zroot/test3/xxx_HT






