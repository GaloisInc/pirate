#include "serialize.hpp"

#include <cstdint>
#include <optional>

template <typename T>
struct Response {
    Response(uint64_t id, T t) : tx_id(id), content(t) {}
    uint64_t tx_id;
    std::optional<T> content;
};

template<typename T>
struct Serialize<Response<T>> {
    static constexpr size_t size = sizeof(uint64_t) + Serialize<T>::size;
    static Response<T> fromBuffer(char const* buffer) {
        return Response<T>(deserialize<uint64_t>(buffer), deserialize<T>(buffer + sizeof (uint64_t)));
    }
    static void toBuffer(std::vector<char> & buffer, Response<T> const& res) {
        serialize(buffer, res.tx_id);
        serialize(buffer, res.content);
    }
};