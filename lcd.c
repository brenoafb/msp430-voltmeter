#include "lcd.h"

// Configurar Pinos I2C - UCSB0
// P3.0 = SDA e P3.1=SCL
void config_I2C(void){
  P3SEL |=  BIT1 | BIT0;    // Use dedicated module
  P3REN |=  BIT1 | BIT0;    // Resistor enable
  P3OUT |=  BIT1 | BIT0;    // Pull-up

  UCB0CTL1 |= UCSWRST;    // UCSI B0 em ressete

  UCB0CTL0 = UCSYNC |     //Síncrono
    UCMODE_3 |   //Modo I2C
    UCMST;       //Mestre

  UCB0BRW = BR100K;       //100 kbps
  //UCB0BRW = BR50K;      // 20 kbps
  //UCB0BRW = BR10K;      // 10 kbps
  UCB0CTL1 = UCSSEL_2;   //SMCLK e remove ressete
}



// Incializar LCD modo 4 bits
void LCD_inic(void){

  PCF_STT_STP();      //Colocar PCF em estado conhecido

  // Preparar I2C para operar
  UCB0I2CSA = PCF_ADR;    //Endereço Escravo
  UCB0CTL1 |= UCTR    |   //Mestre TX
    UCTXSTT;    //Gerar START
  while ( (UCB0IFG & UCTXIFG) == 0);          //Esperar TXIFG=1
  UCB0TXBUF = 0;                              //Saída PCF = 0;
  while ( (UCB0CTL1 & UCTXSTT) == UCTXSTT);   //Esperar STT=0
  if ( (UCB0IFG & UCNACKIFG) == UCNACKIFG)    //NACK?
    while(1);

  // Começar inicialização
  LCD_aux(0);     //RS=RW=0, BL=1
  delay(20000);
  LCD_aux(3);     //3
  delay(10000);
  LCD_aux(3);     //3
  delay(10000);
  LCD_aux(3);     //3
  delay(10000);
  LCD_aux(2);     //2

  // Entrou em modo 4 bits
  LCD_aux(2);     LCD_aux(8);     //0x28
  LCD_aux(0);     LCD_aux(8);     //0x08
  LCD_aux(0);     LCD_aux(1);     //0x01
  LCD_aux(0);     LCD_aux(6);     //0x06
  LCD_aux(0);     LCD_aux(0xF);   //0x0F

  while ( (UCB0IFG & UCTXIFG) == 0)   ;          //Esperar TXIFG=1
  UCB0CTL1 |= UCTXSTP;                           //Gerar STOP
  while ( (UCB0CTL1 & UCTXSTP) == UCTXSTP)   ;   //Esperar STOP
  delay(50);
}


// Auxiliar inicialização do LCD (RS=RW=0)
// *** Só serve para a inicialização ***
void LCD_aux(char dado){
  while ( (UCB0IFG & UCTXIFG) == 0);              //Esperar TXIFG=1
  UCB0TXBUF = ((dado<<4)&0XF0) | BIT_BL;          //PCF7:4 = dado;
  delay(50);
  while ( (UCB0IFG & UCTXIFG) == 0);              //Esperar TXIFG=1
  UCB0TXBUF = ((dado<<4)&0XF0) | BIT_BL | BIT_E;  //E=1
  delay(50);
  while ( (UCB0IFG & UCTXIFG) == 0);              //Esperar TXIFG=1
  UCB0TXBUF = ((dado<<4)&0XF0) | BIT_BL;          //E=0;
}


// Gerar START e STOP para colocar PCF em estado conhecido
void PCF_STT_STP(void){
  int x=0;
  UCB0I2CSA = PCF_ADR;   		//Endereço Escravo

  while (x<5){
    UCB0CTL1 |= UCTR    |   //Mestre TX
      UCTXSTT;    //Gerar START
    while ( (UCB0IFG & UCTXIFG) == 0);	//Esperar TXIFG=1
    UCB0CTL1 |= UCTXSTP;    			//Gerar STOP
    delay(200);
    if ( (UCB0CTL1 & UCTXSTP) == 0)   break;   //Esperar STOP
    x++;
  }
  while ( (UCB0CTL1 & UCTXSTP) == UCTXSTP);   //I2C Travado (Desligar / Ligar)
}


// Ler a porta do PCF
int PCF_read(void){
  int dado;
  UCB0I2CSA = PCF_ADR;        		//Endereço Escravo
  UCB0CTL1 &= ~UCTR;     				//Mestre RX
  UCB0CTL1 |= UCTXSTT;   				//Gerar START
  while ( (UCB0CTL1 & UCTXSTT) == UCTXSTT);
  UCB0CTL1 |= UCTXSTP;    			//Gerar STOP + NACK
  while ( (UCB0IFG & UCRXIFG) == 0);	//Esperar RX
  dado=UCB0RXBUF;
  delay(50);
  return dado;
}


// Escrever dado na porta
void PCF_write(char dado){
  UCB0I2CSA = PCF_ADR;        //Endereço Escravo
  UCB0CTL1 |= UCTR    |       //Mestre TX
    UCTXSTT;        //Gerar START
  while ( (UCB0IFG & UCTXIFG) == 0)   ;          //Esperar TXIFG=1
  UCB0TXBUF = dado;                              //Escrever dado
  while ( (UCB0CTL1 & UCTXSTT) == UCTXSTT)   ;   //Esperar STT=0
  if ( (UCB0IFG & UCNACKIFG) == UCNACKIFG)       //NACK?
    while(1);                          //Escravo gerou NACK
  UCB0CTL1 |= UCTXSTP;    					//Gerar STOP
  while ( (UCB0CTL1 & UCTXSTP) == UCTXSTP)   ;   //Esperar STOP
  delay(50);
}

