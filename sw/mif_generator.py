from PIL import Image
import os
import glob

TILE_SIZE = 8
WIDTH = 32  # bits per word
OUT_FILE = "img_table.mif"

image_files = sorted(glob.glob("images/*.png"))  # load from /images folder
DEPTH = len(image_files) * TILE_SIZE * TILE_SIZE

def get_tile_pixels(img):
    tile = []
    for y in range(TILE_SIZE):
        for x in range(TILE_SIZE):
            r, g, b = img.getpixel((x, y))
            word = (r << 16) | (g << 8) | b
            tile.append(word)
    return tile

with open(OUT_FILE, "w") as f:
    f.write(f"WIDTH={WIDTH};\n")
    f.write(f"DEPTH={DEPTH};\n")
    f.write("ADDRESS_RADIX=UNS;\n")
    f.write("DATA_RADIX=HEX;\n")
    f.write("CONTENT BEGIN\n")

    addr = 0
    for img_id, file in enumerate(image_files):
        img = Image.open(file).convert("RGB")
        if img.size != (TILE_SIZE, TILE_SIZE):
            raise ValueError(f"{file} must be {TILE_SIZE}x{TILE_SIZE}")
        pixels = get_tile_pixels(img)
        for word in pixels:
            f.write(f"    {addr} : {word:06X};\n")
            addr += 1

    f.write("END;\n")

print(f"Wrote {OUT_FILE} with {len(image_files)} tiles ({DEPTH} words)")
