#ifndef __WORLD_SERIALIZER_H__
#define __WORLD_SERIALIZER_H__

#include <cstddef>
#include <cstdint>
#include <functional>
#include "utils.h"

namespace ZZT {
    struct Board;
    struct World;

    typedef enum {
        WorldFormatAny,
        WorldFormatZZT,
        WorldFormatSuperZZT, // TODO
        WorldFormatInternal  
    } WorldFormat;

    class BoardSerializer {
    public:
        virtual size_t estimate_board_size(Board &board) = 0;
        virtual bool serialize_board(Board &board, Utils::IOStream &stream) = 0;
        virtual bool deserialize_board(Board &board, Utils::IOStream &stream) = 0;
    };

    class WorldSerializer {
    public:
        virtual bool serialize_world(World &world, Utils::IOStream &stream, std::function<void(int)> ticker) = 0;
        virtual bool deserialize_world(World &world, Utils::IOStream &stream, bool titleOnly, std::function<void(int)> ticker) = 0;
    };

    class Serializer :
        virtual public BoardSerializer,
        virtual public WorldSerializer
    {
    protected:
        WorldFormat format;
    public:
        inline WorldFormat get_format() {
            return format;
        }
    };

    class SerializerFormatZZT:
        virtual public Serializer
    {
    public:
        SerializerFormatZZT(WorldFormat format);

        size_t estimate_board_size(Board &board) override;
        bool serialize_board(Board &board, Utils::IOStream &stream) override;
        bool deserialize_board(Board &board, Utils::IOStream &stream) override;
        bool serialize_world(World &world, Utils::IOStream &stream, std::function<void(int)> ticker) override;
        bool deserialize_world(World &world, Utils::IOStream &stream, bool titleOnly, std::function<void(int)> ticker) override;
    };
};

#endif /* __WORLD_FORMAT_H__ */