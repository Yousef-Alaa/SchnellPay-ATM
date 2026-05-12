#ifndef BITMASKING_H
#define BITMASKING_H

#define set_bit(reg,bit) (reg |= (1<<bit))
#define clear_bit(reg,bit) (reg &= ~(1<<bit))
#define toggle_bit(reg,bit) (reg ^= (1<<bit))
#define read_bit(reg,bit) ((reg >> bit) & 1)

#endif
