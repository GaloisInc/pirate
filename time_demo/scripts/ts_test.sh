#!/bin/bash

set -x

IN=${0}
REQ=/tmp/${IN}.req
TSR=/tmp/${IN}.tsr
CONF=pki/tsa.conf
TSA_CA=../ca/tsa_ca.pem
SECT=tsa
TS_TEST_BIN=./ts_test

#OPENSSL_DIR=<your_path>/openssl/build
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${OPENSSL_DIR}/lib
#OPENSSL=${OPENSSL_DIR}/bin/openssl

OPENSSL=openssl
VALGRIND_CMD="valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes"

${OPENSSL} version
${OPENSSL} ts -query -config ${CONF} -cert -data ${IN} -out ${REQ}
${OPENSSL} ts -reply -config ${CONF} -section ${SECT} -queryfile ${REQ} -out ${TSR}
${OPENSSL} ts -verify -config ${CONF} -section ${SECT} -CAfile ${TSA_CA} -data ${IN} -in ${TSR}

${VALGRIND_CMD} ${TS_TEST_BIN} -vv ${TS_TEST_BIN}

${VALGRIND_CMD} ${TS_TEST_BIN} -n 10 ${TS_TEST_BIN}