#pragma once
#include <chrono>
#include <functional>

namespace loot_gen {

    /*
     *  Р“РµРЅРµСЂР°С‚РѕСЂ С‚СЂРѕС„РµРµРІ
     */
    class LootGenerator {
    public:
        using RandomGenerator = std::function<double()>;
        using TimeInterval = std::chrono::milliseconds;

        /*
         * base_interval - Р±Р°Р·РѕРІС‹Р№ РѕС‚СЂРµР·РѕРє РІСЂРµРјРµРЅРё > 0
         * probability - РІРµСЂРѕСЏС‚РЅРѕСЃС‚СЊ РїРѕСЏРІР»РµРЅРёСЏ С‚СЂРѕС„РµСЏ РІ С‚РµС‡РµРЅРёРµ Р±Р°Р·РѕРІРѕРіРѕ РёРЅС‚РµСЂРІР°Р»Р° РІСЂРµРјРµРЅРё
         * random_generator - РіРµРЅРµСЂР°С‚РѕСЂ РїСЃРµРІРґРѕСЃР»СѓС‡Р°Р№РЅС‹С… С‡РёСЃРµР» РІ РґРёР°РїР°Р·РѕРЅРµ РѕС‚ [0 РґРѕ 1]
         */
        LootGenerator(TimeInterval base_interval, double probability,
            RandomGenerator random_gen = DefaultGenerator)
            : base_interval_{ base_interval }
            , probability_{ probability }
            , random_generator_{ std::move(random_gen) } {
        }


        /*
         * Р’РѕР·РІСЂР°С‰Р°РµС‚ РєРѕР»РёС‡РµСЃС‚РІРѕ С‚СЂРѕС„РµРµРІ, РєРѕС‚РѕСЂС‹Рµ РґРѕР»Р¶РЅС‹ РїРѕСЏРІРёС‚СЊСЃСЏ РЅР° РєР°СЂС‚Рµ СЃРїСѓСЃС‚СЏ
         * Р·Р°РґР°РЅРЅС‹Р№ РїСЂРѕРјРµР¶СѓС‚РѕРє РІСЂРµРјРµРЅРё.
         * РљРѕР»РёС‡РµСЃС‚РІРѕ С‚СЂРѕС„РµРµРІ, РїРѕСЏРІР»СЏСЋС‰РёС…СЃСЏ РЅР° РєР°СЂС‚Рµ РЅРµ РїСЂРµРІС‹С€Р°РµС‚ РєРѕР»РёС‡РµСЃС‚РІРѕ РјР°СЂРѕРґС‘СЂРѕРІ.
         *
         * time_delta - РѕС‚СЂРµР·РѕРє РІСЂРµРјРµРЅРё, РїСЂРѕС€РµРґС€РёР№ СЃ РјРѕРјРµРЅС‚Р° РїСЂРµРґС‹РґСѓС‰РµРіРѕ РІС‹Р·РѕРІР° Generate
         * loot_count - РєРѕР»РёС‡РµСЃС‚РІРѕ С‚СЂРѕС„РµРµРІ РЅР° РєР°СЂС‚Рµ РґРѕ РІС‹Р·РѕРІР° Generate
         * looter_count - РєРѕР»РёС‡РµСЃС‚РІРѕ РјР°СЂРѕРґС‘СЂРѕРІ РЅР° РєР°СЂС‚Рµ
         */
        unsigned Generate(TimeInterval time_delta, unsigned loot_count, unsigned looter_count);

    private:
        static double DefaultGenerator() noexcept {
            return 1.0;
        };
        TimeInterval base_interval_;
        double probability_;
        TimeInterval time_without_loot_{};
        RandomGenerator random_generator_;
    };

}  // namespace loot_gen