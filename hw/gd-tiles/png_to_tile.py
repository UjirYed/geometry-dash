import sys
import os
import numpy as np
from PIL import Image
import json
import math

def read_palette_file(palette_file):
    """
    Read a color palette file with hex color values.
    Format expected: "RR GG BB XX" per line (XX is ignored)
    """
    palette = []
    
    with open(palette_file, 'r') as f:
        for line in f:
            # Remove comments
            if '//' in line:
                line = line.split('//')[0].strip()
            
            # Skip empty lines
            if not line.strip():
                continue
                
            # Split by spaces
            values = line.strip().split()
            
            # Convert hex strings to RGB values
            if len(values) >= 3:
                try:
                    r = int(values[0], 16)
                    g = int(values[1], 16)
                    b = int(values[2], 16)
                    palette.append((r, g, b))
                except ValueError:
                    print(f"Warning: Could not parse line '{line}'")
    
    return palette

def euclidean_distance(color1, color2):
    """
    Calculate Euclidean distance between two RGB colors.
    """
    return math.sqrt(sum((a - b) ** 2 for a, b in zip(color1, color2)))

def find_closest_color(pixel, palette):
    """
    Find the closest color in the palette to the given pixel.
    Returns the index of the closest color.
    """
    min_distance = float('inf')
    closest_index = 0
    
    for i, color in enumerate(palette):
        distance = euclidean_distance(pixel[:3], color)  # Compare only RGB values
        
        if distance < min_distance:
            min_distance = distance
            closest_index = i
    
    return closest_index

def png_to_palette_matrix(input_file, palette_file, size=(32, 32)):
    """
    Convert a PNG file to a matrix of palette indices.
    
    Args:
        input_file: Path to input PNG file
        palette_file: Path to palette hex file
        size: Target size (width, height) tuple
    
    Returns:
        A matrix of palette indices
    """
    # Read the palette
    palette = read_palette_file(palette_file)
    
    if not palette:
        print("Error: No colors found in palette file")
        return None
    
    print(f"Loaded palette with {len(palette)} colors")
    
    # Open and resize the image
    try:
        img = Image.open(input_file)
        img = img.convert('RGBA')  # Convert to RGBA to handle transparency
        img = img.resize(size, Image.LANCZOS)
        pixels = np.array(img)
    except Exception as e:
        print(f"Error processing image: {e}")
        return None
    
    # Create output matrix
    height, width, _ = pixels.shape
    matrix = []
    
    # Process each pixel
    for y in range(height):
        row = []
        for x in range(width):
            pixel = pixels[y, x]
            palette_index = find_closest_color(pixel, palette)
            row.append(palette_index)
        matrix.append(row)
    
    return matrix

def print_hex_matrix(matrix, palette):
    """Print the matrix with hex color values for verification"""
    for row in matrix:
        hex_row = [f"{palette[idx][0]:02X}{palette[idx][1]:02X}{palette[idx][2]:02X}" for idx in row]
        print(" ".join(hex_row[:10]) + "...")  # Print first 10 values of each row

def print_ascii_art(matrix, palette):
    """Print an ASCII representation of the image using characters for brightness"""
    chars = " .:-=+*#%@"  # Characters from dark to bright
    
    for row in matrix:
        line = ""
        for idx in row:
            # Calculate brightness (0-1)
            r, g, b = palette[idx]
            brightness = (0.299 * r + 0.587 * g + 0.114 * b) / 255
            
            # Map brightness to character
            char_idx = min(int(brightness * len(chars)), len(chars) - 1)
            line += chars[char_idx]
        print(line)

def save_matrix_to_hex_file(matrix, output_hex_file):
    """
    Save the palette index matrix to a hex file in a 32×32 grid format.
    Each value is written as a 2-digit hexadecimal number.
    """
    with open(output_hex_file, 'w') as f:
        for row in matrix:
            # Format each index as a 2-digit hex number with spaces between
            hex_row = ' '.join(f'{idx:02X}' for idx in row)
            f.write(hex_row + '\n')
    print(f"Saved hex matrix to {output_hex_file}")

def main():
    if len(sys.argv) < 3:
        print("Usage: python png_to_palette.py input.png palette.hex [output.hex]")
        return
    
    input_file = sys.argv[1]
    palette_file = sys.argv[2]
    output_hex = sys.argv[3] if len(sys.argv) > 3 else None
    
    if output_hex is None:
        # Default to a hex file if no output is specified
        base_name = os.path.splitext(input_file)[0]
        output_hex = f"{base_name}_palette.hex"
    
    # Check if files exist
    if not os.path.exists(input_file):
        print(f"Error: Input file '{input_file}' not found")
        return
    
    if not os.path.exists(palette_file):
        print(f"Error: Palette file '{palette_file}' not found")
        return
    
    # Process the image
    matrix = png_to_palette_matrix(input_file, palette_file)
    
    if matrix:
        palette = read_palette_file(palette_file)
        
        # Save to hex file
        save_matrix_to_hex_file(matrix, output_hex)
        
        # Print a preview
        print("\nPreview (first 10x10):")
        for i in range(min(10, len(matrix))):
            print(matrix[i][:10])
        
        # Print ASCII art preview
        print("\nASCII Art Preview:")
        print_ascii_art(matrix, palette)

if __name__ == "__main__":
    main()
