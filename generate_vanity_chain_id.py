#!/usr/bin/env python3
#
# A totally unoptimized, singlethreaded and naive vanity chain ID generator
#
# Arguments: input_filename vanity_prefix
#
import hashlib
import os
import random
import re
import string
import sys


SEED_LEN = 90
SEED_CHARS = string.ascii_lowercase + string.ascii_uppercase + string.digits

try:
    filename = sys.argv[1]
    vanity_prefix = sys.argv[2].lower()
except IndexError:
    print("Usage: generate_vanity_chain_id.py <input_filename> <vanity_prefix>")
    exit(1)

with open(filename, 'r') as ifile:
    original = ifile.read()

while True:
    random_str = '{}{}'.format(vanity_prefix, ''.join(random.choice(SEED_CHARS) for _ in range(SEED_LEN - len(vanity_prefix))))
    new_file = re.sub(r'"initial_chain_id": "[0-9a-zA-Z]+"', r'"initial_chain_id": "{}"'.format(random_str), original).encode('ascii')
    if hashlib.sha256(new_file).hexdigest().startswith(vanity_prefix):
        with open(filename + '.vanity', 'wb') as out_file:
            out_file.write(new_file)
        break
