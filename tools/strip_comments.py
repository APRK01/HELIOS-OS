import re
import os
import glob

def remove_comments(text):
    def replacer(match):
        s = match.group(0)
        if s.startswith('/'):
            return " " # note: a space and not an empty string
        else:
            return s
    pattern = re.compile(
        r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
        re.DOTALL | re.MULTILINE
    )
    return re.sub(pattern, replacer, text)

def process_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()
    
    new_content = remove_comments(content)
    
    # Remove empty lines created by comment removal (optional, but looks cleaner)
    # new_content = re.sub(r'\n\s*\n', '\n', new_content)

    if content != new_content:
        print(f"Stripping comments from {filepath}")
        with open(filepath, 'w') as f:
            f.write(new_content)

extensions = ['*.c', '*.h', '*.S', 'Makefile', '*.ld']
files = []
for ext in extensions:
    files.extend(glob.glob(f"**/{ext}", recursive=True))

for f in files:
    if "tools" in f or "limine" in f: continue
    process_file(f)
