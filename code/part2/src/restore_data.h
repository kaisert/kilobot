#ifndef SAVE_DATA_H
#define SAVE_DATA_H

#include "../BA2.h"

#define START_EEPROM 0x100

extern void restore_data(uint8_t *, uint8_t *, struct list *, struct list *, struct references *);

#endif
