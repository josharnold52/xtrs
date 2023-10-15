#
# Makefile for xtrs, the TRS-80 emulator.
# $Id$
#

OBJECTS = \
	z80.o \
	main.o \
	load_cmd.o \
	load_hex.o \
	trs_memory.o \
	trs_keyboard.o \
	error.o \
	debug.o \
	dis.o \
	trs_io.o \
	trs_cassette.o \
	trs_chars.o \
	trs_printer.o \
	trs_rom1.o \
	trs_rom3.o \
	trs_rom4p.o \
	trs_disk.o \
	trs_interrupt.o \
	trs_imp_exp.o \
	trs_hard.o \
	trs_uart.o \
	trs_stringy.o

X_OBJECTS = \
	trs_xinterface.o

DOS_OBJECTS = \
	trs_djgpp.o \
	trs_realtime.o \
	trs_djgpp_modal.o \
	trs_metafile.o \
        trs_ich.o \
	newutils.o

CR_OBJECTS = \
	compile_rom.bldo \
	error.bldo \
	load_cmd.bldo \
	load_hex.bldo

MD_OBJECTS = \
	mkdisk.o

HC_OBJECTS = \
	cmd.bldo \
	error.bldo \
	load_hex.bldo \
	hex2cmd.bldo

CD_OBJECTS = \
	cmddump.o \
	load_cmd.o

JT1_OBJECTS = \
	jahdatst.o \
	trs_ich.o \
	error.o

DOS16 = \
	launcher/target/LAUNCHER.COM \
	keytrap/target/KEYTRAP.COM
	

Z80CODE = export.cmd import.cmd settime.cmd xtrsmous.cmd \
	xtrs8.dct xtrshard.dct \
	fakerom.hex xtrsrom4p.hex esfrom.hex

MANPAGES = xtrs.txt mkdisk.txt cassette.txt cmddump.txt hex2cmd.txt

PDFMANPAGES = cassette.man.pdf \
	cmddump.man.pdf \
	hex2cmd.man.pdf \
	mkdisk.man.pdf \
	xtrs.man.pdf

HTMLDOCS = cpmutil.txt \
	dskspec.txt

PROGS = dosxtrs mkdisk hex2cmd cmddump jahdatst

ZMACINT = ./zmac-internal/zmac

default: $(PROGS) docs dos16

all: default z80code dos16

docs: $(MANPAGES) $(PDFMANPAGES) $(HTMLDOCS)

z80code: $(Z80CODE)

dos16: $(DOS16)

# Local customizations for make variables are done in Makefile.local:
include Makefile.local

$(ZMACINT):
	cd ./zmac-internal && make

scantran/generated_table.inc: scantran/scantran.scala
	bash -c "cd scantran && scala scantran.scala"

CFLAGS += $(DEBUG) $(ENDIAN) $(DEFAULT_ROM) $(READLINE) $(DISKDIR) $(IFLAGS) \
	$(APPDEFAULTS) -DKBWAIT
CXXFLAGS += $(DEBUG) $(ENDIAN) $(DEFAULT_ROM) $(READLINE) $(DISKDIR) $(IFLAGS) \
	$(APPDEFAULTS) -DKBWAIT
LIBS = $(XLIB) $(READLINELIBS) $(EXTRALIBS)

ZMACFLAGS = -h

.SUFFIXES: .dct .man .txt .html

%.bldo : %.c %.o
	$(BUILD_CC) -c $< -o $@



%.cmd : %.hex  $(ZMACINT) hex2cmd
	./hex2cmd $*.hex > $*.cmd

%.dct : %.hex  $(ZMACINT) hex2cmd
	./hex2cmd $*.hex > $*.dct

%.hex : %.z80  $(ZMACINT)
	$(ZMACINT) $(ZMACFLAGS) -o $*.hex -x $*.lst $<

.man.txt:
	nroff -man -c -Tascii $< | colcrt - | cat -s > $*.txt

.html.txt:
	lynx -dump  $< >$@

%.man.pdf: %.man
	groff -Tpdf -man $< > $@

dosxtrs: $(OBJECTS) $(DOS_OBJECTS)
	$(CC) $(LDFLAGS) -o dosxtrs $(OBJECTS) $(DOS_OBJECTS) $(LIBS)

xtrs: $(OBJECTS) $(X_OBJECTS)
	$(CC) $(LDFLAGS) -o xtrs $(OBJECTS) $(X_OBJECTS) $(LIBS)

compile_rom: $(CR_OBJECTS)
	$(BUILD_CC) -o compile_rom $(CR_OBJECTS)

trs_rom1.c: compile_rom $(BUILT_IN_ROM)
	./compile_rom 1 $(BUILT_IN_ROM) > trs_rom1.c

trs_rom3.c: compile_rom $(BUILT_IN_ROM3)
	./compile_rom 3 $(BUILT_IN_ROM3) > trs_rom3.c

trs_rom4p.c: compile_rom $(BUILT_IN_ROM4P)
	./compile_rom 4p $(BUILT_IN_ROM4P) > trs_rom4p.c

#trs_gtkinterface.o: trs_gtkinterface.c
#	$(CC) -c $(CFLAGS) `pkg-config --cflags gtk+-2.0` $<

#keyrepeat.o: keyrepeat.c
#	$(CC) -c $(CFLAGS) `pkg-config --cflags gtk+-2.0` $<

mkdisk:	$(MD_OBJECTS)
	$(CC) $(LDFLAGS) -o mkdisk $(MD_OBJECTS)

hex2cmd: $(HC_OBJECTS)
	$(BUILD_CC) -o hex2cmd $(HC_OBJECTS)

