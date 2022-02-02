
#include "CommHandle.hpp"

#define func auto

CommHandle::CommHandle(const String & tty, int baudRate)
{
    while (!this->serialPort.open(tty, baudRate)) {
        std::cout << "Cannot open serial port " << tty << ", retrying..." << std::endl;
    }
}

template<typename T>
func CommHandle::send(int commandId, T data) -> int
{
    CommandFrame<T> frame(commandId, data);
    return serialPort.send(frame.toBytes());
}

template<typename T>
func CommHandle::receive() -> Option<T>
{
    auto data = serialPort.receive(CommandFrame<T>::dataSize());
    return CommandFrame<T>::parse(data);
}

CommHandle::CommHandle(const SerialPort &serialPortControl)
{
    this->serialPort = serialPortControl;
}
