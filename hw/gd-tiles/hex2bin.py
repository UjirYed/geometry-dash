import sys

if len(sys.argv) < 1:
    print("usage: python3 hex2bin.py <file_name>")


file_name = sys.argv[1].split('.')[0]
print(sys.argv[0])
print(file_name)
with open(f"{sys.argv[1]}") as infile, open(f"{file_name}.bin", "wb") as outfile:
    for line in infile:
        # Remove comments
        if '//' in line:
            line = line.split('//')[0].strip()
        # Remove any extra spaces or newline characters
        clean_line = line.strip().replace("  ", " ").replace("\t", " ")
        if not clean_line:
            continue
        hex_bytes = clean_line.split()
        for hb in hex_bytes:
            outfile.write(bytes([int(hb, 16)]))

