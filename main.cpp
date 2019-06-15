#define MULTIPLE_PROBES
#define DATA_PIN        D7

#include "mbed.h"
#include "rtos.h"
#include "DS1820.h"

#define MAX_PROBES      16

#define SIG_BUTTON 0x1
#define POLL_DELAY 15000 // ms

DigitalOut led(LED1);

DS1820* probe[MAX_PROBES];
volatile int num_devices = 0;
Thread pollProbesThread;

InterruptIn button(USER_BUTTON);

  

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
 
int main() {  
    // Initialize the probe array to DS1820 objects
    pollProbesThread.start(pollProbes); // start polling thread
    button.rise(&buttonSignalISR); // hook up button to signal-setting ISR

    while(true) {
        led = !led;
        Thread::wait(500); // wait for 500 ms blinking an LED
    }
}
