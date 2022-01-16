
#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdint>
#include <cstddef>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>

// Integers

using byte_t = unsigned char;

using Byte  = std::uint8_t;
using Int   = std::int32_t;
using UInt  = std::uint32_t;
using Long  = std::int64_t;
using ULong = std::uint64_t;
using USize = std::size_t;

// String

using String = std::string;

// Containers

template <typename E, USize size>
using Array = std::array<E, size>;

template <typename E, typename Alloc = std::allocator<E>>
using Vec = std::vector<E, Alloc>;

template <typename K, typename V, typename Cmp = std::less<K>>
using Map = std::map<K, V, Cmp>;

template <typename K, typename V, typename Cmp = std::less<K>>
using MultiMap = std::multimap<K, V, Cmp>;

template <typename K, typename V, typename Cmp = std::less<K>>
using HashMap = std::unordered_map<K, V, Cmp>;

template <typename E, typename Cmp = std::less<E>>
using Set = std::set<E, Cmp>;

template <typename E, typename Cmp = std::less<E>>
using HashSet = std::set<E, Cmp>;

// Function

template <typename Signature>
using Function = std::function<Signature>;

template <typename Tp>
using Consumer = Function<void(Tp)>;

template <typename Tp>
using Producer = Function<Tp(void)>;

template <typename Tp>
using Closure = Function<Tp(Tp)>;

// Pointers

template <typename Tp>
using Ref = std::shared_ptr<Tp>;

template <typename Tp>
using URef = std::unique_ptr<Tp>;

template <typename Tp>
using WRef = std::weak_ptr<Tp>;

// Defs

#define str     std::to_string
#define $       std::make_shared
#define mov(_x) std::move(_x)

#define NOP
#define EMPTY_STATEMENT { }
#define INVALID_STATE(Tp)  ((Tp) NULL, exit(1))

// Option

template <typename Entity>
class Option
{
  private:

    using Consumer = std::function<void(Entity)>;
    using Producer = std::function<Entity(void)>;
    using Action = std::function<void(void)>;

    bool present = false;
    Entity entity;

    Option() = default;

  public:

    explicit Option(Entity obj) : entity(mov(obj)), present(true) { }

    inline Entity unwarp() const
    {
        if (this->present) {
            return entity;
        } else {
            throw std::exception();
        }
    }

    inline Entity unwarpOrElse(const Producer & producer)
    {
        if (this->present) {
            return this->entity;
        } else {
            return producer();
        }
    }

    inline bool isPresent() const
    {
        return present;
    }

    inline Option<Entity> & ifPresent(const Consumer & consumer)
    {
        if (this->present) {
            consumer(entity);
        }
        return *this;
    }

    inline void orElse(const Action & action) const
    {
        if (!this->present) {
            action();
        }
    }

    inline static Option<Entity> empty()
    {
        return Option<Entity>();
    }
};

#endif // COMMON_HPP