void delay(long limite){
  volatile long cont=0;
  while (cont++ < limite) ;
}

void lcd_nibble(char n) {
  PCF_write((n << 4) + 0x9);
  PCF_write((n << 4) + 0xd);
  PCF_write((n << 4) + 0x9);
}

void lcd_char(char c) {
  char n1 = (c >> 4) % 0x10;
  char n2 = c % 0x10;

  lcd_nibble(n1);
  lcd_nibble(n2);
}

void lcd_str(char *str) {
  int n = strlen(str);
  int i;
  for (i = 0; i < n; i++) {
    lcd_char(str[i]);
  }
}

int lcd_ocupado() {
  // TODO
  return 0;
}

// converter digito numérico para representação ascii hex
char hex_ascii(unsigned char h) {
  if (h >= 0 && h <= 9) {
    return h + 0x30;
  } else if (h <= 0xf) {
    return h + 0x37;
  } else {
    return 0;
  }
}

// converter digito numérico para representação ascii decimal
char dec_ascii(unsigned char h) {
  if (h >= 0 && h <= 9) {
    return h + 0x30;
  } else return 0;
}

void lcd_hex8(unsigned char n) {
  int arr[2];

  arr[0] = n % 0x10;
  arr[1] = (n / 0x10) % 0x10;
  
  if (arr[1] != 0) lcd_char(hex_ascii(arr[1]));
  lcd_char(hex_ascii(arr[0]));
}

void lcd_hex16(unsigned int n) {
  int arr[4];

  arr[0] = n % 0x10;
  arr[1] = (n /= 0x10) % 0x10;
  arr[2] = (n /= 0x10) % 0x10;
  arr[3] = (n /= 0x10) % 0x10;

  if (arr[3] != 0)
    lcd_char(hex_ascii(arr[3]));
  if (!(arr[3] == 0 && arr[2] == 0))
    lcd_char(hex_ascii(arr[2]));
  if (!(arr[3] == 0 && arr[2] == 0 && arr[1] == 0))
    lcd_char(hex_ascii(arr[1]));
  lcd_char(hex_ascii(arr[0]));
}

void lcd_dec8(unsigned char n) {
  int arr[3];

  arr[0] = n % 10;
  arr[1] = (n /= 10) % 10;
  arr[2] = (n /= 10) % 10;

  if (arr[2] != 0)
    lcd_char(dec_ascii(arr[2]));
  if (!(arr[2] == 0 && arr[1] == 0))
    lcd_char(dec_ascii(arr[1]));
  lcd_char(dec_ascii(arr[0]));
}

void lcd_dec16(unsigned int n) {
  int arr[5];

  arr[0] = n % 10;
  arr[1] = (n /= 10) % 10;
  arr[2] = (n /= 10) % 10;
  arr[3] = (n /= 10) % 10;
  arr[4] = (n /= 10) % 10;

  if (arr[4] != 0)
    lcd_char(dec_ascii(arr[4]));

  if (!(arr[4] == 0 && arr[3] == 0))
    lcd_char(dec_ascii(arr[3]));

  if (!(arr[4] == 0 && arr[3] == 0 && arr[2] == 0))
    lcd_char(dec_ascii(arr[2]));

  if (!(arr[4] == 0 && arr[3] == 0 && arr[2] == 0 && arr[1] == 0))
    lcd_char(dec_ascii(arr[1]));

  lcd_char(dec_ascii(arr[0]));
}

void lcd_float(float x) {
  int d;
  d = x;
  lcd_char(dec_ascii(d));
  lcd_char('.');
  x *= 10;
  x -= d * 10;

  int i;
  for (i = 0; i < 3; i++) {
    d = x;
    lcd_char(dec_ascii(d));
    x *= 10;
    x -= d * 10;
  }
}

void lcd_cursor(unsigned char lin, unsigned char col) {
  unsigned char addr = 0;
  if (lin == 1) addr += 0x40;
  addr += col;

  unsigned char di = addr & 0xf0;
  unsigned char df = (addr & 0xf) << 4;

  PCF_write(BIT_D7 + BIT_BL + di);
  PCF_write(BIT_D7 + BIT_BL + di + BIT_E);
  PCF_write(BIT_D7 + BIT_BL + di);

  PCF_write(BIT_BL + df);
  PCF_write(BIT_BL + df + BIT_E);
  PCF_write(BIT_BL + df);
}

void lcd_clear() {
  PCF_write(BIT_BL);
  PCF_write(BIT_BL + BIT_E);
  PCF_write(BIT_BL);

  PCF_write(BIT_BL + BIT_D4);
  PCF_write(BIT_BL + BIT_D4 + BIT_E);
  PCF_write(BIT_BL + BIT_D4);
}

void lcd_return() {
  PCF_write(BIT_BL);
  PCF_write(BIT_BL + BIT_E);
  PCF_write(BIT_BL);

  PCF_write(BIT_BL + BIT_D5);
  PCF_write(BIT_BL + BIT_D5 + BIT_E);
  PCF_write(BIT_BL + BIT_D5);
}
