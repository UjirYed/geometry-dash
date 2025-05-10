module tiles
  (input logic         VGA_CLK, VGA_RESET,
   output logic [7:0]  VGA_R, VGA_G, VGA_B,
   output logic        VGA_HS, VGA_VS, VGA_BLANK_n,

   input logic 	       mem_clk,         // Clock for memory ports
   
   input logic [13:0]  tm_address,      // Tilemap memory port (14 bits for double width)
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
   
   input logic [9:0]   scroll_offset);  // For pixel-by-pixel scrolling (0-1023)
   
   logic [9:0] 	       hcount;          // From counters
   logic [8:0] 	       vcount;

   logic [4:0] 	       hcount1;         // Pipeline registers (5 bits for 32 pixels)
   logic 	       VGA_HS0, VGA_HS1, VGA_HS2;
   logic 	       VGA_BLANK_n0, VGA_BLANK_n1, VGA_BLANK_n2;	       
   
   /* verilator lint_off UNUSED */
   logic [7:0] 	       tilenumber;      // Memory outputs - only bits [3:0] used in tileset addressing
   logic               unconnected;     // Extra vcount bit from counters
   /* verilator lint_on UNUSED */
   
   logic [3:0] 	       colorindex;

   // Calculate the effective horizontal counter with scrolling applied
   logic [9:0]         effective_hcount;
   assign effective_hcount = hcount + scroll_offset;
   
   // Extract the tile coordinates from the screen position
   logic [3:0]         v_tile;          // Vertical tile position (0-15)
   logic [5:0]         h_tile;          // Horizontal tile position (0-63)
   
   // Map screen coordinates to tile coordinates
   assign v_tile = vcount[8:5];         // Divide y by 32 (5 bit shift)
   assign h_tile = {1'b0, effective_hcount[9:5]}; // Divide x by 32, zero-extend to 6 bits
   
   vga_counters cntrs(.vcount( {unconnected, vcount} ), // VGA Counters
		      .VGA_BLANK_n( VGA_BLANK_n0 ),
		      .VGA_HS( VGA_HS0 ),
		      .*);

   twoportbram #(.DATA_BITS(8), .ADDRESS_BITS(14))  // Tile Map (14 bits = 16K entries)
   tilemap(.clk1  ( VGA_CLK ), .clk2 ( mem_clk ),
	   // 4 bits for vertical (16 rows) + 6 bits for horizontal (64 cols) = 10 bits
	   // We pad with 4 zeros in the upper bits to match the 14-bit address
	   .addr1 ( { 4'b0000, v_tile, h_tile } ),
	   .we1   ( 1'b0 ), .din1( 8'h X ), .dout1( tilenumber ),
	   .addr2 ( tm_address ),
	   .we2   ( tm_we ), .din2( tm_din ), .dout2( tm_dout ));
   
   always_ff @(posedge VGA_CLK)                     // Pipeline registers
     { hcount1, VGA_BLANK_n1, VGA_HS1 } <=
       { effective_hcount[4:0], VGA_BLANK_n0, VGA_HS0 };  // Use effective_hcount for fine scrolling
   
   // Calculate the address into the tileset memory
   // We need a 14-bit address for the tileset memory
   logic [13:0]        ts_addr1;
   assign ts_addr1 = { tilenumber[3:0], vcount[4:0], hcount1 }; // 4+5+5=14 bits
   
   twoportbram #(.DATA_BITS(4), .ADDRESS_BITS(14))  // Tile Set
   tileset(.clk1  ( VGA_CLK ), .clk2 ( mem_clk ),
	   .addr1 ( ts_addr1 ),
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
	       
   