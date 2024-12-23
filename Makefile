#
# Makefile for xtrs, the TRS-80 emulator.
# $Id$
#

OBJECTS = \
	target/dos/z80.o \
	target/dos/main.o \
	target/dos/load_cmd.o \
	target/dos/load_hex.o \
	target/dos/trs_memory.o \
	target/dos/trs_keyboard.o \
	target/dos/error.o \
	target/dos/debug.o \
	target/dos/dis.o \
	target/dos/trs_io.o \
	target/dos/trs_cassette.o \
	target/dos/trs_chars.o \
	target/dos/trs_printer.o \
	target/dos/trs_rom1.o \
	target/dos/trs_rom3.o \
	target/dos/trs_rom4p.o \
	target/dos/trs_disk.o \
	target/dos/trs_interrupt.o \
	target/dos/trs_imp_exp.o \
	target/dos/trs_hard.o \
	target/dos/trs_uart.o \
	target/dos/trs_stringy.o


DOS_OBJECTS = \
	target/dos/trs_djgpp.o \
	target/dos/trs_realtime.o \
	target/dos/trs_djgpp_modal.o \
	target/dos/trs_metafile.o \
	target/dos/trs_ich.o \
	target/dos/newutils.o \
	target/dos/trs_vga.o \
	target/dos/trs_patterns.o \
	target/dos/dpmhw/dpmhw.o \
	target/dos/dpmhw/dpmhw_pci.o

CR_OBJECTS = \
	target/dos/compile_rom.o \
	target/dos/error.o \
	target/dos/load_cmd.o \
	target/dos/load_hex.o

LOCAL_CR_OBJECTS = $(subst /dos/,/local/,$(CR_OBJECTS))

MD_OBJECTS = \
	target/dos/mkdisk.o

LOCAL_MD_OBJECTS = $(subst /dos/,/local/,$(MD_OBJECTS))

HC_OBJECTS = \
	target/dos/cmd.o \
	target/dos/error.o \
	target/dos/load_hex.o \
	target/dos/hex2cmd.o

LOCAL_HC_OBJECTS = $(subst /dos/,/local/,$(HC_OBJECTS))

CD_OBJECTS = \
	target/dos/cmddump.o \
	target/dos/load_cmd.o

LOCAL_CD_OBJECTS = $(subst /dos/,/local/,$(CD_OBJECTS))

JT1_OBJECTS = \
	target/dos/jahdatst.o \
	target/dos/trs_ich.o \
	target/dos/error.o \
	target/dos/dpmhw/dpmhw.o \
	target/dos/dpmhw/dpmhw_pci.o


VE_OBJECTS = target/dos/video-experiments.o \
	target/dos/trs_vga.o

DOS16 = \
	launcher/target/LAUNCHER.COM \
	keytrap/target/KEYTRAP.COM
	

Z80CODE = target/z80/export.cmd target/z80/import.cmd target/z80/settime.cmd target/z80/xtrsmous.cmd \
	target/z80/xtrs8.dct target/z80/xtrshard.dct \
	target/z80/fakerom.hex target/z80/xtrsrom4p.hex target/z80/esfrom.hex

MANPAGES = xtrs.txt mkdisk.txt cassette.txt cmddump.txt hex2cmd.txt

PDFMANPAGES = cassette.man.pdf \
	cmddump.man.pdf \
	hex2cmd.man.pdf \
	mkdisk.man.pdf \
	xtrs.man.pdf

HTMLDOCS = cpmutil.txt \
	dskspec.txt

PROGS = target/dos/dosxtrs.exe target/dos/mkdisk.exe target/dos/hex2cmd.exe target/dos/cmddump.exe target/dos/jahdatst.exe target/dos/videxp.exe

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
# -fno-exceptions 
CXXFLAGS += -fno-exceptions $(DEBUG) $(ENDIAN) $(DEFAULT_ROM) $(READLINE) $(DISKDIR) $(IFLAGS) \
	$(APPDEFAULTS) -DKBWAIT
LIBS = $(XLIB) $(READLINELIBS) $(EXTRALIBS)

ZMACFLAGS = -h

.SUFFIXES: .dct .man .txt .html

