#!/bin/bash

set -x

openssl ts -verify -in $1.tsr -data $1.data -CAfile ../tsa/tsa_ca.pem -untrusted ../tsa/tsa_cert.pem
