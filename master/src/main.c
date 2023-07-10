#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

float temperature;
float lightning_range;

int char_array_to_int(unsigned char arr[], int size){
  int result = 0;
  for(int i = 0; i < size; i++){
    result += (arr[i] - 0)*pow(10, size - i);
  }
  return result;
}

char reading_from_key_pad(){
  /*working with port d0...d6*/
  //column 1
  PORTD |= 0b01111110;
  PORTD &= 0b11111110;
  if (((PIND & 0b00001000) | (PIND & 0b00000001)) == 0){ // 1
    return '1';
  }
  if (((PIND & 0b00010000) | (PIND & 0b00000001)) == 0){ // 4
    return '4';
  }
  if (((PIND & 0b00100000) | (PIND & 0b00000001)) == 0){ // 7
    return '7';
  }
  if (((PIND & 0b01000000) | (PIND & 0b00000001)) == 0){ // *
    return '*';
  }
  //column 2
  PORTD |= 0b01111101;
  PORTD &= 0b11111101;
  if (((PIND & 0b00001000) | (PIND & 0b00000010)) == 0){ // 2
    return '2';
  }
  if (((PIND & 0b00010000) | (PIND & 0b00000010)) == 0){ // 5
    return '5';
  }
  if (((PIND & 0b00100000) | (PIND & 0b00000010)) == 0){ // 8
    return '8';
  }
  if (((PIND & 0b01000000) | (PIND & 0b00000010)) == 0){ // 0
    return '0';
  }
  //column 3
  PORTD |= 0b01111011;
  PORTD &= 0b11111011;
  if (((PIND & 0b00001000) | (PIND & 0b00000100)) == 0){ // 3
    return '3';
  }
  if (((PIND & 0b00010000) | (PIND & 0b00000100)) == 0){ // 6
    return '6';
  }
  if (((PIND & 0b00100000) | (PIND & 0b00000100)) == 0){ // 9
    return '9';
  }
  if (((PIND & 0b01000000) | (PIND & 0b00000100)) == 0){ // #
    return '#';
  }
  return 'n';
}

int main(void) {
  //initial I/O
  DDRA &= ~(1 << PORTA2); // porta2 as a flag to know that you have access to system.
  DDRA |= (1 << PORTA3); // output for toggle show password
  DDRD |= 0b00000111; // reading from keypad with port d

  //select intrupt pin for show password
  GICR |= (1 << INT2); // INT2

  // ADC initialization
  ADMUX |= (1 << REFS0); //AVCC
  ADCSRA = (1 << ADEN) | (0 << ADIE) | (1 << ADPS2) | (0 << ADPS1) | (1 << ADPS0); // enable interrupt & prescale = 8
  ADCSRA |= (1 << ADSC); // start first conversion

  //spi initialization
  DDRB = (1<<DDB7) | (1<<DDB5) | (1<<DDB4); // portb4 as slave select
  SPCR = (1<<MSTR) | (0<<CPOL);
  PORTD |= (1 << PORTD4); // select slave

  sei();

  SPCR |= (1 << SPE);
  while((PINA & (1 << PORTA2)) == 0){
    // sending keypad character
    SPDR = reading_from_key_pad();
    while((SPSR & (1 << SPIF)) == 0);
    _delay_ms(100);
  }
  while (1){
    char ignore;
    // geting temperture
    ADMUX &= 0b11100000;
    ADCSRA |= (1 << ADSC); // start next ADC conversion
    while ((ADCSRA & (1 << ADIF)) == 0){}; // wait until the conversion is complete
    temperature = (unsigned int) ADCW;
    temperature /= 1024;
    temperature *= 500;
    // send int temperature to slave
    SPDR = (int)temperature;
    while(((SPSR >> SPIF) & 1) == 0);
    ignore = SPDR;

    // geting lightning duty cycle
    ADMUX |= (1 << MUX0);
    ADCSRA |= (1 << ADSC); // start next ADC conversion
    while ((ADCSRA & (1 << ADIF)) == 0){}; // wait until the conversion is complete
    lightning_range = (unsigned int) ADCW;
    lightning_range /= 1024;
    lightning_range *= 500;
    // send int light range to slave
    SPDR = (int)lightning_range;
    while(((SPSR >> SPIF) & 1) == 0);
    ignore = SPDR;
  }
}

ISR(INT2_vect){
 PORTA ^= (1 << PORTA3);
}