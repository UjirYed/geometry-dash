/*
 * Avalon memory-mapped agent peripheral that produces a VGA tile display
 *
 * Stephen A. Edwards
 * Columbia University
 *
 * Memory map:
 *
 * 0000 - 1FFF Tilemap (8K, tile number is 8 bits per byte)
 * 2000 - 203F Palette (64, 24 bits every 4 bytes)
 * 4000 - 7FFF Tileset (16K, color index is lower 4 bits of each byte)
 *
 * 00m mmmm mmmm mmmm  Tilemap
 * 010 0000 00pp ppbb  Palette
 * 1ss ssss ssss ssss  Tileset
 *
 * In the 64-byte palette region, every color occupies 4 bytes, although
 * only 24 bits are stored.  Writing to the first 3 bytes in each group
 * writes a byte into the 24-bit color register.  Writing to the fourth
 * byte writes the color register to the palette memory; any data written to
 * these addresses is ignored; they always read 0.
 *
 * | Offset |    On Write          |  On Read          |
 * +--------+----------------------+-------------------+
 * |   0    | creg[7:0] <- data    | palette[0].red    |
 * |   1    | creg[15:8] <- data   | palette[0].green  |
 * |   2    | creg[23:16] <- data  | palette[0].blue   |
 * |   3    | palette[0] <- creg   | Always 0          |
 * |   4    | creg[7:0] <- data    | palette[1].red    |
 * |   5    | creg[15:8] <- data   | palette[1].green  |
 * |   6    | creg[23:16] <- data  | palette[1].blue   |
 * |   7    | palette[1] <- creg   | Always 0          |
 *  ...
 * |   60    | creg[7:0] <- data   | palette[15].red   |
 * |   61    | creg[15:8] <- data  | palette[15].green |
 * |   62    | creg[23:16] <- data | palette[15].blue  |
 * |   63    | palette[15] <- creg | Always 0          |
 *
 */
module vga_tiles
  (input logic 	      clk, reset,                    // Avalon MM Agent port
   input logic 	      chipselect, write,             // read == chipselect & !write
   input logic [14:0] address,                       // 32K window
   input logic [7:0]  writedata,                     // 8-bit interface
   output logic [7:0] readdata,

   input logic        vga_clk_in, VGA_RESET,         // VGA signals
   output logic [7:0] VGA_R, VGA_G, VGA_B,           
   output logic       VGA_CLK, VGA_HS, VGA_VS, VGA_BLANK_n);

   logic [2:0] 	      creg_write;                    // Latch enable per byte
   logic 	      tm_we, ts_we, palette_we;      // Memory write enables
   logic [7:0] 	      tm_dout;                       // Data from tilemap
   logic [3:0] 	      ts_dout;                       // Data from tileset
   logic [23:0]       creg, palette_dout;            // Data to/from palette

   tiles tiles(.mem_clk        ( clk           ),
	       .tm_address     ( address[12:0] ), .tm_din     ( writedata      ),
	       .ts_address     ( address[13:0] ), .ts_din     ( writedata[3:0] ),
	       .palette_address( address[5:2]  ), .palette_din( creg           ), .*);
   assign VGA_CLK = vga_clk_in;

   always_comb begin                                   // Address Decoder
      {tm_we, ts_we, palette_we, creg_write, readdata } = { 6'b 0, 8'h xx };
      if (chipselect)
	if (address[14] == 1'b 1) begin                // Tileset 1--------------
	   ts_we    = write;                           //  Write to tileset mem
	   readdata = { 4'h 0, ts_dout };              //  Read lower 4 bits; pad upper
	end else if (address[13] == 1'b 0) begin       // Tilemap 00-------------
	   tm_we    = write;                           //  Write to tilemap mem
	   readdata = tm_dout;                         //  Read 8 bits
	end else if ( address[12:6] == 7'b 0_0000_00 ) // Palette 010000000------
	   case (address[1:0])
	     2'h 0 : begin readdata = palette_dout[7:0];   // Read red byte
           		   creg_write[0] = write;          // creg <- red
          	     end
	     2'h 1 : begin readdata = palette_dout[15:8];  // Read green byte
                           creg_write[1] = write;          // creg <- green
                     end
             2'h 2 : begin readdata = palette_dout[23:16]; // Read blue byte
                           creg_write[2] = write;          // creg <- blue
                     end
	     2'h 3 : begin readdata = 8'h 00;              // Always reads as 00
                           palette_we = write;             // mem <- creg
                     end
	   endcase
   end

   always_ff @(posedge clk or posedge reset)
     if (reset) creg <= 24'b 0; else begin      
	if (creg_write[0]) creg[7:0]   <= writedata;    // Write byte (color)
	if (creg_write[1]) creg[15:8]  <= writedata;    // to creg according to
	if (creg_write[2]) creg[23:16] <= writedata;    // creg_write bits
     end
endmodule