cmddump: $(CD_OBJECTS)
	$(CC) $(LDFLAGS) -o cmddump $(CD_OBJECTS)

jahdatst: $(JT1_OBJECTS)
	$(CC) $(LDFLAGS) -o jahdatst $(JT1_OBJECTS)

clean:
	rm -f $(OBJECTS) $(MD_OBJECTS) \
		$(X_OBJECTS) $(GTK_OBJECTS) \
		$(CR_OBJECTS) $(HC_OBJECTS) \
		$(JT1_OBJECTS) \
		$(CD_OBJECTS) $(DOS_OBJECTS) trs_rom*.c *~ \
		compile_rom.o  hex2cmd.o \
		$(PROGS) compile_rom gxtrs dosxtrs dosxtrs.exe jahdatst jahdatst.exe \
		$(HTMLDOCS) \
		$(DOS16)

veryclean: clean
	rm -f $(Z80CODE) $(MANPAGES) $(PDFMANPAGES) *.lst

link:	
	rm -f dosxtrs
	make dosxtrs

install: install-progs install-docs

install-progs: $(PROGS) $(CASSETTE)
	$(INSTALL) -d -m 755 $(BINDIR)
	$(INSTALL) -c -m 755 $(PROGS) $(BINDIR)
	$(INSTALL) -c -m 755 $(CASSETTE) $(BINDIR)/cassette

install-docs: docs
	$(INSTALL) -d -m 755 $(MANDIR)
	$(INSTALL) -d -m 755 $(MANDIR)/man1
	$(INSTALL) -c -m 644 xtrs.man $(MANDIR)/man1/xtrs.1
	$(INSTALL) -c -m 644 cassette.man $(MANDIR)/man1/cassette.1
	$(INSTALL) -c -m 644 mkdisk.man $(MANDIR)/man1/mkdisk.1
	$(INSTALL) -c -m 644 cmddump.man $(MANDIR)/man1/cmddump.1
	$(INSTALL) -c -m 644 hex2cmd.man $(MANDIR)/man1/hex2cmd.1
	$(INSTALL) -d -m 755 $(DOCDIR)
	$(INSTALL) -c -m 644 $(PDFMANPAGES) $(DOCDIR)
	$(INSTALL) -c -m 644 cpmutil.html $(DOCDIR)
	$(INSTALL) -c -m 644 cpmutil.txt $(DOCDIR)
	$(INSTALL) -c -m 644 dskspec.html $(DOCDIR)
	$(INSTALL) -c -m 644 dskspec.txt $(DOCDIR)

depend:
	makedepend -Y. --  -- *.c *.cpp 2>&1 | \
		(egrep -v 'cannot find|not in' || true)


keytrap/target/KEYTRAP.COM: keytrap/build.bash keytrap/keytrap.c  keytrap/scanbuf.h
	cd keytrap && bash build.bash

launcher/target/LAUNCHER.COM: launcher/build.bash launcher/launcher.c
	cd launcher && bash build.bash


idebuild: dosxtrs jahdatst

# DO NOT DELETE THIS LINE -- make depend depends on it.

cmddump.o: load_cmd.h
compile_rom.o: z80.h config.h load_cmd.h
debug.o: z80.h config.h trs.h
dis.o: z80.h config.h
error.o: z80.h config.h
hex2cmd.o: cmd.h z80.h config.h
jahdatst.o: z80.h config.h trs.h
load_cmd.o: load_cmd.h
load_hex.o: z80.h config.h
main.o: z80.h config.h trs.h trs_disk.h trs_hard.h load_cmd.h
mkdisk.o: reed.h
newutils.o: newutils.h trs.h z80.h config.h
trs_cassette.o: trs.h z80.h config.h newutils.h
trs_chars.o: trs_iodefs.h
trs_disk.o: z80.h config.h trs.h trs_disk.h trs_hard.h crc.c
trs_djgpp.o: trs_iodefs.h trs.h z80.h config.h trs_disk.h trs_uart.h
trs_djgpp.o: trs_hard.h trs_imp_exp.h keytrap/scanbuf.h trs_djgpp.h
trs_djgpp_modal.o: trs_iodefs.h trs.h z80.h config.h trs_disk.h trs_uart.h
trs_djgpp_modal.o: trs_hard.h trs_imp_exp.h trs_metafile.h newutils.h
trs_djgpp_modal.o: trs_djgpp.h
trs_hard.o: trs.h z80.h config.h trs_hard.h reed.h
trs_imp_exp.o: trs_imp_exp.h z80.h config.h trs.h trs_disk.h trs_hard.h
trs_interrupt.o: z80.h config.h trs.h
trs_io.o: z80.h config.h trs.h trs_disk.h trs_hard.h trs_uart.h
trs_keyboard.o: z80.h config.h trs.h scantran/generated_table.inc
trs_memory.o: z80.h config.h trs.h trs_disk.h trs_hard.h
trs_metafile.o: trs.h z80.h config.h newutils.h trs_metafile.h
trs_printer.o: z80.h config.h trs.h newutils.h
trs_realtime.o: z80.h config.h trs.h
trs_stringy.o: z80.h config.h trs.h trs_disk.h
trs_uart.o: trs.h z80.h config.h trs_uart.h trs_hard.h
trs_xinterface.o: trs_iodefs.h trs.h z80.h config.h trs_disk.h trs_uart.h
trs_xinterface.o: trs_hard.h trs_imp_exp.h
z80.o: z80.h config.h trs.h trs_imp_exp.h
trs_ich.o: z80.h config.h
