#include <iostream>
#include <verilated.h>
#include <Vvga_counters.h>
#include <verilated_vcd_c.h>

unsigned stopat = 80 * 800 * 525;

int main(int argc, const char ** argv, const char ** env) {
  Verilated::commandArgs(argc, argv);

  if (argc > 1)
    stopat = atoi(argv[1]);

  Vvga_counters * dut = new Vvga_counters;
  
  Verilated::traceEverOn(true);
  VerilatedVcdC * tfp = new VerilatedVcdC;
  dut->trace(tfp, 99);
  tfp->open("vga_counters.vcd");

  // Initial values
  
  dut->VGA_RESET = 0;

  int time;
  for (time = 0; time < stopat ; time += 20) {
    dut->VGA_CLK = (time % 40 >= 20) ? 1 : 0; // 25 MHz clock
    if (time == 20) dut->VGA_RESET = 1;
    if (time == 60) dut->VGA_RESET = 0;
    
    dut->eval();
    tfp->dump(time);
  }
  
  tfp->close();
  delete tfp;

  dut->final();
  delete dut;

  return 0;
}

