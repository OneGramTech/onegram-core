import argparse
import glob
import os

parser = argparse.ArgumentParser()
parser.add_argument("dir", nargs=1)
parser.add_argument("output_file", nargs=1)

try:
    NotFoundError = FileNotFoundError
except NameError:
    NotFoundError = IOError

args = parser.parse_args()

print("   Running cat-parts file generation (Python)")

source_files = sorted(glob.glob(args.dir[0] + "/*.hf"))

output_data = bytearray()

for source_file in source_files:
    with open(source_file, "rb") as f:
        output_data += f.read()

old_data = None

print("   Processing %d files" % len(source_files))

try:
    with open(args.output_file[0], "rb") as f:
        old_data = f.read()
except NotFoundError:
    pass

if output_data == old_data:
    print("    File %s up-to-date with .d directory" % args.output_file[0])
    raise SystemExit(0)

output_dir = os.path.dirname(args.output_file[0])

if not os.path.exists(output_dir):
    os.makedirs(output_dir)

with open(args.output_file[0], "wb") as f:
    f.truncate()
    f.write(output_data)

print("    Built %s from .d directory" % args.output_file[0])
