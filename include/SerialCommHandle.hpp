
#ifndef COMMHANDLE_HPP
#define COMMHANDLE_HPP

#include "SerialControl.hpp"
#include "CommandFrame.hpp"
#include "Option.hpp"
#include "Common.hpp"

#include <thread>
#include <mutex>

using Mutex  = std::mutex;
using Thread = std::thread;

class SerialCommHandle
{
  private:

    SerialControl serialPort {};
    byte_t sof = 0x05;

    struct SubscriberBase
    {
        virtual uint16_t cmd() = 0;
        virtual void receive(uint8_t* data) = 0;
    };

    using SubscriberPtr = Ref<SubscriberBase>;
    std::unordered_map<uint16_t, SubscriberPtr> subscribers;

    Mutex serialPortMutex;
    Thread receivingDaemonThread;

    Function<void()> receivingDaemon();

  public:

    template <uint16_t Cmd, typename CmdData>
    class Publisher
    {
      private:

        SerialControl* serialPort;
        Mutex* serialPortMutex;
        byte_t sof;

        uint8_t cmd();

        explicit Publisher(SerialControl* port, Mutex* mutex, byte_t sof = 0x05)
            : serialPort(port), serialPortMutex(mutex), sof(sof)
        { EMPTY_STATEMENT }

      public:

        bool publish(const CmdData & data);
    };

    template <typename CmdData>
    using Callback = Function<void(const CmdData&)>;

  private:

    template <uint16_t Cmd, typename CmdData>
    class Subscriber : public SubscriberBase
    {
      private:

        Callback<CmdData> callback;
        void receive(uint8_t* data) override;

      public:

        uint16_t cmd() override;
        explicit Subscriber(Callback<CmdData> callback) : callback(callback) { }

    };

  public:

    explicit SerialCommHandle(const SerialControl & serialPortControl, byte_t sof = 0x05);

    explicit SerialCommHandle(const String & tty, int baudRate = B115200, byte_t sof = 0x05);

    template <uint16_t Cmd, typename CmdData>
    Publisher<Cmd, CmdData> publisher()
    {
        return SerialCommHandle::Publisher<Cmd, CmdData>(this->serialPort);
    }

    template <uint16_t Cmd, typename CmdData>
    void subscribe(Callback<CmdData> callback)
    {
        SubscriberPtr subscriber = std::make_shared<Subscriber<Cmd, CmdData>>(callback);
        subscribers[Cmd] = subscriber;
    }

    bool startReceiving();

    Thread & getReceivingDaemonThread();
};

template <uint16_t Cmd, typename CmdData>
void SerialCommHandle::Subscriber<Cmd, CmdData>::receive(uint8_t* data)
{
    auto* cmdData = reinterpret_cast<CmdData*>(data);
    this->callback(*cmdData);
}

template <uint16_t Cmd, typename CmdData>
uint16_t SerialCommHandle::Subscriber<Cmd, CmdData>::cmd()
{
    return Cmd;
}


#endif // COMMHANDLE_HPP
