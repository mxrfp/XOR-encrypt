import importlib.util
import traceback
from pathlib import Path

get_data_path = Path(__file__).resolve().parent.parent / "get_data.py"

spec = importlib.util.spec_from_file_location("get_data", get_data_path)
get_data = importlib.util.module_from_spec(spec)  # type: ignore
spec.loader.exec_module(get_data)  # type: ignore
key_path = r"=key\key.txt"


def decrypt(path: str, chunk_size):
    file_path = get_data.parse_file_path(path)
    file_name, ext = get_data.get_name_extension(file_path)
    new_file_path = Path.cwd() / ("decrypted_" + file_name + "." + ext)

    try:
        with open(file_path, "rb") as f:
            size = Path(file_path).stat().st_size

            if size + get_data.get_decrypt_counter() > get_data.get_key_size("B"):
                print("Key length exceded!")
                get_data.reset_state()
                return

            with open(new_file_path, "ba") as n_f:
                for chunk in get_data.get_chunked(f, file_path, "d", chunk_size):
                    file_chunk, key_chunk = chunk

                    n = len(file_chunk)
                    a = int.from_bytes(file_chunk, "little")
                    b = int.from_bytes(key_chunk, "little")
                    found_XOR = (a ^ b).to_bytes(n, "little")
                    n_f.write(found_XOR)

    except FileNotFoundError:
        print("File not found.")
        traceback.print_exc()
    except UnicodeDecodeError:
        print("File found is not valid.")
