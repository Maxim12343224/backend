#pragma once
#include <cassert>
#include <optional>
#include <stdexcept>

class TV {
public:
    constexpr static int MIN_CHANNEL = 1;
    constexpr static int MAX_CHANNEL = 99;

    /*
     * Возвращает информацию о том, включен телевизор или нет.
     */
    [[nodiscard]] bool IsTurnedOn() const noexcept {
        return is_turned_on_;
    }

    /*
     * Возвращает номер выбранного канала или std::nullopt, если телевизор выключен.
     */
    [[nodiscard]] std::optional<int> GetChannel() const noexcept {
        return is_turned_on_ ? std::optional{ channel_ } : std::nullopt;
    }

    /*
     * Включает телевизор, если он был выключен. Если телевизор уже включен, ничего не делает.
     * При включении выбирается тот номер канала, который был выбран перед последним выключением.
     * При самом первом включении выбирает канал #1.
     */
    void TurnOn() noexcept {
        if (!is_turned_on_) {
            is_turned_on_ = true;
            // Восстанавливаем последний канал или используем канал по умолчанию
            channel_ = last_channel_.has_value() ? last_channel_.value() : 1;
        }
    }

    /*
     * Выключает телевизор, если он был включен. Если телевизор уже выключен, ничего не делает.
     */
    void TurnOff() noexcept {
        if (is_turned_on_) {
            is_turned_on_ = false;
            // Сохраняем текущий канал как последний выбранный
            last_channel_ = channel_;
        }
    }

    /*
     * Выбирает канал channel.
     * Ранее выбранный канал запоминается и может быть восстановлен методом SelectLastViewedChannel.
     * Если номер канала совпадает с ранее выбранным каналом, метод ничего не делает.
     * Если телевизор выключен, выбрасывает исключение std::logic_error.
     * Если номер канала за пределами диапазона MIN_CHANNEL, MAX_CHANNEL, выбрасывает out_of_range.
     */
    void SelectChannel(int channel) {
        if (!is_turned_on_) {
            throw std::logic_error("TV is turned off");
        }

        if (channel < MIN_CHANNEL || channel > MAX_CHANNEL) {
            throw std::out_of_range("Channel is out of range");
        }

        if (channel != channel_) {
            // Сохраняем предыдущий канал для функции SelectLastViewedChannel
            previous_channel_ = channel_;
            channel_ = channel;
        }
    }

    /*
     * Выбирает номер канала, который был выбран перед последним вызовом SelectChannel.
     * Многократный последовательный вызов SelectLastViewedChannel переключает два последних выбранных канала.
     * Если телевизор выключен, выбрасывает исключение std::logic_error.
     */
    void SelectLastViewedChannel() {
        if (!is_turned_on_) {
            throw std::logic_error("TV is turned off");
        }

        if (previous_channel_.has_value()) {
            std::swap(channel_, previous_channel_.value());
        }
    }

private:
    bool is_turned_on_ = false;
    int channel_ = 1;
    std::optional<int> last_channel_;      // Последний канал перед выключением
    std::optional<int> previous_channel_;  // Предыдущий канал для переключения
};