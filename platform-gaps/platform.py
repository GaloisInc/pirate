#!/usr/bin/env python3

#
# This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
# agreement with Galois, Inc.  This material is based upon work supported by
# the Defense Advanced Research Projects Agency (DARPA) under Contract No.
# HR0011-19-C-0103.
#
# The Government has unlimited rights to use, modify, reproduce, release,
# perform, display, or disclose computer software or computer software
# documentation marked with this legend. Any reproduction of technical data,
# computer software, or portions thereof marked with this legend must also
# reproduce this marking.
#
# Copyright 2019 Two Six Labs, LLC.  All rights reserved.
#

import hashlib
import hmac
import glob
import os
import shutil
import string
import subprocess
import sys
import tempfile
import uuid

TEMPLATE = """
#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "picohash.h"

${headers}

#define NUM_PROGRAMS (${count})

static unsigned char *programs[NUM_PROGRAMS];
static unsigned int program_lengths[NUM_PROGRAMS];
static char *program_names[NUM_PROGRAMS];
static char *program_ids[NUM_PROGRAMS];
static unsigned int program_id_lengths[NUM_PROGRAMS];

void to_hexdigest(unsigned char *digest, char *hexdigest, int len) {
    int i;
    for (i = 0; i < len; i++) {
        sprintf(hexdigest, "%02x", digest[i]);
        hexdigest += 2;
    }
}

int main(int argc, char *argv[]) {
    int i, memfd, secfd, seclen, program_index, count;
    unsigned char *buf;
    unsigned int remain;
    char *secret;
    picohash_ctx_t ctx;
    unsigned char digest[PICOHASH_SHA256_DIGEST_LENGTH];
    char hexdigest[PICOHASH_SHA256_DIGEST_LENGTH * 2 + 1];

    ${program_bytes}
    ${program_lengths}
    ${program_names}
    ${program_ids}
    ${program_id_lengths}

    memset(hexdigest, 0, PICOHASH_SHA256_DIGEST_LENGTH * 2 + 1);
    memfd = memfd_create("child", MFD_CLOEXEC);
    if (memfd < 0) {
        perror("memfd_create failed: ");
        return 1;
    }

    secfd = open("${secretfile}", O_RDONLY);
    if (secfd < 0) {
        perror("${secretfile} open failed: ");
        return 2;
    }
    seclen = lseek(secfd, 0, SEEK_END);
    if (seclen < 0) {
        perror("${secretfile} seek failed: ");
        return 3;
    }
    if (seclen > (1 << 20)) {
        fprintf(stderr, "${secretfile} too large\\n");
        return 4;
    }
    secret = calloc(seclen + 1, sizeof(char));
    if (secret == NULL) {
        perror("${secretfile} allocation failed: ");
        return 5;
    }
    lseek(secfd, 0, SEEK_SET);
    buf = secret;
    remain = seclen;
    while (remain > 0) {
        count = read(secfd, buf, remain);
        if (count < 0) {
            perror("${secretfile} read failed: ");
            return 6;
        }
        buf += count;
        remain -= count;
    }
    if (secret[seclen - 1] == '\\n') {
        secret[seclen - 1] = 0;
        seclen -= 1;
    }

    picohash_init_hmac(&ctx, picohash_init_sha256, secret, seclen);
    picohash_update(&ctx, "identifier", 10);
    picohash_final(&ctx, digest);
    free(secret);
    to_hexdigest(digest, hexdigest, PICOHASH_SHA256_DIGEST_LENGTH);
    seclen = PICOHASH_SHA256_DIGEST_LENGTH * 2;
    program_index = -1;
    for (i = 0; i < NUM_PROGRAMS; i++) {
        if (program_id_lengths[i] != seclen) {
            continue;
        }
        if (strncmp(hexdigest, program_ids[i], seclen) == 0) {
            program_index = i;
            break;
        }
    }

    if (program_index == -1) {
        fprintf(stderr, "no matching hardware identifier found\\n");
        return 7;
    }

    buf = programs[program_index];
    remain = program_lengths[program_index];

    while (remain > 0) {
        count = write(memfd, buf, remain);
        if (count < 0) {
            perror("memfd write failed: ");
            return 8;
        }
        buf += count;
        remain -= count;
    }

    argv[0] = program_names[program_index];

    if (fexecve(memfd, argv, environ)) {
        perror("fexecve failed: ");
        return 9;
    }
    // should never get here
    return 10;
}
"""


