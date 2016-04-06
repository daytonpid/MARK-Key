# Makefile

MCU     = at90usb162
F_CPU   = 16000000
FORMAT  = ihex

FLAGS   = -std=c99
FLAGS  += -O2
FLAGS  += -funsigned-char
FLAGS  += -funsigned-bitfields
FLAGS  += -ffunction-sections
FLAGS  += -fpack-struct
FLAGS  += -fshort-enums
FLAGS  += -Wall
FLAGS  += -Wstrict-prototypes

CC      = avr-gcc
OBJCOPY = avr-objcopy
REMOVE  = rm -f
FLASH   = dfu-programmer


TARGET  = teensy

SRC     = main.c \
	  usb_serial.c

OBJ = $(patsubst %.c,%.o,$(SRC))
CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL -I. $(FLAGS)

all: $(TARGET).hex $(TARGET).eep

flash: $(TARGET).hex $(TARGET).eep
	@echo Erasing ...
	@$(FLASH) $(MCU) erase
	@echo Flash program ...
	@$(FLASH) $(MCU) flash $(TARGET).hex
	@echo Flash eeprom ...
	@$(FLASH) $(MCU) flash-eeprom $(TARGET).eep
	@echo Done!

start:
	@echo Restarting $(MCU)
	@$(FLASH) $(MCU) start


%.hex: %.elf
	@echo " [OBJCOPY] $@"
	@$(OBJCOPY) -O $(FORMAT) -R .eeprom -R .fuse -R .lock -R .signature $< $@

%.eep: %.elf
	@echo " [OBJCOPY] $@"
	@$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 --no-change-warnings -O $(FORMAT) $< $@

%.elf: $(OBJ)
	@echo " [LD]      $@"
	@$(CC) $(CFLAGS) -o $@ $^

%.o : %.c
	@echo " [CC]      $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

clean:
	@echo Cleaning up ...
	@$(REMOVE) $(TARGET).elf
	@$(REMOVE) $(OBJ)

distclean: clean
	@$(REMOVE) $(TARGET).hex
	@$(REMOVE) $(TARGET).eep

.PHONY: all flash start clean distclean
.PRECIOUS: %.o $(TARGET).elf
