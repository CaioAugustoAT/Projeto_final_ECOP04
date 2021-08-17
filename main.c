/*
 * Author: Caio Augusto Adami Telles
 *
 * Link:
 */

#include <xc.h>
#include "config.h"
#include "bits.h"
#include "lcd.h"
#include "atraso.h"
#include "pwm.h"
#include "adc.h"
#include "i2c.h"
#include "keypad.h"
#include <pic18f4520.h>



#define L_ON 0x0F
#define L_OFF 0x08
#define L_CLR 0x01
#define L_L1 0x80
#define L_L2 0xC0
#define L_L3 0x90
#define L_L4 0xD0

unsigned int kp = 2,
        ki = 0.2,
        kd = 0.005,
        erro_med = 0,
        temp_setP,
        temp_med,
        temp_ult_med,
        proporcional,
        integral,
        derivativo,
        pid;
//int pwm = 128;

void mostralogo(void);
void aprsenta(void);
void inicio(void);
void controle_pid();
void finaliza(void);

void itoa(unsigned int val, char* str);
void itoa1(unsigned int val1, char* str1);

void main(void) {
    unsigned char i, j;


    ADCON1 = 0x06;
    TRISA = 0xC3;
    TRISB = 0xF0;
    TRISC = 0x00;
    TRISD = 0x00;
    TRISE = 0x00;

    lcd_init();
    pwmInit();
    adc_init();

    aprsenta();
    mostralogo();

    while (PORTBbits.RB5);
    for (i = 0; i < 10; i++) {
        bitSet(TRISC, 1);
        PORTB = i;
        PORTD = i;
        atraso_ms(50);
    }
    bitClr(TRISC, 1);

    atraso_ms(1000);
    lcd_cmd(L_CLR);

    inicio();
    controle_pid();
    finaliza();

    for (;;);
    return;
}

void mostralogo(void) {
    unsigned char i, j;
    lcd_cmd(0x40);

    char logo[48] = {
        0x01, 0x03, 0x03, 0x0E, 0x1C, 0x18, 0x08, 0x08,
        0x11, 0x1F, 0x00, 0x01, 0x1F, 0x12, 0x14, 0x1F,
        0x10, 0x18, 0x18, 0x0E, 0x07, 0x03, 0x02, 0x02,
        0x08, 0x18, 0x1C, 0x0E, 0x03, 0x03, 0x01, 0x00,
        0x12, 0x14, 0x1F, 0x08, 0x00, 0x1F, 0x11, 0x00,
        0x02, 0x03, 0x07, 0x0E, 0x18, 0x18, 0x10, 0x00,
    };


    for (i = 0; i < 48; i++) {
        lcd_dat(logo[i]);
    }

    lcd_cmd(L_L3 + 6);
    lcd_dat(0);
    lcd_dat(1);
    lcd_dat(2);
    lcd_cmd(L_L4 + 6);
    lcd_dat(3);
    lcd_dat(4);
    lcd_dat(5);
}

void aprsenta(void) {

    unsigned int i;
    lcd_cmd(0x0C); //liga       
    lcd_cmd(L_L1);
    lcd_str("     ECOP04");
    lcd_cmd(L_L2);
    lcd_str(" Trabalho Final");
    atraso_ms(1000);

    
}

void inicio(void) {

    lcd_cmd(L_L1);
    lcd_str("Entre com o ");
    lcd_cmd(L_L2);
    lcd_str("Setpoint");
    lcd_cmd(L_L3);
    lcd_str("Para a temperatu");
    lcd_cmd(L_L4);
    lcd_str("ra desejada:");
    atraso_ms(5000);
}

void finaliza(void) {
    PORTCbits.RC5 = 0;
    lcd_cmd(L_CLR);
    lcd_cmd(0x0C); //liga       
    lcd_cmd(L_L1 + 3);
    lcd_str("Obrigado!");
    lcd_cmd(L_L2);
    lcd_str(" Trabalho Final");
    lcd_cmd(L_L3 + 5);
    lcd_str("ECOP04");
}

void controle_pid() {
    unsigned int i,valor;
    do {
        char str[6], grau = 223;
        char str1[6];
        unsigned int temp_med;
        /*
             25 --  77 
              0 --  1023
         
             X = 77-25/1023 =0,050830889540567
         */
        //Estou amostrando a temperatura
        temp_setP = (((adc_amostra(0)*0.050830889540567) + 25)*10);
        itoa(temp_setP, str);


        lcd_cmd(L_CLR);
        lcd_str("Ref = ");
        lcd_dat(str[2]);
        lcd_dat(str[3]);
        lcd_dat(',');
        lcd_dat(str[4]);
        lcd_dat(grau);
        lcd_dat('C');

        TRISA = 0x07;

        temp_med = (adc_amostra(1)*10) / 2;
        itoa(temp_med, str);
        lcd_cmd(L_L2);
        lcd_str("Temp: ");
        itoa(temp_med, str);
        lcd_dat(str[2]);
        lcd_dat(str[3]);
        lcd_dat(',');
        lcd_dat(str[4]);
        lcd_dat(grau);
        lcd_dat('C');

        erro_med = abs(temp_setP - temp_med);
        proporcional = erro_med*kp; //kp
        integral += erro_med*ki; //ki
        derivativo = (temp_ult_med - temp_med) * kd; //kd
        temp_ult_med = temp_med;

        pid = proporcional + integral + derivativo;

        if (temp_setP < temp_med) {
            PORTCbits.RC5 = 0;
        } else {
            PORTCbits.RC5 = 1;
        }

        if (temp_setP > temp_med) {
            pwmSet1(0);
        } else {
            pwmSet1(pid);
        }


        lcd_cmd(L_L3);
        lcd_str("Erro: ");
        itoa(erro_med, str);
        lcd_dat(str[2]);
        lcd_dat(str[3]);

        lcd_cmd(L_L4);
        lcd_str("PWM: ");
        itoa(pid, str);
        lcd_dat(str[2]);
        lcd_dat(str[3]);
        atraso_ms(1600);
        
               
    } while (PORTBbits.RB5);
}

void itoa(unsigned int val, char* str) {
    str[0] = (val / 10000) + 0x30;
    str[1] = ((val % 10000) / 1000) + 0x30;
    str[2] = ((val % 1000) / 100) + 0x30;
    str[3] = ((val % 100) / 10) + 0x30;
    str[4] = (val % 10) + 0x30;
    str[5] = 0;
}

void itoa1(unsigned int val1, char* str1) {
    str1[0] = (val1 / 10000) + 0x30;
    str1[1] = ((val1 % 10000) / 1000) + 0x30;
    str1[2] = ((val1 % 1000) / 100) + 0x30;
    str1[3] = ((val1 % 100) / 10) + 0x30;
    str1[4] = (val1 % 10) + 0x30;
    str1[5] = 0;
}