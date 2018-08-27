#!/usr/bin/env bash

sysctl vfs.write_behind
sysctl vm.pmap.pg_ps_enabled
sysctl vm.disable_swapspace_pageouts
sysctl vfs.zfs.arc_max
sysctl vfs.zfs.arc_min
sysctl hw.ncpu
sysctl machdep.hyperthreading_allowed

POOL0=test2
POOL1=zroot/test3

zfs get mountpoint ${POOL0}/yyy_INPUT ${POOL0}/xxx_HT
zfs get mountpoint ${POOL1}/yyy_INPUT ${POOL1}/xxx_HT
zfs get compression ${POOL0}/yyy_INPUT ${POOL0}/xxx_HT
zfs get compression ${POOL1}/yyy_INPUT ${POOL1}/xxx_HT
zfs get atime ${POOL0}/yyy_INPUT ${POOL0}/xxx_HT
zfs get atime ${POOL1}/yyy_INPUT ${POOL1}/xxx_HT
zfs get recordsize ${POOL0}/yyy_INPUT ${POOL0}/xxx_HT
zfs get recordsize ${POOL1}/yyy_INPUT ${POOL1}/xxx_HT


