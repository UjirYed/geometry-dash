module tiles
  (input logic         VGA_CLK, VGA_RESET,
   output logic [7:0]  VGA_R, VGA_G, VGA_B,
   output logic        VGA_HS, VGA_VS, VGA_BLANK_n,

   input logic 	       mem_clk,         // Clock for memory ports
   
   input logic [13:0]  tm_address,      // Tilemap memory port (increased from 12:0 to 13:0 for double width)
   input logic 	       tm_we,
   input logic [7:0]   tm_din,
   output logic [7:0]  tm_dout,

   input logic [13:0]  ts_address,      // Tileset memory port
   input logic 	       ts_we,
   input logic [3:0]   ts_din,
   output logic [3:0]  ts_dout,

   input logic [3:0]   palette_address, // Palette memory port
   input logic 	       palette_we,
   input logic [23:0]  palette_din,
   output logic [23:0] palette_dout,
   
   input logic [9:0]   scroll_offset);  // New input for scroll offset (in pixels to ensure pixel by pixel scrolling)
   
   logic [9:0] 	       hcount;          // From counters
   logic [8:0] 	       vcount;

   logic [4:0] 	       hcount1;         // Pipeline registers (5 bits for 32 pixels)
   logic 	       VGA_HS0, VGA_HS1, VGA_HS2;
   logic 	       VGA_BLANK_n0, VGA_BLANK_n1, VGA_BLANK_n2;	       
   
   logic [7:0] 	       tilenumber;      // Memory outputs
   logic [3:0] 	       colorindex;

   /* verilator lint_off UNUSED */
   logic               unconnected; // Extra vcount bit from counters
   /* verilator lint_on UNUSED */
   
   // Calculate tile-aligned scroll position (divide by 32)
   logic [4:0]         scroll_tile;     // Which tile we're scrolled to (0-31)
   logic [4:0]         scroll_pixel;    // Fine offset within the tile (0-31)
   
   assign scroll_tile = scroll_offset[9:5];   // Upper 5 bits = tile position
   assign scroll_pixel = scroll_offset[4:0];  // Lower 5 bits = pixel position within tile
   
   // Calculate the effective horizontal counter with scrolling applied
   logic [9:0]         effective_hcount;
   assign effective_hcount = hcount + scroll_offset;
   
   vga_counters cntrs(.vcount( {unconnected, vcount} ), // VGA Counters
		      .VGA_BLANK_n( VGA_BLANK_n0 ),
		      .VGA_HS( VGA_HS0 ),
		      .*);

   twoportbram #(.DATA_BITS(8), .ADDRESS_BITS(14))  // Tile Map (increased from 13 to 14 bits)
   tilemap(.clk1  ( VGA_CLK ), .clk2 ( mem_clk ),
	   // Use modulo logic to wrap around at the right edge of the buffer
	   // 6 bits (64 tiles) for horizontal, but we only show 20 tiles (640/32) at a time
	   // The effective_hcount provides continuous scrolling effect
	   .addr1 ( { vcount[8:5], effective_hcount[9:5] & 6'h3F } ), // 6-bit mask for 64 tiles wide
	   .we1   ( 1'b0 ), .din1( 8'h X ), .dout1( tilenumber ),
	   .addr2 ( tm_address ),
	   .we2   ( tm_we ), .din2( tm_din ), .dout2( tm_dout ));
   
   always_ff @(posedge VGA_CLK)                     // Pipeline registers
     { hcount1, VGA_BLANK_n1, VGA_HS1 } <=
       { effective_hcount[4:0], VGA_BLANK_n0, VGA_HS0 };  // Use effective_hcount for fine scrolling
      
   twoportbram #(.DATA_BITS(4), .ADDRESS_BITS(14))  // Tile Set
   tileset(.clk1  ( VGA_CLK ), .clk2 ( mem_clk ),
	   .addr1 ( { tilenumber, vcount[4:0], hcount1 } ), // Changed from [2:0] to [4:0] for local coordinates
	   .we1   ( 1'b0 ), .din1( 4'h X), .dout1( colorindex ),
	   .addr2 ( ts_address ),
	   .we2   ( ts_we ), .din2( ts_din ), .dout2( ts_dout ));   

   always_ff @(posedge VGA_CLK)                     // Pipeline registers
     { VGA_BLANK_n2, VGA_HS2 } <= { VGA_BLANK_n1, VGA_HS1 };

   twoportbram #(.DATA_BITS(24), .ADDRESS_BITS(4))  // Palette
   palette(.clk1  ( VGA_CLK ), .clk2 ( mem_clk ),
	   .addr1 ( colorindex ),
	   .we1   ( 1'b0 ), .din1( 24'h X ), .dout1( { VGA_B, VGA_G, VGA_R } ),
	   .addr2 ( palette_address ),
	   .we2   ( palette_we ), .din2( palette_din ), .dout2( palette_dout ));

   always_ff @(posedge VGA_CLK)                     // Pipeline registers
     { VGA_BLANK_n, VGA_HS } <= { VGA_BLANK_n2, VGA_HS2 };
   
endmodule
	       
   