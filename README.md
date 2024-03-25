### Code for "iiiiiiiiii", an installation at "Junge Kunst aus Nordeuropa", an exhibition at Neues Kunsthaus Ahrenshoop from 23. of March to June 2024

Uses the raspberry pi pico SDK and a PIO audio PWM solution heavily inspired by the official one but much more lightweight.
Voltage from a Piezo transducer is translated into an audio signal.

### Build

to build, install the raspberry pi pico C sdk according to their instructions. there's no other dependencies so
```
> mkdir build
> cd build
> cmake ..
> make -j4
```
should build the rest
