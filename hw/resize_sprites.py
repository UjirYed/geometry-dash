import os
from PIL import Image

IMAGE_DIR = "images"
OUTPUT_SIZE = (32, 32)  # (width, height)

for filename in os.listdir(IMAGE_DIR):
    if filename.lower().endswith(".png"):
        path = os.path.join(IMAGE_DIR, filename)
        img = Image.open(path)
        resized = img.resize(OUTPUT_SIZE, Image.NEAREST)
        resized.save(path)  # Overwrites original
        print(f"✔ Resized {filename} to {OUTPUT_SIZE}")
