#!/usr/bin/env bash


sysctl_normal(){
  sysctl -d $1
  sysctl -h $1
}

sysctl_in_GiB(){
  sysctl -d $1
  sysctl -h $1
  val=`sysctl -n $1`
  retval=`echo $val "/1024/1024/1024" | bc -l`
  echo $1"="$retval"GiB"
}

sysctl_in_KiB(){
  sysctl -d $1
  sysctl -h $1
  val=`sysctl -n $1`
  retval=`echo $val "/1024/1024" | bc -l`
  echo $1"="$retval"GiB"
}

sysctl_normal vfs.write_behind
sysctl_normal vm.pmap.pg_ps_enabled
sysctl_normal vm.disable_swapspace_pageouts

sysctl_in_GiB vfs.zfs.arc_max
sysctl_in_GiB vfs.zfs.arc_min

sysctl_normal hw.ncpu
sysctl_normal machdep.hyperthreading_allowed
sysctl_normal vfs.zfs.min_auto_ashift
sysctl_normal vfs.zfs.default_bs
sysctl_normal security.bsd.stack_guard_page
sysctl_normal vm.max_wired


sysctl_normal vfs.zfs.prefetch_disable
sysctl_normal vfs.zfs.vdev.async_write_max_active
sysctl_normal vfs.zfs.vdev.async_write_min_active
sysctl_normal vfs.zfs.vdev.async_read_max_active
sysctl_normal vfs.zfs.vdev.async_read_min_active
sysctl_normal vfs.zfs.vdev.sync_write_max_active
sysctl_normal vfs.zfs.vdev.sync_write_min_active
sysctl_normal vfs.zfs.vdev.sync_read_max_active
sysctl_normal vfs.zfs.vdev.sync_read_min_active

sysctl_normal vfs.zfs.vdev.async_write_active_max_dirty_percent
sysctl_normal vfs.zfs.vdev.async_write_active_min_dirty_percent



sysctl_normal vfs.zfs.delay_min_dirty_percent
sysctl_in_GiB vfs.zfs.dirty_data_max_max
sysctl_in_GiB vfs.zfs.dirty_data_max
sysctl_normal vfs.zfs.dirty_data_max_percent
sysctl_in_KiB vfs.zfs.max_recordsize
sysctl_normal vfs.zfs.delay_scale




sysctl_in_GiB vfs.zfs.dirty_data_sync
sysctl_normal vfs.zfs.top_maxinflight
sysctl_normal vfs.zfs.trim.txg_delay
sysctl_normal vfs.zfs.txg.timeout
sysctl_in_GiB vfs.zfs.vdev.aggregation_limit
sysctl_in_GiB vfs.zfs.vdev.write_gap_limit
sysctl_in_GiB vfs.read_max
sysctl_in_GiB vfs.zfs.vdev.cache.size
sysctl_normal vfs.zfs.vdev.cache.max
echo "spectre, Page Table Isolation  1 enabled(default), 0 disabled"
sysctl_normal vm.pmap.pti
echo "Indirect Branch Restricted Speculation"
sysctl_normal hw.ibrs_active
sysctl_normal hw.ibrs_disable
sysctl_normal kern.random.fortuna.minpoolsize
sysctl_normal kern.ipc.shm_use_phys


# test for drive alignment

SYSTEM_DRIVES=0
ENCRYPTED_DRIVES=0

get_drives(){
  bare_drives=`zpool status | grep -E 'nvd|ada|da'|grep -v data| awk '{print $1}' | cut -d. -f1`
  encrypted_drives=`zpool status | grep -E 'nvd|ada|da'|grep -v data| awk '{print $1}' | grep eli`
  SYSTEM_DRIVES="$bare_drives"
  ENCRYPTED_DRIVES="$encrypted_drives"
}

get_drives

echo "ZPOOL DRIVES"
echo $SYSTEM_DRIVES
echo "ENCRYPTED DRIVES"
echo $ENCRYPTED_DRIVES

test_4k_offset(){
  echo -n $1 " :%4k ="
  sudo diskinfo -v $1 | grep stripeoffset | awk '{print $1}' | xargs -J % echo % "%4096" | bc
}

for i in $SYSTEM_DRIVES;do test_4k_offset $i;done
for i in $ENCRYPTED_DRIVES;do test_4k_offset $i;done

# test for ashift in the pool(s)
test_ashift(){
  sudo zdb | grep -E "^[[:alnum:]]+|ashift" | tr -d "\n" | xargs echo
}

test_ashift

#
POOL0=test2
POOL1=zroot/test3
INPUT=INPUT
#INPUT=yyy_INPUT
HT=HT
#HT=xxx_HT

#zfs get mountpoint ${POOL0}/yyy_INPUT ${POOL0}/xxx_HT
zfs get mountpoint,compression,atime,recordsize ${POOL1}/INPUT ${POOL1}/HT ${POOL1}/INPUT/data ${POOL1}/INPUT/hashid ${POOL1}/INPUT/keyid ${POOL1}/HT/data ${POOL1}/HT/table

