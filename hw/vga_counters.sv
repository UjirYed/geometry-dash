module vga_counters(
   input  logic       VGA_CLK, VGA_RESET,
   output logic [9:0] hcount, // 0-639 active, 640-799 blank/sync
   output logic [9:0] vcount, // 0-479 active, 480-524 blank/sync
   output logic       VGA_HS, VGA_VS, VGA_BLANK_n);
 
   logic endOfLine;
   assign endOfLine = hcount == 10'd 799;
   
   always_ff @(posedge VGA_CLK or posedge VGA_RESET)
     if (VGA_RESET)      hcount <= 10'd 797;
     else if (endOfLine) hcount <= 0;
     else                hcount <= hcount + 10'd 1;
       
   logic endOfFrame;
   assign endOfFrame = vcount == 10'd 524;
   
   always_ff @(posedge VGA_CLK or posedge VGA_RESET)
     if (VGA_RESET)      vcount <= 10'd 524;
     else if (endOfLine)
       if (endOfFrame)   vcount <= 10'd 0;
       else              vcount <= vcount + 10'd 1;

   // 656 <= hcount <= 751
   assign VGA_HS = !( hcount[9:7] == 3'b101 &
                      hcount[6:4] != 3'b000 & hcount[6:4] != 3'b111 );
   assign VGA_VS = !( vcount[9:1] == 9'd 245 ); // Lines 490 and 491

   // hcount < 640 && vcount < 480
   assign VGA_BLANK_n = !( hcount[9] & (hcount[8] | hcount[7]) ) &
                        !( vcount[9] | (vcount[8:5] == 4'b1111) );   
endmodule  
