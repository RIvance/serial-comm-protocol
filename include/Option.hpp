#ifndef OPTION_HPP
#define OPTION_HPP

#include <type_traits>
#include <functional>

template <typename Data>
class Option
{
  private:

    using RawData = typename std::aligned_storage<
        sizeof(Data), std::alignment_of<Data>::value
    >::type;

    bool present = false;
    RawData rawData;

  public:

    using Consumer = std::function<void(Data)>;
    using Producer = std::function<Data(void)>;

    static Option None()
    {
        return Option();
    }

    Option() : present(false) { }

    explicit Option(const Data & v)
    {
        create(v);
    }

    explicit Option(Data && v) : present(false)
    {
        create(std::move(v));
    }

    ~Option()
    {
        destroy();
    }

    Option(const Option & other) : present(false)
    {
        if (other.isPresent()) {
            assign(other);
        }
    }

    Option(Option && other) noexcept : present(false)
    {
        if (other.isPresent()) {
            assign(std::move(other));
            other.destroy();
        }
    }

    Option & operator = (Option && other) noexcept
    {
        assign(std::move(other));
        return *this;
    }

    Option & operator = (const Option & other)
    {
        assign(other);
        return *this;
    }

    template <class... Args>
    void emplace(Args && ... args)
    {
        destroy();
        create(std::forward<Args>(args)...);
    }

    bool isPresent() const { return  present; }
    bool isEmpty()   const { return !present; }

    Data const & unwrap()
    {
        if (isPresent()) {
            present = false;
            return *((Data*) (&rawData));
        } else {
            throw std::exception();
        }
    }

    Data const & unwrapOr(const Data & other)
    {
        if (isPresent()) {
            present = false;
            return *((Data*) (&rawData));
        } else {
            return other;
        }
    }

    Data const & unwrapOrProduce(const Producer & producer)
    {
        if (isPresent()) {
            present = false;
            return *((Data*) (&rawData));
        } else {
            return producer();
        }
    }

    Data const & unwrapOr(Data && other)
    {
        if (isPresent()) {
            present = false;
            return *((Data*) (&rawData));
        } else {
            return other;
        }
    }

    void ifPresent(const Consumer & consumer)
    {
        if (isPresent()) {
            consumer(std::forward<Data>(unwrap()));
        }
    }

    Data const & operator * ()
    {
        return unwrap();
    }

    bool operator == (const Option<Data> & rhs) const
    {
        return (!bool(*this)) == (!rhs) && (!bool(*this) || (*(*this)) == (*rhs));
    }

    bool operator != (const Option<Data> & rhs) const
    {
        return *this != rhs;
    }

  private:

    template <class... Args>
    void create(Args && ... args)
    {
        new(&rawData) Data(std::forward<Args>(args)...);
        present = true;
    }

    void destroy()
    {
        if (present) {
            present = false;
            ((Data*)(&rawData))->~Data();
        }
    }

    void assign(const Option & other)
    {
        if (other.isPresent()) {
            copy(other.rawData);
            present = true;
        } else {
            destroy();
        }
    }

    void assign(Option && other)
    {
        if (other.isPresent()) {
            move(std::move(other.rawData));
            present = true;
            other.destroy();
        } else {
            destroy();
        }
    }

    void move(RawData && val)
    {
        destroy();
        new(&rawData) Data(std::move(*((Data*) (&val))));
    }

    void copy(const RawData & val)
    {
        destroy();
        new(&rawData) Data(*((Data*) (&val)));
    }
};

#endif // OPTION_HPP
