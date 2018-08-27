#!/usr/bin/env bash

COMPRESSION=lz4
ATIME=off
RECORDSIZE_SEQUENTIAL=128K
RECORDSIZE_RANDOM=4K


BASE_POOL=$1
MOUNTPOINT=${2}
INPUT_BASE=${BASE_POOL}/yyy_INPUT
I_HASH=${INPUT_BASE}/hashid
I_KEY=${INPUT_BASE}/keyid
I_DATA=${INPUT_BASE}/data
MOUNT_YYY=${3:-"false"}
MOUNT_XXX=${4:-"false"}



HT_BASE=${1}/xxx_HT
H_TAB=${HT_BASE}/table
H_DATA=${HT_BASE}/data



echo "BASE_POOL=${BASE_POOL}"
echo "MOUNTPOINT=${MOUNTPOINT}"
echo "INPUT_BASE=${INPUT_BASE}"
echo "HT_BASE=${HT_BASE}"
echo "mount INPUT=${MOUNT_YYY}"
echo "mount HT=${MOUNT_XXX}"

if [ "$BASE_POOL" == "" ]; then
  echo "argv[1], basepool, must be populated.";
  exit 1
fi

if [ "$MOUNTPOINT" == "" ]; then
  echo "argv[2], mountpoint, must be populated.";
  exit 1
fi





echo sudo zfs create ${INPUT_BASE}
echo sudo zfs set compression=${COMPRESSION} ${INPUT_BASE}
echo sudo zfs set atime=${ATIME} ${INPUT_BASE}
echo sudo zfs set recordsize=${RECORDSIZE_SEQUENTIAL} ${INPUT_BASE}
if [ ${MOUNT_YYY} = true ]; then
  echo sudo zfs set mountpoint=${MOUNTPOINT}/INPUT
fi
echo sudo zfs create ${I_KEY}
echo sudo zfs set recordsize=${RECORDSIZE_RANDOM} ${INPUT_BASE}
echo sudo zfs create ${I_HASH}
echo sudo zfs create ${I_DATA}

echo sudo zfs create ${HT_BASE}
echo sudo zfs set compression=${COMPRESSION} ${INPUT_BASE}
echo sudo zfs set atime=${ATIME} ${INPUT_BASE}
echo sudo zfs set recordsize=${RECORDSIZE_SEQUENTIAL} ${INPUT_BASE}
if [ ${MOUNT_XXX} = true ]; then
  echo sudo zfs set mountpoint=${MOUNTPOINT}/HT
fi
echo sudo zfs create ${H_TAB}
echo sudo zfs create ${H_DATA}

