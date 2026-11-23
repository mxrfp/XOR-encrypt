# XOR_encrypt

A command-line tool written in Python and C++ to encrypt and decrypt files using XOR and a One-Time Pad (OTP) key.

## Project structure

```text
.
├── main.py
├── README.md
├── key_gen/
│   ├── env.txt
│   ├── key_gen.cpp
│   └── key_gen.exe
└── scr/
    ├── get_data.py
    ├── state.json
    ├── key/
    │   ├── key_A.txt
    │   └── key_B.txt
    ├── py_decrypt/
    └── py_encrypt/
```

- `main.py`: CLI entry point to encrypt or decrypt files.
- `key_gen/`: C++ multi-threaded key generator (`key_gen.cpp` / `key_gen.exe`) configured via `env.txt`.
- `scr/get_data.py`: handles file paths and cursor state updates.
- `scr/state.json`: stores the encrypt and decrypt cursor offsets so key bytes are never reused.
- `scr/key/`: folder where you put `key_A.txt` (encrypt key) and `key_B.txt` (decrypt key).
- `scr/py_encrypt/` & `scr/py_decrypt/`: chunked XOR encryption and decryption modules.

## Requirements

- Python 3.8 or higher (tested on Python 3.11)
- Windows (for `key_gen.exe`)

## How to set it up and use it

### Step 1: Generate the keys (`key_gen`)

Before encrypting anything, you need two key files. You can generate them with `key_gen/key_gen.exe`, which reads its settings from `key_gen/env.txt`.

`env.txt` must have 5 lines in this exact order:

```text
<unit for key size: byte or gb>
<size_key>
<unit for chunk size: byte or gb>
<size_chunk>
<n_threads>
```

Example `env.txt` (generates a 2 GB key using 10,000-byte chunks on 8 threads):

```text
gb
2
byte
10000
8
```

Edit `key_gen/env.txt`, run `key_gen.exe` to generate your key files, and move them into `scr/key/`.

### Step 2: Put the two key files in `scr/key/`

Inside the `scr/key/` folder you must have 2 files named:
- `key_A.txt`: used for encryption (`e` / `encrypt`)
- `key_B.txt`: used for decryption (`d` / `decrypt`)

How to set them up depending on your use case:
- **Between two users (User 1 and User 2):** generate two keys (Key 1 and Key 2) and swap them between the two devices.
  - User 1 puts Key 1 as `key_A.txt` (encrypt) and Key 2 as `key_B.txt` (decrypt).
  - User 2 puts Key 2 as `key_A.txt` (encrypt) and Key 1 as `key_B.txt` (decrypt).
  In other words, your `key_A.txt` must be the exact same file as the other user's `key_B.txt`, and vice versa.
- **Testing locally on one PC:** if you just want to encrypt and decrypt your own files on the same computer to test the tool, copy the same generated key as both `key_A.txt` and `key_B.txt`.

### Step 3: Run `main.py`

1. `main.py` reads the target file in chunks and XORs it with the next unused chunk from `key_A.txt` (when encrypting) or `key_B.txt` (when decrypting).
2. After each operation, it saves the updated cursor position in `scr/state.json` so the same key bytes are never reused.

## Options (`-h`)

Run the help command to see all available arguments:

```bash
python main.py -h
```

Output of `-h`:

```text
usage: Encrypt or decrypt eny file. [-h] [-r] [-c CHUNK_SIZE] dir mode

positional arguments:
  dir                   File path
  mode                  Selected mode (d/e/decrypt/encrypt)

options:
  -h, --help            show this help message and exit
  -r, --reset           reset file cursor
  -c CHUNK_SIZE, --chunk-size CHUNK_SIZE
                        Select size (in bytes) of a chunk
```

Arguments overview:
- `dir`: path to the file you want to encrypt or decrypt.
- `mode`: operation mode (`e` or `encrypt` to encrypt, `d` or `decrypt` to decrypt).
- `-r` / `--reset`: resets the cursor in `scr/state.json` back to 0 before running (useful for testing).
- `-c` / `--chunk-size`: sets the chunk size in bytes (default is `1024`).

## Usage examples

Encrypt a text file (uses `scr/key/key_A.txt`):

```bash
python main.py message.txt e
```

Or using the full mode name:

```bash
python main.py message.txt encrypt
```

Decrypt a received file (uses `scr/key/key_B.txt`):

```bash
python main.py encrypted_message.txt d
```

Or:

```bash
python main.py encrypted_message.txt decrypt
```

Use a larger chunk size (e.g. 64 KB) to process bigger files faster:

```bash
python main.py video.mp4 e -c 65536
```

Reset the cursor to 0 and encrypt a file from the start (for testing only):

```bash
python main.py test.txt e -r
```

## Notes

- Encrypted files should be decrypted in the same order they were encrypted so both `state.json` cursors stay in sync.
- Do not use `-r` (`--reset`) on real messages with an already-used key: reusing the same XOR key bytes across different files breaks security.
- Make sure `key_A.txt` and `key_B.txt` are larger than the total amount of data you want to encrypt/decrypt, otherwise the program will stop once it runs out of key bytes.
- Keep `scr/key/` and `scr/state.json` in `.gitignore` so you don't accidentally upload your keys to GitHub.
