/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-08-05 11:44:23
 * @LastEditTime: 2025-09-08 18:14:46
 * @License: GPL 3.0
 */
#include "arduino_cpp_bus_driver_library.h"

struct Interrupt_Arg
{
    std::function<void(void)> interrupt_function;
};

static std::unordered_map<uint8_t, std::unique_ptr<Interrupt_Arg>> Interrupt_Map;

auto Arduino_Cpp_Bus_Driver_Tool = std::make_unique<Cpp_Bus_Driver::Tool>();

static void IRAM_ATTR Interrupt_Callback_Template(void *arg)
{
    auto *local_arg = static_cast<Interrupt_Arg *>(arg);
    if (local_arg->interrupt_function)
    {
        local_arg->interrupt_function();
    }
}

void delay(uint32_t ms)
{
    Arduino_Cpp_Bus_Driver_Tool->delay_ms(ms);
}

void delayMicroseconds(uint32_t us)
{
    Arduino_Cpp_Bus_Driver_Tool->delay_us(us);
}

int64_t millis(void)
{
    return Arduino_Cpp_Bus_Driver_Tool->get_system_time_ms();
}

void pinMode(uint8_t pin, uint8_t mode)
{
    if (pin == static_cast<uint8_t>(-1))
    {
        Arduino_Cpp_Bus_Driver_Tool->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "pinMode fail (pin == -1)\n");
        return;
    }

    switch (mode)
    {
    case INPUT:
        Arduino_Cpp_Bus_Driver_Tool->pin_mode(pin, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT);
        break;
    case OUTPUT:
        Arduino_Cpp_Bus_Driver_Tool->pin_mode(pin, Cpp_Bus_Driver::Tool::Pin_Mode::OUTPUT);
        break;
    case PULLUP:
        Arduino_Cpp_Bus_Driver_Tool->pin_mode(pin, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT_OUTPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);
        break;
    case INPUT_PULLUP:
        Arduino_Cpp_Bus_Driver_Tool->pin_mode(pin, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);
        break;
    case PULLDOWN:
        Arduino_Cpp_Bus_Driver_Tool->pin_mode(pin, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT_OUTPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);
        break;
    case INPUT_PULLDOWN:
        Arduino_Cpp_Bus_Driver_Tool->pin_mode(pin, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);
        break;
    case OPEN_DRAIN:
        Arduino_Cpp_Bus_Driver_Tool->pin_mode(pin, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT_OUTPUT_OD);
        break;
    case OUTPUT_OPEN_DRAIN:
        Arduino_Cpp_Bus_Driver_Tool->pin_mode(pin, Cpp_Bus_Driver::Tool::Pin_Mode::OUTPUT_OD);
        break;
    case ANALOG:
        Arduino_Cpp_Bus_Driver_Tool->pin_mode(pin, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE);
        break;

    default:
        Arduino_Cpp_Bus_Driver_Tool->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "set pinMode fail (unknown mode: %d)\n", mode);
        break;
    }
}

void digitalWrite(uint8_t pin, uint8_t val)
{
    if (pin == static_cast<uint8_t>(-1))
    {
        // Arduino_Cpp_Bus_Driver_Tool->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "digitalWrite fail (pin == -1)\n");
        return;
    }

    Arduino_Cpp_Bus_Driver_Tool->pin_write(pin, val);
}

int digitalRead(uint8_t pin)
{
    if (pin == static_cast<uint8_t>(-1))
    {
        // Arduino_Cpp_Bus_Driver_Tool->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "digitalRead fail (pin == -1)\n");
        return 0;
    }

    return Arduino_Cpp_Bus_Driver_Tool->pin_read(pin);
}

void attachInterrupt(uint8_t pin, std::function<void(void)> intRoutine, int mode)
{
    if (pin == static_cast<uint8_t>(-1))
    {
        Arduino_Cpp_Bus_Driver_Tool->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "attachInterrupt fail (pin == -1)\n");
        return;
    }

    Cpp_Bus_Driver::Tool::Interrupt_Mode buffer_mode = Cpp_Bus_Driver::Tool::Interrupt_Mode::DISABLE;
    switch (mode)
    {
    case DISABLED:
        break;
    case RISING:
        buffer_mode = Cpp_Bus_Driver::Tool::Interrupt_Mode::RISING;
        break;
    case FALLING:
        buffer_mode = Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING;
        break;
    case CHANGE:
        buffer_mode = Cpp_Bus_Driver::Tool::Interrupt_Mode::CHANGE;
        break;
    case ONLOW:
        buffer_mode = Cpp_Bus_Driver::Tool::Interrupt_Mode::ONLOW;
        break;
    case ONHIGH:
        buffer_mode = Cpp_Bus_Driver::Tool::Interrupt_Mode::ONHIGH;
        break;
    case ONLOW_WE:
        buffer_mode = Cpp_Bus_Driver::Tool::Interrupt_Mode::ONLOW;
        break;
    case ONHIGH_WE:
        buffer_mode = Cpp_Bus_Driver::Tool::Interrupt_Mode::ONHIGH;
        break;

    default:
        Arduino_Cpp_Bus_Driver_Tool->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "set attachInterrupt mode fail (unknown mode: %d)\n", mode);
        break;
    }

    auto arg = std::make_unique<Interrupt_Arg>(intRoutine);
    Interrupt_Map[pin] = std::move(arg);

    if (Arduino_Cpp_Bus_Driver_Tool->create_gpio_interrupt(pin, buffer_mode, Interrupt_Callback_Template, Interrupt_Map[pin].get()) == false)
    {
        Arduino_Cpp_Bus_Driver_Tool->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "create_gpio_interrupt fail\n");
    }
}

void detachInterrupt(uint8_t pin)
{
    if (pin == static_cast<uint8_t>(-1))
    {
        Arduino_Cpp_Bus_Driver_Tool->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "detachInterrupt fail (pin == -1)\n");
        return;
    }

    if (Arduino_Cpp_Bus_Driver_Tool->delete_gpio_interrupt(pin) == false)
    {
        Arduino_Cpp_Bus_Driver_Tool->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "delete_gpio_interrupt fail\n");
    }
}
