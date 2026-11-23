import argparse
import sys
import json
from pathlib import Path

from scr.get_data import parse_file_path, reset_state
from scr.py_decrypt.XOR_decrypt import decrypt
from scr.py_encrypt.XOR_encrypt import encrypt

STATE_FILE = Path(__file__).resolve().parent / "scr" / "state.json"

if not STATE_FILE.exists():
  STATE_FILE.parent.mkdir(parents=True, exist_ok=True)
  default_state = {"decrypt_counter": 0, "encrypt_counter": 0}
  STATE_FILE.write_text(
      json.dumps(default_state, indent=4),
      encoding="utf-8",
  )

parser = argparse.ArgumentParser(description="Encrypt or decrypt any file.")

parser.add_argument("dir", type=str, help="File path")

parser.add_argument("mode", type=str, help="Selected mode (d/e/decrypt/encrypt)")

parser.add_argument(
    "-r",
    "--reset",
    action="store_true",
    help="reset file cursor",
)
parser.add_argument(
    "-c",
    "--chunk-size",
    type=int,
    default=1024,
    help="Select size (in bytes) of a chunk",
)

args = parser.parse_args()
path = parse_file_path(args.dir)

if args.reset:
    reset_state()

if args.mode == "d" or args.mode == "decrypt":
    decrypt(path, args.chunk_size)
elif args.mode == "e" or args.mode == "encrypt":
    encrypt(path, args.chunk_size)
else:
    print("Mode selected is not valid.")
    sys.exit(1)
