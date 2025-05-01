import os
from PIL import Image

IMAGE_DIR = "images"
OUTPUT_FILE = "sprites.hex"
WORD_COUNT = 4096

def rgb_to_word(r, g, b):
    return (r << 16) | (g << 8) | b

words = []
expected_size = None

for filename in sorted(os.listdir(IMAGE_DIR)):
    if filename.lower().endswith(".png"):
        path = os.path.join(IMAGE_DIR, filename)
        img = Image.open(path).convert("RGB")

        if expected_size is None:
            expected_size = img.size  # (width, height)
        elif img.size != expected_size:
            raise ValueError(f"ERROR: {filename} is {img.size}, expected {expected_size}")

        pixels = list(img.getdata())
        for r, g, b in pixels:
            words.append(rgb_to_word(r, g, b))

# Pad or trim to fit RAM
words = (words + [0] * WORD_COUNT)[:WORD_COUNT]

with open(OUTPUT_FILE, "w") as f:
    for word in words:
        f.write(f"{word:08X}\n")

print(f"Generated {OUTPUT_FILE} with {len(words)} 32-bit words from {IMAGE_DIR}/")
print(f"Sprite size: {expected_size[0]}×{expected_size[1]}")