
#include "CommHandle.hpp"

CommHandle::CommHandle(const String & tty, int baudRate)
{
    while (!this->serialPort.open(tty, baudRate)) {
        std::cout << "Cannot open serial port " << tty << ", retrying..." << std::endl;
    }
}

template<typename T>
int CommHandle::send(int commandId, T data)
{
    CommandFrame<T> frame(commandId, data);
    return serialPort.send(frame.toBytes());
}

template<typename T>
Option<T> CommHandle::receive()
{
    auto data = serialPort.receive(CommandFrame<T>::dataSize());
    return CommandFrame<T>::parse(data);
}

CommHandle::CommHandle(const SerialPort &serialPortControl)
{
    this->serialPort = serialPortControl;
}
