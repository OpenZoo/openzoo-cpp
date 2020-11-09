#ifndef __WORLD_SERIALIZER_H__
#define __WORLD_SERIALIZER_H__

#include <cstddef>
#include <cstdint>
#include <functional>
#include "utils.h"

namespace ZZT {
    struct Board;
    struct World;

    class BoardSerializer {
    public:
        virtual size_t estimate_buffer_size(Board &board) = 0;
        virtual bool serialize(Board &board, Utils::IOStream &stream) = 0;
        virtual bool deserialize(Board &board, Utils::IOStream &stream) = 0;
    };

    class WorldSerializer {
    public:
        virtual bool serialize(World &world, Utils::IOStream &stream, std::function<void(int)> ticker) = 0;
        virtual bool deserialize(World &world, Utils::IOStream &stream, bool titleOnly, std::function<void(int)> ticker) = 0;
    };

    class SerializerFormatZZT:
        public BoardSerializer,
        public WorldSerializer
    {
    public:
        size_t estimate_buffer_size(Board &board) override;
        bool serialize(Board &board, Utils::IOStream &stream) override;
        bool deserialize(Board &board, Utils::IOStream &stream) override;
        bool serialize(World &world, Utils::IOStream &stream, std::function<void(int)> ticker) override;
        bool deserialize(World &world, Utils::IOStream &stream, bool titleOnly, std::function<void(int)> ticker) override;
    };
};

#endif /* __WORLD_FORMAT_H__ */