# Uses
#
# verilator
# inkscape
# awk
# pdflatex
# cc
# xxd
# convert

TIMING = vga_counters-reset vga_counters-endl0 vga_counters-endhs \
	vga_counters-start1 vga_counters-endactive

FIGURES = pac-man-game1.png  pac-man-game-tiles1.png pac-man-tiles.png \
	pac-man-tiles-2.png pac-man-detail.png 8x16-font.png qsys-system.pdf \
	tiles2ppm.c tiles1.png qsys-component.png screenshot.jpg

SVFILES = vga_counters.sv tiles.sv twoportbram.sv vga_tiles.sv

tiles.pdf : tiles.tex $(FIGURES) $(SVFILES) $(TIMING:%=%.pdf) 
	pdflatex -halt-on-error tiles.tex

.PHONY : lint clean veryclean

lint :
	for file in $(SVFILES) ; do \
	verilator --lint-only -Wall $$file ; done

obj_dir/Vvga_counters : vga_counters.sv vga_counters.cpp
	verilator -trace -Wall -cc vga_counters.sv -exe vga_counters.cpp \
		-top-module vga_counters
	cd obj_dir && make -j -f Vvga_counters.mk

vga_counters.vcd : obj_dir/Vvga_counters
	obj_dir/Vvga_counters 27033600

SIGNALS = VGA_CLK hcount:u vcount:u VGA_BLANK_n VGA_HS VGA_VS

# Just after reset; line 0 starts
vga_counters-reset.svg : vga_counters.vcd vcd2svg
	./vcd2svg -v textw=60 -v from=0 -v to=500 -v zoom=0.02 -v lead=10 \
	-v steep=3 VGA_RESET $(SIGNALS) < $< > $@

# End of line 0; blank; hsync
vga_counters-endl0.svg : vga_counters.vcd vcd2svg
	./vcd2svg -v textw=60 -v from=25520 -v to=26520 -v zoom=0.01 \
	-v lead=10 -v steep=3 $(SIGNALS) < $< > $@

# Leaving hsync on line 0
vga_counters-endhs.svg : vga_counters.vcd vcd2svg
	./vcd2svg -v textw=60 -v from=29920 -v to=30920 -v zoom=0.01 \
	-v lead=10 -v steep=3 $(SIGNALS) < $< > $@

# Start of line 1
vga_counters-start1.svg : vga_counters.vcd vcd2svg
	./vcd2svg -v textw=60 -v from=31880 -v to=32880 -v zoom=0.01 \
	-v lead=10 -v steep=3 $(SIGNALS) < $< > $@

# End of active region; vertical sync
vga_counters-endactive.svg : vga_counters.vcd vcd2svg
	./vcd2svg -v textw=60 -v from=15264140 -v to=15864140 -v zoom=0.00002 \
	-v steep=0 -v lead=15 vcount:u VGA_BLANK_n VGA_HS VGA_VS < $< > $@

%.pdf : %.svg
#	inkscape --export-filename=$@ $<
	inkscape --export-pdf=$@ $<

tiles2ppm : tiles2ppm.c
	cc -o $@ -Wall -O2 $<

%.bin : %.hex
	xxd -r -p $< > $@

tiles1.ppm : tiles2ppm tilemap1.bin tileset1.bin palette1.bin
	./tiles2ppm tilemap1.bin tileset1.bin palette1.bin > $@

%.png : %.ppm
	convert $< $@

PROJECTFILES = Makefile \
	soc_system.qpf soc_system.qsf soc_system.qsys soc_system_board_info.xml \
	soc_system.sdc soc_system.srf soc_system.tcl soc_system_top.sv \
	soc_system.dts vga_tiles_hw.tcl

TARFILES = Makefile \
	tiles.tex \
	$(SVFILES) \
	$(FIGURES) \
	vcd2svg \
	vga_counters.cpp \
	tilemap1.hex tileset1.hex palette1.hex \
	$(PROJECTFILES:%=DE1-SoC/%)

tiles.tar.gz : $(TARFILES)
	tar zcfC tiles.tar.gz .. $(TARFILES:%=tiles/%)

clean :
	rm -rf obj_dir *.aux *.log *.out tiles.pdf

veryclean : clean
	rm -rf $(TIMING:%=%.pdf) $(TIMING:%=%.svg) *.vcd \
	tiles1.png tiles1.ppm tilemap1.bin tileset1.bin palette1.bin \
	tiles2ppm
	(cd soc_system ; make clean)
