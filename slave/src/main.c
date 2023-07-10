#include <avr/io.h>
#include <LCD.h>
#include <avr/interrupt.h>

unsigned char password_incorrect[14] = "Wrong password";
unsigned char password_correct[17] = "Access is granted";
unsigned char password[5] = "1234";
unsigned char input[4] = "----";
int temperature;
int lightning_range;
char show_password_flag = 0;
int counter = 0;
char keypad_input;

void show_array_on_lcd(unsigned char arr[], int size, int show_password_flag){
  if (show_password_flag){
    for(int i = 0; i < size; i++){
      LCD_write(arr[i]);
    }
  }
  else{
    for(int i = 0; i < size; i++){
      LCD_write('*');
    }
  }
}

int compare_two_array(unsigned char arr_1[], unsigned char arr_2[], int size){
  for(int i = 0; i < size; i++){
    if (arr_1[i] != arr_2[i]){
      return 0;
    }
  }
  return 1;
}

void lcd_delete_last_char() {
    if (counter > 0){
      if (counter > 0){
        counter -= 1;
      }
      input[counter] = '-';
      LCD_cmd(0x10); // sift corsure one character to left
      LCD_write(' ');
      LCD_cmd(0x10); // sift corsure one character to left
    }
}

int key_pad_action(char character){
  switch(character){
    case '*': // check password
      counter = 0;
      return compare_two_array(input, password, 4);
      break;

    case '#': // clear last character
      lcd_delete_last_char();
      break;

    case 'n': // null
      break;

    default:// print character
      if (counter < 4){
        if (show_password_flag){
          LCD_write(character);
        }
        else{
          LCD_write('*');
        }
        input[counter] = character;
        if (counter < 4){
          counter += 1;
        }
      }
  }
  return -1; // the password didn't check
}

int main(void) {
  char state = -1;

  // I/O initialization
  DDRD = 0b11110000; // portd4 as OC1B, portd5 as OC1A, portd6 as heater motor, portd7 as cooler motor
  DDRA |= (1<<PORTA7) | (1<<PORTA6) | (1<<PORTA2); // porta7->red led, porta6->blue led, porta2 as flag password true
  DDRA |= (1<<PORTA5) | (1<<PORTA4) | (1<<PORTA3); // lcd ctrl pin
  DDRC = 0xFF; // lcd input

  // timer/counter initialization for cooler and heater and lightning
	TCCR1A=(1<<COM1A1)|(1<<COM1B1)|(1<<WGM12)|(1<<WGM10); //non-inverting fast PWM 8-bit
	TCCR1B=(1<<CS11)|(1<<CS10); //1::64 Pre-Scaler
	// OC1A as cooler and heater duty cycle
	OCR1AH = 0;
	OCR1AL = 0;
	// OC1B as lightning duty cycle
	OCR1BH = 0;
	OCR1BL = 0;

  // spi initialization for slave
  DDRB = 0b01000000; // portb4 as slave select, INT2

  GICR |= (1 << INT2); //INT2 enable

  sei();
  init_LCD();
  
  PORTA &= ~(1 << PORTA2); // flag password false
  SPCR |= (1 << SPE);
  while(1){
    // reciving keypad and take action
    while ((SPSR & (1 << SPIF)) == 0);
    state = key_pad_action(SPDR);
    if(state == 1){
      // show access message
      LCD_cmd(0x01); // clear lcd screen
      show_array_on_lcd(password_correct, 17, 1);
      PORTA |= (1 << PORTA2); // flag password true

      while (1){
        // geting temperature form master
        while (((SPSR >> SPIF) & 1) == 0);
        temperature = SPDR;
        // checking temperature
        if (temperature > 55){
          PORTA ^= (1 << PORTA7); // red led blink
          PORTD &= 0b00111111; // cooler and heater off
        }
        else if (temperature == 0){
          PORTA ^= (1 << PORTA6); // blue led blink
          PORTD &= 0b00111111; // cooler and heater off
        }
        else if ((25 < temperature) & (temperature < 55)){
          OCR1AL = ((float)(50 + ((int)temperature - 25)*2)/100)*255;
          PORTD |= (1 << PORTD7); // cooler on
          PORTD &= 0b10111111; // heater off
          PORTA &= 0b00111111; //red and blue led off
        }
        else if ((0 < temperature) & (temperature < 20)){
          OCR1AL = ((float)(100 - ((int)temperature*5))/100)*255;
          PORTD |= (1 << PORTD6); // heater on
          PORTD &= 0b01111111; // cooler off
          PORTA &= 0b00111111; //red and blue led off
        }
        else{
          PORTD &= 0b00111111; // cooler and heater off
          PORTA &= 0b00111111; //red and blue led off
        }

        // geting light range from masster
        while (((SPSR >> SPIF) & 1) == 0);
        lightning_range = SPDR;        
        // set lightning duty cycle
        if((0 <= lightning_range) & (lightning_range < 55)){
          OCR1BL = 255;
        }
        else if((55 <= lightning_range) & (lightning_range < 92)){
          OCR1BL = ((float)75/100)*255;
        }
        else if((92 <= lightning_range) & (lightning_range < 121)){
          OCR1BL = ((float)50/100)*255;
        }
        else{
          OCR1BL = ((float)25/100)*255;
        }
      }
    }
    else if(state == 0){
      // show access denied message
      LCD_cmd(0x01); // clear lcd screen
      show_array_on_lcd(password_incorrect, 14, 1);
      LCD_cmd(0x01); // clear lcd screen
    }
  }
}

ISR(INT2_vect){
  show_password_flag ^= 1;
  LCD_cmd(0x01); // clear screen
  show_array_on_lcd(input, counter, show_password_flag);
}