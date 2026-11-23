import json
from pathlib import Path


def get_decrypt_counter():
    with open(r"scr\state.json", "r", encoding="utf-8") as j:
        data = json.load(j)

    return data["decrypt_counter"]


def get_encrypt_counter():
    with open(r"scr\state.json", "r", encoding="utf-8") as j:
        data = json.load(j)

    return data["encrypt_counter"]


def add_decrypt_counter(n):
    with open(r"scr\state.json", "r", encoding="utf-8") as j:
        data = json.load(j)

    data["decrypt_counter"] = data.get("decrypt_counter", 0) + n

    with open(r"scr\state.json", "w", encoding="utf-8") as j:
        json.dump(data, j, indent=4, ensure_ascii=False)


def add_encrypt_counter(n):
    with open(r"scr\state.json", "r", encoding="utf-8") as j:
        data = json.load(j)

    data["encrypt_counter"] = data.get("encrypt_counter", 0) + n

    with open(r"scr\state.json", "w", encoding="utf-8") as j:
        json.dump(data, j, indent=4, ensure_ascii=False)


def reset_state(path="..\\state.json"):
    add_decrypt_counter(-get_decrypt_counter())
    add_encrypt_counter(-get_encrypt_counter())


def parse_file_path(p: str) -> str:
    to_return = p.strip()

    if to_return[0] == '"':
        to_return = to_return[1:]
    if to_return[-1] == '"':
        to_return = to_return[:-1:]

    return to_return.replace("/", "\\")


def get_name_extension(file_path: str):
    lst_path = file_path.split("\\")
    to_check = lst_path[-1]

    return to_check[: to_check.index(".") :], to_check[to_check.index(".") + 1 :]


def get_parent(file_path: str):
    lst_path = file_path.split("\\")

    return "\\".join(lst_path[:-1:])


def get_key_size(k:str):
    return Path(f"scr\\key\\key_{k}.txt").stat().st_size


def get_chunked(file, file_path, mode, chunk_size=1024):
    size = Path(file_path).stat().st_size
    counter = 0
    current_count = get_decrypt_counter() if mode == "d" else get_encrypt_counter()
    with open(f"scr\\key\\key_{'A' if mode == 'e' else 'B'}.txt", "rb") as k:
        while counter < size:
            file.seek(counter)
            file_chunk = file.read(
                chunk_size if counter + chunk_size <= size else size - counter
            )
            k.seek(current_count + counter)
            counter += chunk_size if counter + chunk_size <= size else size - counter

            print(f"\rChunk: {counter}/{size}", end="", flush=True)

            key_chunk = k.read(len(file_chunk))

            yield file_chunk, key_chunk

        if mode == "d":
            add_decrypt_counter(size)
        elif mode == "e":
            add_encrypt_counter(size)
