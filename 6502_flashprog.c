// Flash Programmer for the 6502 CPU using the CH32X035 Microcontroller
// Created by Matthew Kibarian 
// Date: 2025-05-27/28

#include "funconfig.h" // Include the configuration header for CH32X035
#include "ch32fun.h"
#include <stdio.h>

#define BUS_ENABLE PB12
#define FLASH_PROGB PC6
#define FLASH_WEB PA7

#define D0 PC15 // Data bus pins
#define D1 PA8
#define D2 PA9
#define D3 PA10
#define D4 PA11
#define D5 PA12
#define D6 PA13
#define D7 PA14
// const int databus[8] = {D0, D1, D2, D3, D4, D5, D6, D7}; // Array for data bus pins

#define A0 PA15 // Address bus pins
#define A1 PA16
#define A2 PA17 
#define A3 PA18 
#define A4 PA19 
#define A5 PA20 
#define A6 PA21 
#define A7 PA22 
#define A8 PA23 
#define A9 PA0
#define A10 PA1
#define A11 PA2
#define A12 PA3
#define A13 PA4
#define A14 PA5
#define A15 PA6 
// const int addrbus[16] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15}; // Array for address bus pins

void pin_init();
int read_databus();
void write_addressbus(int address);
void write_databus(int data);
void chip_erase();
void write_byte(int address, int data);

int main() {
    SystemInit();
    funGpioInitAll(); // Enable GPIOs
    funPinMode(BUS_ENABLE, FUN_OUTPUT); // Set BUS_ENABLE pin as output
    funPinMode(FLASH_PROGB, FUN_OUTPUT); // Set FLASH_PROGB pin as output
    funPinMode(FLASH_WEB, FUN_OUTPUT); // Set FLASH_WEB pin as output
    pin_init(); // initialize extended addrbus pins

    chip_erase(); // erase flash chip before programming

    funDigitalWrite(BUS_ENABLE, FUN_LOW); // set BUS_ENABLE to low
    int testaddr = 0x1234; // test address to write
    write_addressbus(testaddr); // write  the address to the address bus
}

void pin_init() { // initializes addrbus GPIO pins as outputs
    GPIOA->CFGLR |= 0x01111111;    // Use PA0 to PA6 to outputs
    GPIOA->CFGHR |= 0x10000000;    // Use PA15 as output
    GPIOA->CFGXR |= 0x11111111;    // Use PA16 to PA23 as outputs

}

int read_databus() { // reads the data bus (PC15, PA8 to PA14)
    int port_a = GPIOA->INDR;
    int port_c = GPIOC->INDR;
    const int C_MASK = 0x8000; // mask for PC15 (D0)
    const int A_MASK = 0x1FC00; // mask for PA8 to PA14 (D1 to D7)
    int databus_val = 0; // initialize the data bus value

    databus_val = ((port_c & C_MASK) >> 15) | ((port_a & A_MASK) >> 7); // combine the values from masked port A and port C
    return databus_val; // return the combined data from the data bus
}

void write_databus(int data) {
    int port_a = GPIOA->OUTDR;
    int port_c = GPIOC->OUTDR;
    const int C_MASK = 0x8000; // mask for PC15 (D0)
    const int A_MASK = 0x1FC00; // mask for PA8 to PA14 (D1 to D7)

    port_c = (port_c & C_MASK) | ((data & 0x01) << 15); // set D0 in port C
    port_a = (port_a & A_MASK) | ((data & 0xFE) << 7); // set D1 to D7 in port A
}

void write_addressbus(int address) {
    int flipped_addr = ((address & 0x1FF) << 15) | ((address & 0xFE00) >> 9); // flip and shift address bits to match wiring
    GPIOA->OUTDR = (GPIOA->OUTDR & ~0xFF807F) | flipped_addr; // set only the address bits in the GPIOA output data register
}

void chip_erase() {
    // performs chip erase for SST39SF010A flash chip

    // Flash OE# is controlled by NANDing prog# and A15, so set those low to set OE# high
    funDigitalWrite(FLASH_PROGB, FUN_LOW);
    funDigitalWrite(A15, FUN_LOW);

    int address_sequence[6] = {0x5555, 0x2AAA, 0x5555, 0x5555, 0x2AAA, 0x5555};
    int data_sequence[6] = {0xAA, 0x55, 0x80, 0xAA, 0x55, 0x10};

    for (int i = 0; i < 6; i++) {
        write_addressbus(address_sequence[i] | 0x8000);
        write_databus(data_sequence[i]);
        funDigitalWrite(FLASH_WEB, FUN_HIGH);
        Delay_Ms(1); // wait 1ms (>> T_wp min of 40ns)
        funDigitalWrite(FLASH_WEB, FUN_LOW);
    }
    Delay_Ms(100); // wait 100ms (T_SCE max) for chip erase to complete

    funDigitalWrite(FLASH_PROGB, FUN_HIGH); // set prog# high --> output is enabled
}

void write_byte(int address, int data) {
    // writes a byte to the specified address in the flash chip

    // set OE# high
    funDigitalWrite(FLASH_PROGB, FUN_LOW);
    funDigitalWrite(A15, FUN_LOW);

    int address_sequence[4] = {0x5555, 0x2AAA, 0x5555, address};
    int data_sequence[4] = {0xAA, 0x55, 0xA0, data};
    for (int i = 0; i < 4; i++) {
        write_addressbus(address_sequence[i] | 0x8000);
        write_databus(data_sequence[i]);
        funDigitalWrite(FLASH_WEB, FUN_HIGH);
        Delay_Ms(1); // wait 1ms (>> T_wp min of 40ns)
        funDigitalWrite(FLASH_WEB, FUN_LOW);
    }
    Delay_Us(10); // wait 10us (T_bp max of 10us for byte program)
}