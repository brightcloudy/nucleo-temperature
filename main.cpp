#include "mbed.h"
#include "rtos.h"
#include "DS1820.h"

#define MULTIPLE_PROBES
#define MAX_PROBES 16

#define DATA_PIN D7 // DS18B20 temperature sensors connected to this pin
#define RESET_PIN D8 // pin to reset device

#define NUM_RELAYS 4
#define RELAY_ONE D2 // nightlight relay is connected to this pin
#define RELAY_TWO D3 // cooling device relay board channel connected to this pin
#define RELAY_THREE D4
#define RELAY_FOUR D5

#define SIG_ANY 0x0 // any signal
#define SIG_RESET 0x1 // button signal
#define SIG_SELECT 0x2 // select new temperature sensor for control

#define POLL_DELAY 1000 // ms

const float TEMP_SETPOINT = 10.0; // °C
const float TEMP_HYSTERESIS = 0.5; // °C

Serial pc(USBTX, USBRX);
DigitalOut led(LED1);
DigitalOut reset_pin(RESET_PIN, 1); // initialize NRST connected pin high
osThreadId main_thread;

DS1820* probe[MAX_PROBES];
DS1820* selected_probe;
volatile int num_devices = 0;
volatile int selected_sensor = 0;
Thread pollProbesThread;

DigitalOut* relay_outputs[4];

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

void switchRelays(uint32_t relays, int value) {
    for (int i = 0; i < NUM_RELAYS; i++) {
      if (relays & (1 << i))
        relay_outputs[i]->write(value);
    }
}

void pollProbes() {
    enumerateProbes();
    selected_probe = probe[selected_sensor];
    while(true) {
        probe[0]->convertTemperature(true, DS1820::all_devices); // blocking call
        for (int i = 0; i<num_devices; i++)
            printf("%02d %2.2f\n", i, probe[i]->temperature());
        float selectedTemp = selected_probe->temperature();
        printf("-DEBUG- Control probe #%02d: %2.2f\n", selected_sensor, selectedTemp);
        if (selectedTemp <= (TEMP_SETPOINT - TEMP_HYSTERESIS)) {
            if (!relay_outputs[0]->read())
                switchRelays((uint32_t) (1 | 2), 1); // turn off relays one and two
            printf("-DEBUG- Relays OFF\n");
        } else if (selectedTemp > (TEMP_SETPOINT + TEMP_HYSTERESIS)) {
            if (relay_outputs[0]->read())
                switchRelays((uint32_t) (1 | 2), 0); // turn on relays one and two
            printf("-DEBUG- Relays ON\n");
        }
        osEvent result = Thread::signal_wait(SIG_SELECT, POLL_DELAY); // wait for signal or timeout
        if (result.status == osEventSignal) {
            if (result.value.signals & SIG_SELECT) {
              selected_probe = probe[selected_sensor]; // update selected probe pointer
              printf("-DEBUG- Updated selected probe to sensor #%d\n", selected_sensor);
            }
        }
    }
}
 
void serialRecv() {
    char commandChar = pc.getc(); // get command character recieved from computer
    switch (commandChar) {
        case 'r':
            osSignalSet(main_thread, SIG_RESET); // signal main thread by ID for reset
            break;
        case '0':
            selected_sensor = 0; // set selected sensor for control to 0
            pollProbesThread.signal_set(SIG_SELECT); // signal thread to update pointer
            break;
        case '1':
            selected_sensor = 1; // set selected sensor for control to 0
            pollProbesThread.signal_set(SIG_SELECT); // signal thread to update pointer
            break;
        default:
            break;
    }
}

int main() {  
    // initialize our array of relay outputs
    // i wish there were a better way to do this but the relays are kind of arbitrary -RK
    relay_outputs[0] = new DigitalOut(RELAY_ONE, 1);
    relay_outputs[1] = new DigitalOut(RELAY_TWO, 1);
    relay_outputs[2] = new DigitalOut(RELAY_THREE, 1);
    relay_outputs[3] = new DigitalOut(RELAY_FOUR, 1);

    // make main thread ID available for signalling 
    main_thread = Thread::gettid();

    // Initialize the probe array to DS1820 objects
    pollProbesThread.start(pollProbes); // start polling thread

    pc.attach(&serialRecv); // attach serial recieve interrupt to ISR

    while(true) {
        led = !led;
        osEvent result = Thread::signal_wait(SIG_RESET, 500); // wait for 500 ms to blink led or recieve reset signal
        if (result.status == osEventSignal) {
            printf("-DEBUG- RESET\n");
            reset_pin.write(0); // reset the device
        }
    }
}
