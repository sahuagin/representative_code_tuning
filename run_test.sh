#!/usr/bin/env bash

TEST=$1
echo $TEST
NUMBER=$2
echo $NUMBER

ls -ahl ${TEST}
ls -ahlR testdata
OUTPUT_DIR=test_${NUMBER}
mkdir -p ./${OUTPUT_DIR}
${TEST}/../../get_settings.sh
time truss -f -o ${OUTPUT_DIR}/yyy_truss_${NUMBER}.txt ${TEST}/yyy testdata | grep user
echo du -ch testdata/INPUT
du -ch testdata/INPUT
echo du -Ah testdata/INPUT
du -Ah testdata/INPUT
time truss -f -o ${OUTPUT_DIR}/xxx_truss_${NUMBER}.txt ${TEST}/xxx testdata | grep user
echo du -ch testdata/HT
du -ch testdata/HT
echo du -Ah testdata/HT
du -Ah testdata/HT


