/*
 * Avalon memory-mapped peripheral that generates VGA
 *
 * Stephen A. Edwards
 * Columbia University
 *
 * Register map:
 * 
 * Byte Offset  7 ... 0   Meaning
 *        0    | player_y_pos   | Player y position (0-480)
 *        2    | x_shift        | The current top leftmost map_block
 *        4    | background_r   | Red component of background color (0-255)
 *        6    | background_g   | Green component
 *        8    | background_b   | Blue component
 *        10   | map_block      | A section of map containing an obstacle id
 *        12   | flags          | Start, Acknowledgment
 *        14   | output         | Output flags
 */

module player_sprite(input logic        clk,
            input logic 	   reset,
        input logic [15:0]  writedata,
        input logic 	   write,
        input 		   chipselect,
        input logic [2:0]  address,

        output logic [7:0] VGA_R, VGA_G, VGA_B,
        output logic 	   VGA_CLK, VGA_HS, VGA_VS,
                           VGA_BLANK_n,
        output logic 	   VGA_SYNC_n);

    logic [10:0]	   hcount;
    logic [9:0]     vcount;

    logic [7:0] 	   background_r, background_g, background_b;

    // REGISTERS
    logic [15:0] player_y_pos;
    logic [15:0] x_shift;
    logic [7:0]  map_block;
    logic [7:0]  flags;
    logic [7:0]  output_flags;
    
   vga_counters counters(.clk50(clk), .*);

    always_ff @(posedge clk)
        if (reset) begin
            background_r <= 8'h0;
            background_g <= 8'h0;
            background_b <= 8'h80;
        end else if (chipselect && write)
        case (address)
            3'h0: player_y_pos <= writedata;
            3'h1: x_shift <= writedata;
            3'h2: background_r <= writedata[7:0];
            3'h3: background_g <= writedata[7:0];
            3'h4: background_b <= writedata[7:0];
            3'h5: map_block <= writedata[7:0];
            3'h6: flags <= writedata[7:0];
            3'h7: output_flags <= writedata[7:0];
        endcase

    always_comb begin
        {VGA_R, VGA_G, VGA_B} = {8'h0, 8'h0, 8'h0};
        if (VGA_BLANK_n )
            if ( (hcount[10:6] == 5'd3) && (vcount >= player_y_pos) && (vcount < player_y_pos + 16) )
                {VGA_R, VGA_G, VGA_B} = {8'hff, 8'hff, 8'hff};
        else
            {VGA_R, VGA_G, VGA_B} =
                {background_r, background_g, background_b};
    end
           
endmodule

module vga_counters(
 input logic 	     clk50, reset,
 output logic [10:0] hcount,  // hcount[10:1] is pixel column
 output logic [9:0]  vcount,  // vcount[9:0] is pixel row
 output logic 	     VGA_CLK, VGA_HS, VGA_VS, VGA_BLANK_n, VGA_SYNC_n);

/*
 * 640 X 480 VGA timing for a 50 MHz clock: one pixel every other cycle
 * 
 * HCOUNT 1599 0             1279       1599 0
 *             _______________              ________
 * ___________|    Video      |____________|  Video
 * 
 * 
 * |SYNC| BP |<-- HACTIVE -->|FP|SYNC| BP |<-- HACTIVE
 *       _______________________      _____________
 * |____|       VGA_HS          |____|
 */
   // Parameters for hcount
   parameter HACTIVE      = 11'd 1280,
             HFRONT_PORCH = 11'd 32,
             HSYNC        = 11'd 192,
             HBACK_PORCH  = 11'd 96,   
             HTOTAL       = HACTIVE + HFRONT_PORCH + HSYNC +
                            HBACK_PORCH; // 1600
   
   // Parameters for vcount
   parameter VACTIVE      = 10'd 480,
             VFRONT_PORCH = 10'd 10,
             VSYNC        = 10'd 2,
             VBACK_PORCH  = 10'd 33,
             VTOTAL       = VACTIVE + VFRONT_PORCH + VSYNC +
                            VBACK_PORCH; // 525

   logic endOfLine;
   
   always_ff @(posedge clk50 or posedge reset)
     if (reset)          hcount <= 0;
     else if (endOfLine) hcount <= 0;
     else  	         hcount <= hcount + 11'd 1;

   assign endOfLine = hcount == HTOTAL - 1;
       
   logic endOfField;
   
   always_ff @(posedge clk50 or posedge reset)
     if (reset)          vcount <= 0;
     else if (endOfLine)
       if (endOfField)   vcount <= 0;
       else              vcount <= vcount + 10'd 1;

   assign endOfField = vcount == VTOTAL - 1;

   // Horizontal sync: from 0x520 to 0x5DF (0x57F)
   // 101 0010 0000 to 101 1101 1111
   assign VGA_HS = !( (hcount[10:8] == 3'b101) &
              !(hcount[7:5] == 3'b111));
   assign VGA_VS = !( vcount[9:1] == (VACTIVE + VFRONT_PORCH) / 2);

   assign VGA_SYNC_n = 1'b0; // For putting sync on the green signal; unused
   
   // Horizontal active: 0 to 1279     Vertical active: 0 to 479
   // 101 0000 0000  1280	       01 1110 0000  480
   // 110 0011 1111  1599	       10 0000 1100  524
   assign VGA_BLANK_n = !( hcount[10] & (hcount[9] | hcount[8]) ) &
            !( vcount[9] | (vcount[8:5] == 4'b1111) );

   /* VGA_CLK is 25 MHz
    *             __    __    __
    * clk50    __|  |__|  |__|
    *        
    *             _____       __
    * hcount[0]__|     |_____|
    */
   assign VGA_CLK = hcount[0]; // 25 MHz clock: rising edge sensitive
   
endmodule
