module twoportbram
  #(parameter int DATA_BITS = 8,  ADDRESS_BITS = 10)
   (input logic 		   clk1,  clk2,
    input logic [ADDRESS_BITS-1:0] addr1, addr2,
    input logic [DATA_BITS-1:0]    din1,  din2,
    input logic 		   we1,   we2,
    output logic [DATA_BITS-1:0]   dout1, dout2);

   localparam WORDS = 1 << ADDRESS_BITS;

   /* verilator lint_off MULTIDRIVEN */
   logic [DATA_BITS-1:0] 	   mem [WORDS-1:0];
   /* verilator lint_on MULTIDRIVEN */
   
   always_ff @(posedge clk1)
     if (we1) begin
	mem[addr1] <= din1;
	dout1 <= din1;
     end else dout1 <= mem[addr1];

   always_ff @(posedge clk2)
     if (we2) begin
	mem[addr2] <= din2;
	dout2 <= din2;
     end else dout2 <= mem[addr2];
   
endmodule