class Config:
    def __init__(self):
        self.program_to_id = {}
        self.executable = ""
        self.secretfile = ""


def is_exe(fpath):
    return os.path.isfile(fpath) and os.access(fpath, os.X_OK)


def which(program):
    fpath, _ = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None


def build_config():
    if len(sys.argv) % 2 == 0:
        sys.exit("must provide an even number of command-line options")
    config = Config()
    i = 3
    program_to_id = {}
    id_to_program = {}
    program_names = set()
    config.executable = sys.argv[1]
    config.secretfile = sys.argv[2]
    while i < len(sys.argv):
        prog = sys.argv[i]
        ident = sys.argv[i + 1]
        if not is_exe(prog):
            sys.exit(prog + " is not executable")
        prog_name = os.path.basename(prog)
        if prog in program_to_id:
            sys.exit("duplicate program " + prog)
        if prog_name in program_names:
            sys.exit("duplicate program name " + prog_name)
        if ident in id_to_program:
            sys.exit("duplicate identifier " + ident)
        program_to_id[prog] = ident
        id_to_program[ident] = prog
        program_names.add(prog_name)
        i += 2
    config.program_to_id = program_to_id
    return config


def create_blobs(config, tempdir):
    if which("xxd") is None:
        sys.exit("xxd is not found in $PATH")
    shutil.copyfile("picohash.h", os.path.join(tempdir, "picohash.h"))
    for key in config.program_to_id.keys():
        progname = os.path.join(tempdir, os.path.basename(key) + ".c")
        with open(progname, "w") as outfile:
            subprocess.run(["xxd", "-i", key], stdout=outfile)


def create_template(config, tempdir):
    subs = {'headers': '',
            'count': '',
            'program_bytes': '',
            'program_lengths': '',
            'program_names': '',
            'program_ids': '',
            'program_id_lengths': '',
            'secretfile': ''}
    program_to_id = config.program_to_id
    subs['count'] = str(len(program_to_id))
    subs['secretfile'] = config.secretfile

    for idx, key in enumerate(program_to_id.keys()):
        progname = os.path.basename(key)
        val = program_to_id[key]
        message = bytes('identifier', 'utf-8')
        secret = bytes(val, 'utf-8')
        val_hash = hmac.new(secret, message, hashlib.sha256).hexdigest()

        subs['headers'] += "extern unsigned char {}[];\n".format(progname)
        subs['headers'] += "extern unsigned int {}_len;\n".format(progname)

        if idx > 0:
            sep = "    "
            subs['program_bytes'] += sep
            subs['program_lengths'] += sep
            subs['program_names'] += sep
            subs['program_ids'] += sep
            subs['program_id_lengths'] += sep

        subs['program_bytes'] += "programs[{}] = {};\n".format(idx, progname)
        subs['program_lengths'] += "program_lengths[{}] = {}_len;\n".format(
            idx, progname)
        subs['program_names'] += 'program_names[{}] = "{}";\n'.format(
            idx, progname)
        subs['program_ids'] += 'program_ids[{}] = "{}";\n'.format(
            idx, val_hash)
        subs['program_id_lengths'] += "program_id_lengths[{}] = {};\n".format(
            idx, len(val_hash))

    template = string.Template(TEMPLATE)
    print(template.substitute(subs))
    filename = uuid.uuid4().hex + ".c"
    outfile = open(os.path.join(tempdir, filename), "w")
    outfile.write(template.substitute(subs))
    outfile.close()


def compile_executable(config, tempdir):
    srcs = glob.glob(os.path.join(tempdir, "*.c"))
    if os.getenv("CC") is None:
        cmd = "gcc"
    else:
        cmd = os.getenv("CC")
    if which(cmd) is None:
        sys.exit(cmd + " not found in $PATH")
    subprocess.run([cmd, "-o", config.executable] + srcs)


def main():
    if len(sys.argv) == 1:
        print()
        print(
            "platform.py: [generated executable] [identifier file] [prog1] [id1] [prog2] [id2] ...")
        print()
        sys.exit(0)
    with tempfile.TemporaryDirectory(prefix="platform-", suffix="-gaps") as tempdir:
        config = build_config()
        create_blobs(config, tempdir)
        create_template(config, tempdir)
        compile_executable(config, tempdir)


if __name__ == "__main__":
    main()
