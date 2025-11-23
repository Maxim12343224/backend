==== = collision_detector.h ==== =
#pragma once

#include "geom.h"

#include <algorithm>
#include <vector>

namespace collision_detector {

    struct CollectionResult {
        bool IsCollected(double collect_radius) const {
            return proj_ratio >= 0 && proj_ratio <= 1 && sq_distance <= collect_radius * collect_radius;
        }

        // РєРІР°РґСЂР°С‚ СЂР°СЃСЃС‚РѕСЏРЅРёСЏ РґРѕ С‚РѕС‡РєРё
        double sq_distance;

        // РґРѕР»СЏ РїСЂРѕР№РґРµРЅРЅРѕРіРѕ РѕС‚СЂРµР·РєР°
        double proj_ratio;
    };

    // Р”РІРёР¶РµРјСЃСЏ РёР· С‚РѕС‡РєРё a РІ С‚РѕС‡РєСѓ b Рё РїС‹С‚Р°РµРјСЃСЏ РїРѕРґРѕР±СЂР°С‚СЊ С‚РѕС‡РєСѓ c.
    // Р­С‚Р° С„СѓРЅРєС†РёСЏ СЂРµР°Р»РёР·РѕРІР°РЅР° РІ СѓСЂРѕРєРµ.
    CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c);

    struct Item {
        geom::Point2D position;
        double width;
    };

    struct Gatherer {
        geom::Point2D start_pos;
        geom::Point2D end_pos;
        double width;
    };

    class ItemGathererProvider {
    protected:
        ~ItemGathererProvider() = default;

    public:
        virtual size_t ItemsCount() const = 0;
        virtual Item GetItem(size_t idx) const = 0;
        virtual size_t GatherersCount() const = 0;
        virtual Gatherer GetGatherer(size_t idx) const = 0;
    };

    struct GatheringEvent {
        size_t item_id;
        size_t gatherer_id;
        double sq_distance;
        double time;
    };

    // Р­С‚Сѓ С„СѓРЅРєС†РёСЋ РІР°Рј РЅСѓР¶РЅРѕ Р±СѓРґРµС‚ СЂРµР°Р»РёР·РѕРІР°С‚СЊ РІ СЃРѕРѕС‚РІРµС‚СЃС‚РІСѓСЋС‰РµРј Р·Р°РґР°РЅРёРё.
    // РџСЂРё РїСЂРѕРІРµСЂРєРµ РІР°С€РёС… С‚РµСЃС‚РѕРІ РѕРЅР° РЅРµ РЅСѓР¶РЅР° - С„СѓРЅРєС†РёСЏ Р±СѓРґРµС‚ Р»РёРЅРєРѕРІР°С‚СЊСЃСЏ СЃРЅР°СЂСѓР¶Рё.
    std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider);

}  // namespace collision_detector