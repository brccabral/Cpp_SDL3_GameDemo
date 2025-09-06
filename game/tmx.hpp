#pragma once
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace tmx
{
    struct Layer
    {
        int id{};
        std::string name{};
        std::vector<int> data{}; // CSV
    };

    struct LayerObject
    {
        int id{};
        std::string name{};
        std::string type{};
        float x{};
        float y{};
    };

    struct ObjectGroup
    {
        int id{};
        std::string name{};
        std::vector<LayerObject> objects{};
    };

    struct Image
    {
        std::string source{};
        int width{}, height{};
    };

    struct Tile
    {
        int id{};
        Image image{};
    };

    struct TileSet
    {
        int count{}, tileWidth{}, tileHeight{}, columns{}, firstgid{};
        std::vector<Tile> tiles{};

        TileSet(
                const int firstgid, const int count, const int tileWidth, const int tileHeight,
                const int columns)
            : count(count), tileWidth(tileWidth), tileHeight(tileHeight), columns(columns),
              firstgid(firstgid)
        {
        }
    };

    struct Map
    {
        int mapWidth{}, mapHeight{};
        int tileWidth{}, tileHeight{};
        std::vector<TileSet> tileSets{};
        std::vector<std::variant<Layer, ObjectGroup>> layers{};
    };

    std::unique_ptr<Map> loadMap(const std::string& filename);
}
