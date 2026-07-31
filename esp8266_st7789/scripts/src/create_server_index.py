import argparse
import os
import string
import collections
import hashlib
import humanize
from tabulate import tabulate, SEPARATING_LINE

def get_paths_recursive(root_path):
    for filename in os.listdir(root_path):
        filepath = os.path.join(root_path, filename)
        if os.path.isdir(filepath):
            for path in get_paths_recursive(filepath):
                yield path
        elif os.path.isfile(filepath):
            yield filepath
        else:
            print(f"Ignoring '{filepath}'")

DEFAULT_MIMETYPE = "application/octet-stream" 
MIMETYPES = {
    "bin": "application/octet-stream",
    "js": "application/javascript",
    "html": "text/html",
    "css": "text/css",
    "ico": "image/x-icon",
}

def get_mime_type(filepath):
    _, ext = os.path.splitext(filepath)
    if len(ext) <= 1:
        return DEFAULT_MIMETYPE
    ext = ext[1:]
    return MIMETYPES.get(ext, DEFAULT_MIMETYPE)

FileEntry = collections.namedtuple("FileEntry", ["filepath", "size", "mime_type", "sha1"])

def print_file_entries(file_entries):
    table_headers = ["filepath", "size", "mime_type", "sha1"]
    table_rows = []

    total_size_bytes = 0
    for index, entry in enumerate(file_entries):
        total_size_bytes += entry.size
        table_rows.append([
            f"/{entry.filepath}",
            humanize.naturalsize(entry.size),
            entry.mime_type,
            entry.sha1,
        ])
    table_rows.append(SEPARATING_LINE)
    table_rows.append([
        None,
        humanize.naturalsize(total_size_bytes),
        None,
        None,
    ])
    print(tabulate(table_rows, table_headers, tablefmt="outline"))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--static", default="./static", type=str, help="Directory of website files")
    parser.add_argument("--output", default="./static/server_files.csv", type=str, help="Filepath of index containing server file metadata")
    args = parser.parse_args()

    output_filepath = os.path.abspath(args.output)

    filepaths = get_paths_recursive(args.static)
    file_entries = []
    for filepath in filepaths:
        relative_filepath = os.path.relpath(filepath, args.static).replace("\\", "/")
        if ',' in relative_filepath:
            print(f"[ERROR]: Skipping '{filepath}' since it contains an illegal character ','")
            continue
        absolute_filepath = os.path.abspath(filepath)
        if absolute_filepath == output_filepath:
            continue
        with open(filepath, "rb") as fp:
            data = fp.read()
        size = len(data)
        mime_type = get_mime_type(filepath)
        sha1 = hashlib.sha1(data).hexdigest()
        entry = FileEntry(relative_filepath, size, mime_type, sha1)
        file_entries.append(entry)

    print(f"Indexing {len(file_entries)} files to '{output_filepath}'")
    print_file_entries(file_entries)

    with open(output_filepath, "w+") as fp:
        fp.write("filepath, size, mime_type, sha1\n")
        for entry in file_entries:
            fp.write(f"{entry.filepath},{entry.size},{entry.mime_type},{entry.sha1}\n")


if __name__ == "__main__":
    main()
