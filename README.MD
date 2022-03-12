# Serial Comm Control
### A lightweight library for serial port communication.

## Example

### Sender

```c++
#include "SerialCommHandle.hpp"

#define CMD_POS 0x01 // command id

struct Vec2f
{
    float x, y;
};

int main()
{
    SerialCommHandle comm("/dev/ttyACM0"); // create a serial communication instance
    auto positionPublisher = comm.publisher<CMD_POS, Vec2f>();

    while (true)
    {
        Vec2f pos {};
        // ...
        positionPublisher.publish(pos);
    }
}
```

### Receiver

```c++
#include "SerialCommHandle.hpp"
#include <iostream>

#define CMD_POS 0x01

struct Vec2f
{
    float x, y;
};

int main()
{
    SerialControl serialControl; // you can also open a serial device manually
    serialControl.open("/dev/ttyACM0", B115200);
    
    SerialCommHandle comm(serialControl);
    
    comm.subscribe<CMD_POS, Vec2f>( // call back function here
        [](const Vec2f & pos)
        {
            std::cout << "x: " << pos.x << ", y: " << pos.y;
            std::cout << std::endl;
        }
    );
    
    comm.startReceiving(); // start the receiving daemon thread
}
```