target/deps/%.o:
	mkdir -p target/deps && mkdir -p $(@D) && touch $@

target/dos/%.o: %.c target/deps/%.o
	mkdir -p $(@D) && $(CC) $(CFLAGS) $(CPPFLAGS) -fverbose-asm -save-temps=obj -c -o $@ $<

target/dos/%.o: %.cpp target/deps/%.o
	mkdir -p $(@D) && $(CXX) $(CPPFLAGS) $(CXXFLAGS) -fverbose-asm  -save-temps=obj -c -o $@ $<

target/local/%.o: %.c target/deps/%.o
	mkdir -p target/local && $(BUILD_CC) -c -o $@ $<

target/z80/%.cmd : target/z80/%.hex  target/local/hex2cmd
	./target/local/hex2cmd target/z80/$*.hex > target/z80/$*.cmd

target/z80/%.dct : target/z80/%.hex  target/local/hex2cmd
	./target/local/hex2cmd target/z80/$*.hex > target/z80/$*.dct

target/z80/%.hex : %.z80  $(ZMACINT)
	mkdir -p target/z80 && $(ZMACINT) $(ZMACFLAGS) -o target/z80/$*.hex -x target/z80/$*.lst $<

.man.txt:
	nroff -man -c -Tascii $< | colcrt - | cat -s > $*.txt

.html.txt:
	lynx -dump  $< >$@

%.man.pdf: %.man
	groff -Tpdf -man $< > $@

target/dos/dosxtrs.exe: $(OBJECTS) $(DOS_OBJECTS)
	$(CC) $(LDFLAGS) -o target/dos/dosxtrs.exe $(OBJECTS) $(DOS_OBJECTS) $(LIBS)

target/dos/compile_rom.exe: $(CR_OBJECTS)
	$(CC) $(LDFLAGS) -o target/dos/compile_rom.exe $(CR_OBJECTS)

target/local/compile_rom: $(LOCAL_CR_OBJECTS)
	$(BUILD_CC) $(LDFLAGS) -o target/local/compile_rom $(LOCAL_CR_OBJECTS)

trs_rom1.c: target/local/compile_rom $(BUILT_IN_ROM)
	./target/local/compile_rom 1 $(BUILT_IN_ROM) > trs_rom1.c

trs_rom3.c: target/local/compile_rom $(BUILT_IN_ROM3)
	./target/local/compile_rom 3 $(BUILT_IN_ROM3) > trs_rom3.c

trs_rom4p.c: target/local/compile_rom $(BUILT_IN_ROM4P)
	./target/local/compile_rom 4p $(BUILT_IN_ROM4P) > trs_rom4p.c

target/dos/mkdisk.exe:	$(MD_OBJECTS)
	$(CC) $(LDFLAGS) -o target/dos/mkdisk.exe $(MD_OBJECTS)

target/local/mkdisk:	$(LOCAL_MD_OBJECTS)
	$(BUILD_CC) $(LDFLAGS) -o target/local/mkdisk $(LOCAL_MD_OBJECTS)

target/dos/hex2cmd.exe: $(HC_OBJECTS)
	$(CC) $(LDFLAGS) -o target/dos/hex2cmd.exe $(HC_OBJECTS)

target/local/hex2cmd: $(LOCAL_HC_OBJECTS)
	$(BUILD_CC) $(LDFLAGS) -o target/local/hex2cmd $(LOCAL_HC_OBJECTS)

target/dos/cmddump.exe: $(CD_OBJECTS)
	$(CC) $(LDFLAGS) -o target/dos/cmddump.exe $(CD_OBJECTS)

target/local/cmddump: $(LOCAL_CD_OBJECTS)
	$(BUILD_CC) $(LDFLAGS) -o target/local/cmddump $(LOCAL_CD_OBJECTS)

target/dos/jahdatst.exe: $(JT1_OBJECTS)
	$(CC) $(LDFLAGS) -o target/dos/jahdatst.exe $(JT1_OBJECTS)

target/dos/videxp.exe: $(VE_OBJECTS)
	$(CC) $(LDFLAGS) -o target/dos/videxp.exe $(VE_OBJECTS)

