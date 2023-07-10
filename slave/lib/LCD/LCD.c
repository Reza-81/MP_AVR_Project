#include <avr/io.h>
#include <util/delay.h>

#define LCD_DATA PORTC
#define ctrl PORTA
// #define en PINA.5
// #define rw PINA.4                     
// #define rs PINA.3

void LCD_cmd(unsigned char cmd);
void init_LCD(void);
void LCD_write(unsigned char data);
void init_LCD(void)
{
    LCD_cmd(0x38);         // 8-bit mode                   
    _delay_ms(1);
    LCD_cmd(0x01);         // clear the screen                     
    _delay_ms(1);
    LCD_cmd(0x0E);         // turn on the cursor                
    _delay_ms(1);
    LCD_cmd(0x80);         // move cursor to the first place of the first row   
    _delay_ms(1);
    return;
}

void LCD_cmd(unsigned char cmd)
{
    LCD_DATA=cmd;
    ctrl |= (1 << PORTA5);  // Register Select = 0, Read/Write = 0, Enable = 1
    ctrl &= 0b11100111;
    _delay_ms(1);
    ctrl &= 0b11011111;     // Enable = 0
    _delay_ms(50);
    return;
}

void LCD_write(unsigned char data)
{
    LCD_DATA= data;
    ctrl |= (1 << PORTA5) | (1 << PORTA3);  // Register Select = 1, Read/Write = 0, Enable = 1
    ctrl &= 0b11101111;
    _delay_ms(1);
    ctrl &= 0b11011111;     // Enable = 0
    _delay_ms(50);
    return ;
}