# geometry-dash
Hardware implementation of the popular polygon parkour game

Physical address: 0x0000_8000

# Image Table Memory Map (Sprite ROM)

This design implements a memory-mapped ROM to store 256 8×8 sprites, each pixel encoded as a 24-bit RGB value.

---

## RAM Layout

- **Total sprites**: 256  
- **Sprite size**: 8 × 8 pixels  
- **Pixel format**: 24-bit RGB  
- **Total pixels**: 256 × 64 = 16,384  
- **Total memory**: 16,384 × 3 bytes = **49,152 bytes**

---

## Address Format (14 bits)

Each 14-bit address selects **one pixel**:

```
[13:6]  = Image ID (8 bits)     → 256 images
[5:3]   = Y pixel row (3 bits)  → 8 rows
[2:0]   = X pixel col (3 bits)  → 8 columns
```

### Bit Diagram

```
 Address[13:0]:
 ┌──────┬───────┬───────┐
 │ 13:6 │  5:3  │  2:0  │
 └──────┴───────┴───────┘
   Img     Y       X
```

### Output Data: 24 bits (RGB)

```
Output[23:0]:
 ┌────────┬────────┬────────┐
 │ 23:16  │ 15:8   │  7:0   │
 └────────┴────────┴────────┘
     Red     Green   Blue
```

---

## Sprite File Generation

Run `gen_sprites.py` to convert 16×16 PNGs in the `images/` folder into `sprites.hex`.

### Requirements

```bash
pip install Pillow
```

### Usage

```bash
python3 gen_sprites.py
```

This generates `sprites.hex`, a 32-bit word file preloading your ROM in Platform Designer.

---

## Quartus Configuration

- Component: **On-Chip Memory (ROM)**
- Total Memory: **49152 bytes**
- Data Width: **24 or 32 bits**
- Address Width: **14 bits**
- Init File: **sprites.hex**

---

## Example Image Layout

Assuming Image ID = `0x12`, pixel at (x=3, y=5):

- Address = `0x12 << 6 | 5 << 3 | 3` = `0x12A3`  
- Data = RGB value of that pixel

---

## Folder Structure

```
lab3-hw/
├── images/            # PNG sprite sources (16×16, RGB)
├── gen_sprites.py     # PNG → HEX generator
├── sprites.hex        # Memory init file
├── vga_ball.sv        # Display logic
└── soc_system.qsys    # Platform Designer system
```