clean:
	rm -rf target && rm -f \
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
	makedepend -ptarget/deps/ -Y. --  -- *.c *.cpp dpmhw/*.cpp 2>&1 | \
		(egrep -v 'cannot find|not in' || true)


keytrap/target/KEYTRAP.COM: keytrap/build.bash keytrap/keytrap.c  keytrap/scanbuf.h
	cd keytrap && bash build.bash

launcher/target/LAUNCHER.COM: launcher/build.bash launcher/launcher.c
	cd launcher && bash build.bash


idebuild: target/dos/dosxtrs.exe target/dos/jahdatst.exe target/dos/videxp.exe

# DO NOT DELETE THIS LINE -- make depend depends on it.

target/deps/cmddump.o: load_cmd.h
target/deps/compile_rom.o: z80.h config.h load_cmd.h
target/deps/debug.o: z80.h config.h trs.h
target/deps/dis.o: z80.h config.h
target/deps/error.o: z80.h config.h
target/deps/hex2cmd.o: cmd.h z80.h config.h
target/deps/jahdatst.o: z80.h config.h trs.h
target/deps/load_cmd.o: load_cmd.h
target/deps/load_hex.o: z80.h config.h
target/deps/main.o: z80.h config.h trs.h trs_disk.h trs_hard.h load_cmd.h
target/deps/mkdisk.o: reed.h
target/deps/newutils.o: newutils.h trs.h z80.h config.h
target/deps/trs_cassette.o: trs.h z80.h config.h newutils.h
target/deps/trs_chars.o: trs_iodefs.h
target/deps/trs_disk.o: z80.h config.h trs.h trs_disk.h trs_hard.h crc.c
target/deps/trs_djgpp.o: trs_iodefs.h trs.h z80.h config.h trs_disk.h
target/deps/trs_djgpp.o: trs_uart.h trs_hard.h trs_imp_exp.h
target/deps/trs_djgpp.o: keytrap/scanbuf.h trs_djgpp.h trs_vga.h
target/deps/trs_djgpp_modal.o: trs_iodefs.h trs.h z80.h config.h trs_disk.h
target/deps/trs_djgpp_modal.o: trs_uart.h trs_hard.h trs_imp_exp.h
target/deps/trs_djgpp_modal.o: trs_metafile.h newutils.h trs_djgpp.h
target/deps/trs_hard.o: trs.h z80.h config.h trs_hard.h reed.h
target/deps/trs_imp_exp.o: trs_imp_exp.h z80.h config.h trs.h trs_disk.h
target/deps/trs_imp_exp.o: trs_hard.h
target/deps/trs_interrupt.o: z80.h config.h trs.h
target/deps/trs_io.o: z80.h config.h trs.h trs_disk.h trs_hard.h trs_uart.h
target/deps/trs_keyboard.o: z80.h config.h trs.h scantran/generated_table.inc
target/deps/trs_memory.o: z80.h config.h trs.h trs_disk.h trs_hard.h
target/deps/trs_metafile.o: trs.h z80.h config.h newutils.h trs_metafile.h
target/deps/trs_patterns.o: trs_iodefs.h z80.h config.h
target/deps/trs_printer.o: z80.h config.h trs.h newutils.h
target/deps/trs_realtime.o: z80.h config.h trs.h
target/deps/trs_stringy.o: z80.h config.h trs.h trs_disk.h
target/deps/trs_uart.o: trs.h z80.h config.h trs_uart.h trs_hard.h
target/deps/trs_xinterface.o: trs_iodefs.h trs.h z80.h config.h trs_disk.h
target/deps/trs_xinterface.o: trs_uart.h trs_hard.h trs_imp_exp.h
target/deps/z80.o: z80.h config.h trs.h trs_imp_exp.h
target/deps/trs_ich.o: z80.h config.h
target/deps/trs_vga.o: trs.h z80.h config.h trs_vga.h trs_iodefs.h
target/deps/dpmhw/dpmhw.o: dpmhw/dpmhw.h dpmhw/dpmhw_impl.h z80.h config.h
target/deps/dpmhw/dpmhw_pci.o: dpmhw/dpmhw_pci.h dpmhw/dpmhw.h
target/deps/dpmhw/dpmhw_pci.o: dpmhw/dpmhw_impl.h
