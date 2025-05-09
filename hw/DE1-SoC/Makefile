SYSTEM = soc_system

TCL = $(SYSTEM).tcl
QSYS = $(SYSTEM).qsys
SOPCINFO = $(SYSTEM).sopcinfo
QIP = $(SYSTEM)/synthesis/$(SYSTEM).qip
HPS_PIN_TCL = $(SYSTEM)/synthesis/submodules/hps_sdram_p0_pin_assignments.tcl
HPS_PIN_MAP = hps_sdram_p0_all_pins.txt
QPF = $(SYSTEM).qpf
QSF = $(SYSTEM).qsf
SDC = $(SYSTEM).sdc
SRF = $(SYSTEM).srf

BOARD_INFO = $(SYSTEM)_board_info.xml
DTS = $(SYSTEM).dts
DTB = $(SYSTEM).dtb

SOF = output_files/$(SYSTEM).sof
RBF = output_files/$(SYSTEM).rbf

# project
#
# Run the topmost tcl script to generate the initial project files
#
# This also adds the pin constraints for the sdram subsystem, which
# Platform Designer places in in the HPS_PIN_TCL file.
#
# However, to run that, quartus_map has to run once to prepare the netlist
# enough so that the HPS_PIN_TCL script can run

.PHONY : project
project : $(QPF) $(QSF) $(SDC)

$(QPF) $(QSF) $(SDC) : $(TCL) $(HPS_PIN_TCL)
	quartus_sh -t $(TCL)
	quartus_map $(SYSTEM)
	quartus_sta -t $(HPS_PIN_TCL) $(SYSTEM)

# qsys
#
# From the .qsys file, generate the .sopcinfo, .qip, and directory
# (named according to the system) with all the Verilog files, etc.

.PHONY : qsys
qsys : $(SOPCINFO)

$(SOPCINFO) $(QIP) $(HPS_PIN_TCL) $(SYSTEM)/ $(PRELOADER_SETTINGS_DIR) : $(QSYS)
	rm -rf $(SOPCINFO) $(SYSTEM)/
	qsys-generate $(QSYS) --synthesis=VERILOG

# quartus
#
# Run Quartus on the Qsys-generated files
#
# Note that for this to succeed, quartus_map has to be run once
# on the project so that quartus_sta can be run on the HPS_PIN_TCL
# script to set up the proper constraints for all the SDRAM pins

.PHONY : quartus
quartus : $(SOF)

$(SOF) $(HPS_PIN_MAP) : $(QIP) $(QPF) $(QSF) $(HPS_PIN_TCL)
	quartus_sh --flow compile $(QPF)

# rbf
#
# Convert the .sof file (for programming through the USB blaster)
# to an .rbf file to be placed on an SD card and written by u-boot
.PHONY : rbf
rbf : $(RBF)

$(RBF) : $(SOF)
	quartus_cpf -c $(SOF) $(RBF)

# dtb
#
# Use the .sopcinfo file to generate a device tree blob file
# with information about the memory map of the peripherals
.PHONY : dtb
dtb : $(DTB)

$(DTB) : $(DTS)
	@which dtc || (echo "dtc not found.  Did you run embedded_command_shell.sh?"; exit 1)
	dtc -I dts -O dtb -o $(DTB) $(DTS)

$(DTS) : $(SOPCINFO) $(BOARD_INFO)
	@which sopc2dts || (echo "sopc2dts not found.  Did you run embedded_command_shell.sh?"; exit 1)
	sopc2dts --input $(SOPCINFO) \
		--output $(DTS) \
		--type dts \
		--board $(BOARD_INFO) \
		--clocks

# clean
#
# Remove all generated files

.PHONY : clean quartus-clean qsys-clean project-clean
clean : quartus-clean qsys-clean project-clean dtb-clean

project-clean :
	rm -rf $(QPF) $(QSF) $(SDC)

qsys-clean :
	rm -rf $(SOPCINFO) $(QIP) $(SYSTEM)/ .qsys_edit \
	hps_isw_handoff/ hps_sdram_p0_summary.csv

quartus-clean :
	rm -rf  $(SOF) output_files db incremental_db $(SYSTEM).qdf \
	c5_pin_model_dump.txt $(HPS_PIN_MAP)

dtb-clean :
	rm -rf $(DTS) $(DTB)
