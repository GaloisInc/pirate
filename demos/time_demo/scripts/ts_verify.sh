#!/bin/bash

set -x

openssl ts -verify -in $1.tsr -data $1.data -CAfile ../ca/tsa_ca.pem -untrusted pki/tsa_cert.pem
