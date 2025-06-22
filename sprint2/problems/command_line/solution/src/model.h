#pragma once
#include "tagged.h"
#include <string>
#include <vector>

namespace model {

    using Dimension = int;
    using Coord = Dimension;

    struct Point {
        Coord x, y;
    };

    struct Size {
        Dimension width, height;
    };

    struct Rectangle {
        Point position;
        Size size;
    };

    struct Offset {
        Dimension dx, dy;
    };

    class Road {
    public:
        struct HorizontalTag {};
        struct VerticalTag {};

        constexpr static HorizontalTag HORIZONTAL{};
        constexpr static VerticalTag VERTICAL{};

        Road(HorizontalTag, Point start, Coord end_x) noexcept;
        Road(VerticalTag, Point start, Coord end_y) noexcept;

        bool IsHorizontal() const noexcept;
        bool IsVertical() const noexcept;
        Point GetStart() const noexcept;
        Point GetEnd() const noexcept;

    private:
        Point start_;
        Point end_;
    };

    class Building {
    public:
        explicit Building(Rectangle bounds) noexcept;
        const Rectangle& GetBounds() const noexcept;

    private:
        Rectangle bounds_;
    };

    class Office {
    public:
        using Id = util::Tagged<std::string, Office>;

        Office(Id id, Point position, Offset offset) noexcept;
        const Id& GetId() const noexcept;
        Point GetPosition() const noexcept;
        Offset GetOffset() const noexcept;

    private:
        Id id_;
        Point position_;
        Offset offset_;
    };

} // namespace model