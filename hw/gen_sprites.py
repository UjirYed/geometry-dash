import os
from PIL import Image

IMAGE_DIR = "images"
OUTPUT_FILE = "sprites.hex"
WORD_COUNT = 4096  # matches 16384-byte RAM with 32-bit words

def rgb_to_word(r, g, b):
    return (r << 16) | (g << 8) | b

words = []

# Load each .png file
for filename in sorted(os.listdir(IMAGE_DIR)):
    if filename.lower().endswith(".png"):
        path = os.path.join(IMAGE_DIR, filename)
        img = Image.open(path).convert("RGB")
        pixels = list(img.getdata())
        for r, g, b in pixels:
            words.append(rgb_to_word(r, g, b))

# Pad with zeros if needed
while len(words) < WORD_COUNT:
    words.append(0)

# Trim if too long
words = words[:WORD_COUNT]

# Write to .hex
with open(OUTPUT_FILE, "w") as f:
    for word in words:
        f.write(f"{word:08X}\n")

print(f"Generated {OUTPUT_FILE} with {len(words)} 32-bit words from PNGs in '{IMAGE_DIR}/'.")

