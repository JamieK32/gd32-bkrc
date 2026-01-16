/*

ds18b20.c
created: 10/1/2025
author:
*/
#include "ds18b20.h"
#include "gd32f4xx.h"

//连接P7端口 PD15

// Fahrenheit to Celsius conversion formula: F = C * 9/5 + 32
// Celsius from Fahrenheit: C = (F - 32) * 5/9

void ds18b20_delay_us(uint32_t us) {
	delay_us(us);
}


ds18b20_t ds18b20 = {
    .init = DS18B20_Initialize,
    .get_temperature = DS18B20_GetTemperature,
};


// Initialize the DS18B20's IO pin and check for presence
// Returns 1: not present
// Returns 0: present
unsigned char DS18B20_Initialize(void)
{
		rcu_periph_clock_enable(DS18B20_RTC);
	  gpio_output_options_set(DS18B20_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_200MHZ, DS18B20_PIN);
    DS18B20_SetPinAsOutput();
    DS18B20_SetPinOutput(1);
    DS18B20_Reset();
    return DS18B20_CheckPresence();
}


// Set data pin output
void DS18B20_SetPinOutput(int value)
{
	gpio_bit_write(DS18B20_PORT, DS18B20_PIN, (bit_status)value);
}

// Set data pin as input
void DS18B20_SetPinInput(void)
{
	gpio_mode_set(DS18B20_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, DS18B20_PIN);
}

// Set data pin as output
void DS18B20_SetPinAsOutput(void)
{
	gpio_mode_set(DS18B20_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, DS18B20_PIN);
}

// Reset the DS18B20 sensor
void DS18B20_Reset(void)
{
    DS18B20_SetPinAsOutput();  // Set pin as output
    DS18B20_SetPinOutput(0);   // Pull pin low
    ds18b20_delay_us(480);              // Hold low for 480us
    DS18B20_SetPinOutput(1);   // Release the pin
    ds18b20_delay_us(15);               // Wait for 15us
}

// Wait for the DS18B20's response
// Returns 1: DS18B20 not detected
// Returns 0: DS18B20 detected
unsigned char DS18B20_CheckPresence(void)
{
    unsigned char retry = 0;
    DS18B20_SetPinInput(); // Set pin as input
    while (gpio_input_bit_get(DS18B20_PORT, DS18B20_PIN) && retry < 200)
    {
        retry++;
        ds18b20_delay_us(1);
    }
    if (retry >= 200)
        return 1; // Not detected
    retry = 0;
    while (!gpio_input_bit_get(DS18B20_PORT, DS18B20_PIN) && retry < 240)
    {
        retry++;
        ds18b20_delay_us(1);
    }
    if (retry >= 240)
        return 1; // Not detected
    return 0; // Detected
}

// Read a single bit from the DS18B20
// Returns 1 or 0
unsigned char DS18B20_ReadBit(void)
{
    unsigned char data;
    DS18B20_SetPinAsOutput(); // Set pin as output
    DS18B20_SetPinOutput(0);
    ds18b20_delay_us(2);
    DS18B20_SetPinOutput(1);
    DS18B20_SetPinInput(); // Set pin as input
    ds18b20_delay_us(11);
    data = gpio_input_bit_get(DS18B20_PORT, DS18B20_PIN) & 0x01; // Read the bit
    ds18b20_delay_us(50);
    return data;
}

// Read a byte from the DS18B20
// Returns the read data
unsigned char DS18B20_ReadByte(void)
{
    unsigned char i, bitValue, byteValue = 0;
    for (i = 0; i < 8; i++)
    {
        bitValue = DS18B20_ReadBit();
        byteValue = (bitValue << 7) | (byteValue >> 1);
    }
    return byteValue;
}

// Write a byte to the DS18B20
// dat: byte to write
void DS18B20_WriteByte(unsigned char dat)
{
    unsigned char j, bitValue;
    DS18B20_SetPinAsOutput(); // Set pin as output
    DS18B20_SetPinOutput(1);
    for (j = 0; j < 8; j++)
    {
        bitValue = dat & 0x01; // Get the least significant bit
        dat >>= 1; // Shift right

        if (bitValue)
        {
            DS18B20_SetPinOutput(0); // Write 1
            ds18b20_delay_us(2);
            DS18B20_SetPinOutput(1);
            ds18b20_delay_us(60);
        }
        else
        {
            DS18B20_SetPinOutput(0); // Write 0
            ds18b20_delay_us(60);
            DS18B20_SetPinOutput(1);
            ds18b20_delay_us(2);
        }
    }
}

// Start temperature conversion
void DS18B20_StartConversion(void)
{
    DS18B20_Reset();
    DS18B20_CheckPresence();
    DS18B20_WriteByte(0xcc); // Skip ROM
    DS18B20_WriteByte(0x44); // Start conversion
}


// Get temperature value from DS18B20
// Precision: 0.1°C
// Returns temperature value (-55.0°C to +125.0°C)
float DS18B20_GetTemperature(void)
{
    unsigned char temp;
    unsigned char LSB, MSB;
    short rawTemperature;
    DS18B20_StartConversion();
    DS18B20_Reset();
    DS18B20_CheckPresence();
    DS18B20_WriteByte(0xcc); // Skip ROM
    DS18B20_WriteByte(0xbe); // Read scratchpad
    LSB = DS18B20_ReadByte(); // Read LSB
    MSB = DS18B20_ReadByte(); // Read MSB
    if (MSB > 7)
    {
        MSB = ~MSB;
        LSB = ~LSB;
        temp = 0; // Temperature is negative
    }
    else
        temp = 1; // Temperature is positive

    rawTemperature = MSB; // Get high byte
    rawTemperature <<= 8;
    rawTemperature += LSB; // Get low byte
    float temperatureValue = (float)rawTemperature * 0.0625f; // Convert to Celsius
    return temp ? temperatureValue : -temperatureValue; // Return temperature value
}
