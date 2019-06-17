#include "mbed.h"
#include "rtos.h"
#include "DS1820.h"

#define MULTIPLE_PROBES
#define DATA_PIN D7 // DS18B20 temperature sensors connected to this pin
#define RELAY_OUT D2 // cooling device relay board channel connected to this pin
#define MAX_PROBES 16
#define SIG_BUTTON 0x1 // button signal

#define POLL_DELAY 5000 // ms

Serial pc(USBTX, USBRX);
DigitalOut led(LED1);

DS1820* probe[MAX_PROBES];
volatile int num_devices = 0;
Thread pollProbesThread;

InterruptIn button(USER_BUTTON); // user button input to re-scan DS18B20 devices

DigitalOut cooler_relay(RELAY_OUT); // output to relay board for controlling cooling device

const float TEMP_SETPOINT = 10.0; // °C
const float TEMP_HYSTERESIS = 0.5; // °C

void enumerateProbes() {
    num_devices = 0;
    while(DS1820::unassignedProbe(DATA_PIN)) {
        probe[num_devices] = new DS1820(DATA_PIN);
        probe[num_devices]->setResolution(12);
        printf("-DEBUG- Found device %d with ID %llX\n", num_devices, probe[num_devices]->getROMvalue());
        num_devices++;
        if (num_devices == MAX_PROBES)
            break;
    }
    
    printf("-DEBUG- Found %d device(s)\n", num_devices);
}

void pollProbes() {
    enumerateProbes();
    while(true) {
        probe[0]->convertTemperature(true, DS1820::all_devices); // blocking call
        for (int i = 0; i<num_devices; i++)
            printf("%02d %2.2f\n", i, probe[i]->temperature());
        osEvent result = Thread::signal_wait(SIG_BUTTON, POLL_DELAY); // wait for signal or timeout
        if (result.status == osEventSignal) // signalled to refresh probe list
            enumerateProbes();
    }
}

void buttonSignalISR() {
    pollProbesThread.signal_set(SIG_BUTTON); // signal thread to refresh list of probes
}
 
void serialRecv() {
    char commandChar = pc.getc(); // get command character recieved from computer
    switch (commandChar) {
        case 'r':
            pollProbesThread.signal_set(SIG_BUTTON); // signal thread to refresh probes
            break;
        default:
            break;
    }
}

int main() {  
    // Initialize the probe array to DS1820 objects
    pollProbesThread.start(pollProbes); // start polling thread
    button.rise(&buttonSignalISR); // hook up button to signal-setting ISR

    while(true) {
        led = !led;
        Thread::wait(500); // wait for 500 ms blinking an LED
    }
}
