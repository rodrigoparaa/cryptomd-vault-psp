# Dual Makefile: Compiles for PSP hardware (EBOOT.PBP) or macOS host (test / cli)

ifneq ($(shell which psp-config 2>/dev/null),)
    # PSP SDK Environment Active
    TARGET = CryptoMDVault
    OBJS = main.o \
           core/crypto_primitives.o \
           core/cipher_classic.o \
           core/cipher_aes256.o \
           core/cipher_chacha20.o \
           core/pdoc_container.o \
           core/md_parser.o \
           core/scroll_physics.o \
           core/input_logic.o \
           core/app_state.o \
           platform/psp_entropy.o \
           platform/psp_fs.o \
           platform/psp_input.o \
           platform/psp_power.o \
           ui/graphics.o \
           ui/theme_engine.o \
           ui/font_8x8.o \
           ui/font_7seg.o \
           ui/screens.o

    INCDIR = core platform ui
    CFLAGS = -O2 -G0 -Wall -Wextra -std=c99 -D__PSP__
    CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
    ASFLAGS = $(CFLAGS)

    LIBDIR =
    LIBS = -lpspgu -lpspdisplay -lpsprtc -lpsppower -lpspctrl -lm

    EXTRA_TARGETS = EBOOT.PBP
    PSP_EBOOT_TITLE = CryptoMD Vault v1.0
    PSP_EBOOT = EBOOT.PBP
    BUILD_PRX = 1

    PSPSDK = $(shell psp-config --pspsdk-path)
    include $(PSPSDK)/lib/build.mak

else
    # Host Environment (macOS / Linux)
    CC = clang
    CFLAGS = -std=c99 -Wall -Wextra -Werror -O2 -I./core -I./platform -I./ui -lm

    all: test cli
		@echo "Host build completed. For PSP EBOOT.PBP, run with PSPDEV toolchain or via Docker."

endif

# Host target: Unit tests
test:
	$(CC) $(CFLAGS) \
	core/crypto_primitives.c \
	core/cipher_classic.c \
	core/cipher_aes256.c \
	core/cipher_chacha20.c \
	core/pdoc_container.c \
	core/md_parser.c \
	core/scroll_physics.c \
	core/input_logic.c \
	core/app_state.c \
	tests/test_core.c \
	-o tests/test_core
	./tests/test_core

# Host target: Companion CLI
cli:
	$(CC) $(CFLAGS) \
	core/crypto_primitives.c \
	core/cipher_classic.c \
	core/cipher_aes256.c \
	core/cipher_chacha20.c \
	core/pdoc_container.c \
	tools/cryptodoc_cli.c \
	-o tools/cryptodoc_cli

clean_host:
	rm -f tests/test_core tools/cryptodoc_cli *.o core/*.o platform/*.o ui/*.o
