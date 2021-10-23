#include "SerialPort.hpp"

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <termios.h> /* POSIX terminal control definitions */

#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open a tty serial port
 * @param tty  tty device, like "/dev/ttyUSB0"
 * @return  File descriptor of the tty device
 * @reference https://www.cmrr.umn.edu/~strupp/serial.html
 */
int openPort(const char* tty, int flags)
{
    int fd; /* File descriptor for the port */

    /**
     * @def O_RDWR     read and write flag
     * @def O_NOCTTY   prevent the process to be a tty control process
     * @def O_NDELAY   non-blocking mode
     */
    fd = open(tty, O_RDWR | O_NOCTTY | O_NDELAY | flags);
    if (fd == -1)
    {
        // Could not open the port.
        perror("open_port: Unable to open /dev/ttyf1 - ");
    }
    else
    {
        fcntl(fd, F_SETFL, 0);
    }

    return fd;
}

#define ADD_FLAG(dst, src)  dst |= (src)
#define RM_FLAG(dst, src)   dst &= ~(src)

inline bool fileAccessible(int fd)
{
    struct stat buf {};
    return fstat(fd, &buf) != -1;
}

inline int _baud(int baudRate)
{
    if (baudRate < B0) return -1;
    if (
        (baudRate >= B0 && baudRate <= B38400) ||
        (baudRate >= B57600 && baudRate <= __MAX_BAUD)
    ) {
        return baudRate;
    } else {
        switch (baudRate) {
            case 0:       return B0;
            case 50:      return B50;
            case 75:      return B75;
            case 110:     return B110;
            case 134:     return B134;
            case 150:     return B150;
            case 200:     return B200;
            case 300:     return B300;
            case 600:     return B600;
            case 1200:    return B1200;
            case 1800:    return B1800;
            case 2400:    return B2400;
            case 4800:    return B4800;
            case 9600:    return B9600;
            case 19200:   return B19200;
            case 38400:   return B38400;
            case 57600:   return B57600;
            case 115200:  return B115200;
            case 230400:  return B230400;
            case 460800:  return B460800;
            case 500000:  return B500000;
            case 576000:  return B576000;
            case 921600:  return B921600;
            case 1000000: return B1000000;
            case 1152000: return B1152000;
            case 1500000: return B1500000;
            case 2000000: return B2000000;
            case 2500000: return B2500000;
            case 3000000: return B3000000;
            case 3500000: return B3500000;
            case 4000000: return B4000000;
            default: return -1;
        }
    }
}

#define BAUD(X) _baud(X)

#ifdef __cplusplus
}; // end extern "C"
#endif

bool SerialPort::open(const String &ttyPathname, int baudRate, int flags)
{
    if ((baudRate = BAUD(baudRate)) == -1) return false;
    this->fileDescriptor = ::openPort(ttyPathname.c_str(), baudRate | flags);
    return this->fileDescriptor != -1;
}

int SerialPort::send(void* data, size_t size) const
{
    ssize_t bytesWritten = ::write(this->fileDescriptor, data, size);
    return bytesWritten == -1 ? 0 : (int) bytesWritten;
}

template<typename T>
int SerialPort::send(T* data) const
{
    return this->send(data, sizeof(T));
}

template<typename T>
int SerialPort::send(const T & data) const
{
    return this->send(&data, sizeof(T));
}

bool SerialPort::isOpen() const
{
    return ::fileAccessible(this->fileDescriptor);
}

void SerialPort::close() const
{
    ::close(this->fileDescriptor);
}

void SerialPort::setBaudRate(int baud) const
{
    termios options {};
    // Get the current options for the port
    ::tcgetattr(this->fileDescriptor, &options);
    ::cfsetispeed(&options, baud);
    ::cfsetospeed(&options, baud);
    // Enable the receiver and set local mode
    ADD_FLAG(options.c_cflag, CLOCAL | CREAD);
    // Set the new options for the port
    ::tcsetattr(this->fileDescriptor, TCSANOW, &options);
}

void SerialPort::addFlag(int flag) const
{
    termios options {};
    // Get the current options for the port
    ::tcgetattr(this->fileDescriptor, &options);
    ADD_FLAG(options.c_cflag, flag);
    // Set the new options for the port
    ::tcsetattr(this->fileDescriptor, TCSANOW, &options);
}

void SerialPort::removeFlag(int flag) const
{
    termios options {};
    // Get the current options for the port
    ::tcgetattr(this->fileDescriptor, &options);
    RM_FLAG(options.c_cflag, flag);
    // Set the new options for the port
    ::tcsetattr(this->fileDescriptor, TCSANOW, &options);
}

int SerialPort::receive(void *data, size_t size) const
{
    return read(this->fileDescriptor, data, size);
}
