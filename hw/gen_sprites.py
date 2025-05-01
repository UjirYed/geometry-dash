import os
from PIL import Image

IMAGE_DIR = "images"
OUTPUT_FILE = "sprites.hex"
SPRITE_DIM = (32, 32)  # width × height
MAX_SPRITES = 16
WORDS_EXPECTED = 49152  # 2^14 = 16384 * 3

def rgb_to_word(r, g, b):
    return (r << 16) | (g << 8) | b  # 24-bit RGB

# Pre-fill memory with black pixels
memory = [0x00000000] * WORDS_EXPECTED

sprite_files = sorted([
    f for f in os.listdir(IMAGE_DIR)
    if f.lower().endswith(".png")
])[:MAX_SPRITES]

print(f"Found {len(sprite_files)} sprites. Filling remaining with black.")

for image_id, filename in enumerate(sprite_files):
    path = os.path.join(IMAGE_DIR, filename)
    img = Image.open(path).convert("RGB")

    if img.size != SPRITE_DIM:
        raise ValueError(f"{filename} is {img.size}, expected {SPRITE_DIM}.")

    for y in range(8):
        for x in range(8):
            r, g, b = img.getpixel((x, y))
            addr = (image_id << 6) | (y << 3) | x
            memory[addr] = rgb_to_word(r, g, b)

# Write hex file
with open(OUTPUT_FILE, "w") as f:
    for word in memory:
        f.write(f"{word:08X}\n")

print(f"✅ Wrote {OUTPUT_FILE} with {WORDS_EXPECTED} 32-bit words.")