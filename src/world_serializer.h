#ifndef __WORLD_SERIALIZER_H__
#define __WORLD_SERIALIZER_H__

#include <cstddef>
#include <cstdint>
#include <functional>
#include "utils/iostream.h"

namespace ZZT {
    struct Board;
    struct World;

    typedef enum: uint8_t {
        WorldFormatAny,
        WorldFormatZZT,
        WorldFormatSuperZZT
    } WorldFormat;

    class BoardSerializer {
    public:
        virtual size_t estimate_board_size(Board &board) = 0;
        virtual bool serialize_board(Board &board, IOStream &stream, bool internal) = 0;
        virtual bool deserialize_board(Board &board, IOStream &stream, bool internal) = 0;
    };

    class WorldSerializer {
    public:
        virtual bool serialize_world(World &world, IOStream &stream, std::function<void(int)> ticker) = 0;
        virtual bool deserialize_world(World &world, IOStream &stream, bool titleOnly, std::function<void(int)> ticker) = 0;
    };

    class Serializer :
        virtual public BoardSerializer,
        virtual public WorldSerializer
    {
    protected:
        WorldFormat format;
    public:
        inline WorldFormat get_format() const {
            return format;
        }
    };

    class SerializerFormatZZT:
        virtual public Serializer
    {
    public:
        SerializerFormatZZT(WorldFormat format);

        size_t estimate_board_size(Board &board) override;
        bool serialize_board(Board &board, IOStream &stream, bool internal) override;
        bool deserialize_board(Board &board, IOStream &stream, bool internal) override;
        bool serialize_world(World &world, IOStream &stream, std::function<void(int)> ticker) override;
        bool deserialize_world(World &world, IOStream &stream, bool titleOnly, std::function<void(int)> ticker) override;
    };
};

#endif /* __WORLD_FORMAT_H__ */