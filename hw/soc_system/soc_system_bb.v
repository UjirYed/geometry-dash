
module soc_system (
	audio_0_external_interface_BCLK,
	audio_0_external_interface_DACDAT,
	audio_0_external_interface_DACLRCK,
	audio_and_video_config_0_external_interface_SDAT,
	audio_and_video_config_0_external_interface_SCLK,
	clk_clk,
	hps_hps_io_emac1_inst_TX_CLK,
	hps_hps_io_emac1_inst_TXD0,
	hps_hps_io_emac1_inst_TXD1,
	hps_hps_io_emac1_inst_TXD2,
	hps_hps_io_emac1_inst_TXD3,
	hps_hps_io_emac1_inst_RXD0,
	hps_hps_io_emac1_inst_MDIO,
	hps_hps_io_emac1_inst_MDC,
	hps_hps_io_emac1_inst_RX_CTL,
	hps_hps_io_emac1_inst_TX_CTL,
	hps_hps_io_emac1_inst_RX_CLK,
	hps_hps_io_emac1_inst_RXD1,
	hps_hps_io_emac1_inst_RXD2,
	hps_hps_io_emac1_inst_RXD3,
	hps_hps_io_sdio_inst_CMD,
	hps_hps_io_sdio_inst_D0,
	hps_hps_io_sdio_inst_D1,
	hps_hps_io_sdio_inst_CLK,
	hps_hps_io_sdio_inst_D2,
	hps_hps_io_sdio_inst_D3,
	hps_hps_io_usb1_inst_D0,
	hps_hps_io_usb1_inst_D1,
	hps_hps_io_usb1_inst_D2,
	hps_hps_io_usb1_inst_D3,
	hps_hps_io_usb1_inst_D4,
	hps_hps_io_usb1_inst_D5,
	hps_hps_io_usb1_inst_D6,
	hps_hps_io_usb1_inst_D7,
	hps_hps_io_usb1_inst_CLK,
	hps_hps_io_usb1_inst_STP,
	hps_hps_io_usb1_inst_DIR,
	hps_hps_io_usb1_inst_NXT,
	hps_hps_io_spim1_inst_CLK,
	hps_hps_io_spim1_inst_MOSI,
	hps_hps_io_spim1_inst_MISO,
	hps_hps_io_spim1_inst_SS0,
	hps_hps_io_uart0_inst_RX,
	hps_hps_io_uart0_inst_TX,
	hps_hps_io_i2c0_inst_SDA,
	hps_hps_io_i2c0_inst_SCL,
	hps_hps_io_i2c1_inst_SDA,
	hps_hps_io_i2c1_inst_SCL,
	hps_hps_io_gpio_inst_GPIO09,
	hps_hps_io_gpio_inst_GPIO35,
	hps_hps_io_gpio_inst_GPIO40,
	hps_hps_io_gpio_inst_GPIO48,
	hps_hps_io_gpio_inst_GPIO53,
	hps_hps_io_gpio_inst_GPIO54,
	hps_hps_io_gpio_inst_GPIO61,
	hps_ddr3_mem_a,
	hps_ddr3_mem_ba,
	hps_ddr3_mem_ck,
	hps_ddr3_mem_ck_n,
	hps_ddr3_mem_cke,
	hps_ddr3_mem_cs_n,
	hps_ddr3_mem_ras_n,
	hps_ddr3_mem_cas_n,
	hps_ddr3_mem_we_n,
	hps_ddr3_mem_reset_n,
	hps_ddr3_mem_dq,
	hps_ddr3_mem_dqs,
	hps_ddr3_mem_dqs_n,
	hps_ddr3_mem_odt,
	hps_ddr3_mem_dm,
	hps_ddr3_oct_rzqin,
	reset_reset_n,
	vga_b,
	vga_blank_n,
	vga_clk,
	vga_g,
	vga_hs,
	vga_r,
	vga_sync_n,
	vga_vs,
	fifo_1_in_irq_irq,
	fifo_1_in_csr_address,
	fifo_1_in_csr_read,
	fifo_1_in_csr_writedata,
	fifo_1_in_csr_write,
	fifo_1_in_csr_readdata,
	fifo_1_in_writedata,
	fifo_1_in_write,
	fifo_1_in_address,
	fifo_1_in_waitrequest,
	audio_0_reset_reset,
	audio_0_clk_clk);	

	input		audio_0_external_interface_BCLK;
	output		audio_0_external_interface_DACDAT;
	input		audio_0_external_interface_DACLRCK;
	inout		audio_and_video_config_0_external_interface_SDAT;
	output		audio_and_video_config_0_external_interface_SCLK;
	input		clk_clk;
	output		hps_hps_io_emac1_inst_TX_CLK;
	output		hps_hps_io_emac1_inst_TXD0;
	output		hps_hps_io_emac1_inst_TXD1;
	output		hps_hps_io_emac1_inst_TXD2;
	output		hps_hps_io_emac1_inst_TXD3;
	input		hps_hps_io_emac1_inst_RXD0;
	inout		hps_hps_io_emac1_inst_MDIO;
	output		hps_hps_io_emac1_inst_MDC;
	input		hps_hps_io_emac1_inst_RX_CTL;
	output		hps_hps_io_emac1_inst_TX_CTL;
	input		hps_hps_io_emac1_inst_RX_CLK;
	input		hps_hps_io_emac1_inst_RXD1;
	input		hps_hps_io_emac1_inst_RXD2;
	input		hps_hps_io_emac1_inst_RXD3;
	inout		hps_hps_io_sdio_inst_CMD;
	inout		hps_hps_io_sdio_inst_D0;
	inout		hps_hps_io_sdio_inst_D1;
	output		hps_hps_io_sdio_inst_CLK;
	inout		hps_hps_io_sdio_inst_D2;
	inout		hps_hps_io_sdio_inst_D3;
	inout		hps_hps_io_usb1_inst_D0;
	inout		hps_hps_io_usb1_inst_D1;
	inout		hps_hps_io_usb1_inst_D2;
	inout		hps_hps_io_usb1_inst_D3;
	inout		hps_hps_io_usb1_inst_D4;
	inout		hps_hps_io_usb1_inst_D5;
	inout		hps_hps_io_usb1_inst_D6;
	inout		hps_hps_io_usb1_inst_D7;
	input		hps_hps_io_usb1_inst_CLK;
	output		hps_hps_io_usb1_inst_STP;
	input		hps_hps_io_usb1_inst_DIR;
	input		hps_hps_io_usb1_inst_NXT;
	output		hps_hps_io_spim1_inst_CLK;
	output		hps_hps_io_spim1_inst_MOSI;
	input		hps_hps_io_spim1_inst_MISO;
	output		hps_hps_io_spim1_inst_SS0;
	input		hps_hps_io_uart0_inst_RX;
	output		hps_hps_io_uart0_inst_TX;
	inout		hps_hps_io_i2c0_inst_SDA;
	inout		hps_hps_io_i2c0_inst_SCL;
	inout		hps_hps_io_i2c1_inst_SDA;
	inout		hps_hps_io_i2c1_inst_SCL;
	inout		hps_hps_io_gpio_inst_GPIO09;
	inout		hps_hps_io_gpio_inst_GPIO35;
	inout		hps_hps_io_gpio_inst_GPIO40;
	inout		hps_hps_io_gpio_inst_GPIO48;
	inout		hps_hps_io_gpio_inst_GPIO53;
	inout		hps_hps_io_gpio_inst_GPIO54;
	inout		hps_hps_io_gpio_inst_GPIO61;
	output	[14:0]	hps_ddr3_mem_a;
	output	[2:0]	hps_ddr3_mem_ba;
	output		hps_ddr3_mem_ck;
	output		hps_ddr3_mem_ck_n;
	output		hps_ddr3_mem_cke;
	output		hps_ddr3_mem_cs_n;
	output		hps_ddr3_mem_ras_n;
	output		hps_ddr3_mem_cas_n;
	output		hps_ddr3_mem_we_n;
	output		hps_ddr3_mem_reset_n;
	inout	[31:0]	hps_ddr3_mem_dq;
	inout	[3:0]	hps_ddr3_mem_dqs;
	inout	[3:0]	hps_ddr3_mem_dqs_n;
	output		hps_ddr3_mem_odt;
	output	[3:0]	hps_ddr3_mem_dm;
	input		hps_ddr3_oct_rzqin;
	input		reset_reset_n;
	output	[7:0]	vga_b;
	output		vga_blank_n;
	output		vga_clk;
	output	[7:0]	vga_g;
	output		vga_hs;
	output	[7:0]	vga_r;
	output		vga_sync_n;
	output		vga_vs;
	output		fifo_1_in_irq_irq;
	input	[2:0]	fifo_1_in_csr_address;
	input		fifo_1_in_csr_read;
	input	[31:0]	fifo_1_in_csr_writedata;
	input		fifo_1_in_csr_write;
	output	[31:0]	fifo_1_in_csr_readdata;
	input	[31:0]	fifo_1_in_writedata;
	input		fifo_1_in_write;
	input		fifo_1_in_address;
	output		fifo_1_in_waitrequest;
	input		audio_0_reset_reset;
	input		audio_0_clk_clk;
endmodule
