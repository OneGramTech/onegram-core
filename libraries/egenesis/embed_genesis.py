import argparse
import re

from embed_genesis_lib import convert_to_c_array, replace_with_dictionary, read_file_data, GENESIS_FILE_BANNER, \
    GENESIS_ARRAY_WIDTH

parser = argparse.ArgumentParser()
parser.add_argument("-t", "--tmplsub", action="append", required=True,
                    help="Given argument of form src.cpp.tmpl---dest.cpp, " +
                         "write dest.cpp expanding template invocations in src")

# This is not required in the original `embed_genesis` binary,
# but we expect an existing json file to be provided,
# as we do not support creating an example json file
parser.add_argument("-g", "--genesis-json", required=True, help="File to read genesis state from")
parser.add_argument("--chain-id", help="Force chain-id")

args = parser.parse_args()

print("   Running embed_genesis file generation (Python)")

json_hash, length, genesis_c_array = read_file_data(args.genesis_json)

args.chain_id = json_hash if args.chain_id is None else args.chain_id

template_substitution = {
    b"generated_file_banner": GENESIS_FILE_BANNER.encode("utf-8"),
    b"chain_id": args.chain_id.encode("utf-8"),
    b"genesis_json_length": str(length).encode("utf-8"),
    b"genesis_json_array": convert_to_c_array(genesis_c_array),
    b"genesis_json_hash": json_hash.encode("utf-8"),
    b"genesis_json_array_width": str(GENESIS_ARRAY_WIDTH).encode("utf-8"),
    b"genesis_json_array_height": str(-(-length // GENESIS_ARRAY_WIDTH)).encode("utf-8")  # ceil(length/width)
}

# do the substitution for all the files
for file_subs in args.tmplsub:
    sub_params = file_subs.split("---", 1)
    if len(sub_params) < 2:
        break

    source_file_path, dest_file_path, *rest = sub_params

    print("    Processing %s --- %s" % (source_file_path, dest_file_path))

    with open(source_file_path, "rb") as f:
        source_template = f.read()

    substituted_file = re.sub(b"\${(.+?)}",
                              lambda match_obj: replace_with_dictionary(match_obj, template_substitution),
                              source_template)

    with open(dest_file_path, "wb") as f:
        f.truncate()
        f.write(substituted_file)

print("   Genesis hash: %s" % json_hash)
print("   Chain-id: %s" % args.chain_id)
