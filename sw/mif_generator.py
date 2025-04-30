from PIL import Image

TILE_SIZE = 8
TILES = 256
WIDTH = 32  # bits per word
DEPTH = TILES * TILE_SIZE * TILE_SIZE  # 256 tiles × 8 × 8 pixels = 16384

image = Image.open("tilesheet.png").convert("RGB")
img_width, img_height = image.size

if img_width < TILE_SIZE or img_height < TILE_SIZE:
    raise ValueError("Image must be at least 8×8 pixels.")

# Make sure image contains enough tiles
tiles_x = img_width // TILE_SIZE
tiles_y = img_height // TILE_SIZE
if tiles_x * tiles_y < TILES:
    raise ValueError(f"Not enough tiles in tilesheet: found {tiles_x * tiles_y}, need {TILES}")

def get_tile(img, tile_index):
    tx = tile_index % tiles_x
    ty = tile_index // tiles_x
    tile = []
    for y in range(TILE_SIZE):
        for x in range(TILE_SIZE):
            r, g, b = img.getpixel((tx * TILE_SIZE + x, ty * TILE_SIZE + y))
            word = (r << 16) | (g << 8) | b
            tile.append(word)
    return tile

with open("img_table.mif", "w") as f:
    f.write(f"WIDTH={WIDTH};\n")
    f.write(f"DEPTH={DEPTH};\n")
    f.write("ADDRESS_RADIX=UNS;\n")
    f.write("DATA_RADIX=HEX;\n")
    f.write("CONTENT BEGIN\n")

    addr = 0
    for tile_index in range(TILES):
        pixels = get_tile(image, tile_index)
        for word in pixels:
            f.write(f"    {addr} : {word:06X};\n")
            addr += 1

    f.write("END;\n")

print("Generated img_table.mif")
