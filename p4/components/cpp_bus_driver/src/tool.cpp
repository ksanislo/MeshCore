/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 10:22:46
 * @LastEditTime: 2026-01-21 12:10:27
 * @License: GPL 3.0
 */
#include "tool.h"

namespace Cpp_Bus_Driver
{
    void Tool::assert_log(Log_Level level, const char *file_name, size_t line_number, const char *format, ...)
    {
#if defined CPP_BUS_DRIVER_LOG_LEVEL_BUS || defined CPP_BUS_DRIVER_LOG_LEVEL_CHIP || \
    defined CPP_BUS_DRIVER_LOG_LEVEL_INFO || defined CPP_BUS_DRIVER_LOG_LEVEL_DEBUG

        switch (level)
        {
#if defined CPP_BUS_DRIVER_LOG_LEVEL_DEBUG
        case Log_Level::DEBUG:
        {
            va_list args;
            va_start(args, format);
            auto buffer = std::make_unique<char[]>(MAX_LOG_BUFFER_SIZE);
            snprintf(buffer.get(), MAX_LOG_BUFFER_SIZE, "[cpp_bus_driver][log debug]->[%s][%u line]: %s", file_name, line_number, format);
            vprintf(buffer.get(), args);
            va_end(args);

            break;
        }
#endif
#if defined CPP_BUS_DRIVER_LOG_LEVEL_INFO
        case Log_Level::INFO:
        {
            va_list args;
            va_start(args, format);
            auto buffer = std::make_unique<char[]>(MAX_LOG_BUFFER_SIZE);
            snprintf(buffer.get(), MAX_LOG_BUFFER_SIZE, "[cpp_bus_driver][log info]->[%s][%u line]: %s", file_name, line_number, format);
            vprintf(buffer.get(), args);
            va_end(args);

            break;
        }
#endif
#if defined CPP_BUS_DRIVER_LOG_LEVEL_BUS
        case Log_Level::BUS:
        {
            va_list args;
            va_start(args, format);
            auto buffer = std::make_unique<char[]>(MAX_LOG_BUFFER_SIZE);
            snprintf(buffer.get(), MAX_LOG_BUFFER_SIZE, "[cpp_bus_driver][log bus]->[%s][%u line]: %s", file_name, line_number, format);
            vprintf(buffer.get(), args);
            va_end(args);

            break;
        }
#endif
#if defined CPP_BUS_DRIVER_LOG_LEVEL_CHIP
        case Log_Level::CHIP:
        {
            va_list args;
            va_start(args, format);
            auto buffer = std::make_unique<char[]>(MAX_LOG_BUFFER_SIZE);
            snprintf(buffer.get(), MAX_LOG_BUFFER_SIZE, "[cpp_bus_driver][log chip]->[%s][%u line]: %s", file_name, line_number, format);
            vprintf(buffer.get(), args);
            va_end(args);

            break;
        }
#endif
        default:
            break;
        }

#endif
    }

    bool Tool::search(const uint8_t *search_library, size_t search_library_length, const char *search_sample, size_t sample_length, size_t *search_index)
    {
        // 检查参数有效性
        if (search_sample == nullptr)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(search_sample == nullptr)\n");
            return false;
        }
        else if (search_library == nullptr)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(search_library == nullptr\n");
            return false;
        }
        else if (sample_length == 0)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(sample_length == 0)\n");
            return false;
        }
        else if (search_library_length == 0)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(search_library_length == 0)\n");
            return false;
        }
        else if (sample_length > search_library_length)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(sample_length > search_library_length)\n");
            return false;
        }

        auto buffer = std::search(search_library, search_library + search_library_length, search_sample, search_sample + sample_length);
        // 检查是否找到了数据
        if (buffer == (search_library + search_library_length))
        {
            // assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search fail\n");
            return false;
        }

        if (search_index != nullptr)
        {
            *search_index = buffer - search_library;
        }

        return true;
    }

    bool Tool::search(const char *search_library, size_t search_library_length, const char *search_sample, size_t sample_length, size_t *search_index)
    {
        // 检查参数有效性
        if (search_sample == nullptr)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(search_sample == nullptr)\n");
            return false;
        }
        else if (search_library == nullptr)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(search_library == nullptr\n");
            return false;
        }
        else if (sample_length == 0)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(sample_length == 0)\n");
            return false;
        }
        else if (search_library_length == 0)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(search_library_length == 0)\n");
            return false;
        }
        else if (sample_length > search_library_length)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(sample_length > search_library_length)\n");
            return false;
        }

        auto buffer = std::search(search_library, search_library + search_library_length, search_sample, search_sample + sample_length);
        // 检查是否找到了数据
        if (buffer == (search_library + search_library_length))
        {
            // assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search fail\n");
            return false;
        }

        if (search_index != nullptr)
        {
            *search_index = buffer - search_library;
        }

        return true;
    }

    bool Tool::pin_mode(uint32_t pin, Pin_Mode mode, Pin_Status status)
    {
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        gpio_config_t config;
        config.pin_bit_mask = BIT64(pin);
        switch (mode)
        {
        case Pin_Mode::DISABLE:
            config.mode = GPIO_MODE_INPUT;
            break;
        case Pin_Mode::INPUT:
            config.mode = GPIO_MODE_INPUT;
            break;
        case Pin_Mode::OUTPUT:
            config.mode = GPIO_MODE_OUTPUT;
            break;
        case Pin_Mode::OUTPUT_OD:
            config.mode = GPIO_MODE_OUTPUT_OD;
            break;
        case Pin_Mode::INPUT_OUTPUT_OD:
            config.mode = GPIO_MODE_INPUT_OUTPUT_OD;
            break;
        case Pin_Mode::INPUT_OUTPUT:
            config.mode = GPIO_MODE_INPUT_OUTPUT;
            break;

        default:
            break;
        }
        switch (status)
        {
        case Pin_Status::DISABLE:
            config.pull_up_en = GPIO_PULLUP_DISABLE;
            config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case Pin_Status::PULLUP:
            config.pull_up_en = GPIO_PULLUP_ENABLE;
            config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case Pin_Status::PULLDOWN:
            config.pull_up_en = GPIO_PULLUP_DISABLE;
            config.pull_down_en = GPIO_PULLDOWN_ENABLE;
            break;

        default:
            break;
        }
        config.intr_type = GPIO_INTR_DISABLE;
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
        config.hys_ctrl_mode = GPIO_HYS_SOFT_ENABLE;
#endif

        esp_err_t assert = gpio_config(&config);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_config fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
#elif defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
        switch (mode)
        {
        case Pin_Mode::DISABLE:
            nrf_gpio_cfg_default(pin);
            break;
        case Pin_Mode::INPUT:
            switch (status)
            {
            case Pin_Status::DISABLE:
                pinMode(pin, 0x0);
                break;
            case Pin_Status::PULLUP:
                pinMode(pin, 0x2);
                break;
            case Pin_Status::PULLDOWN:
                pinMode(pin, 0x3);
                break;

            default:
                pinMode(pin, 0x0);
                break;
            }
            break;
        case Pin_Mode::OUTPUT:
            pinMode(pin, 0x1);
            break;

        default:
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_mode fail (error mode: %#X)\n", mode);
            return false;
        }

        return true;
#else
        return false;
#endif
    }

    bool Tool::pin_write(uint32_t pin, bool value)
    {
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        esp_err_t assert = gpio_set_level(static_cast<gpio_num_t>(pin), value);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_config fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
#elif defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
        digitalWrite(pin, value);
        return true;
#else
        return false;
#endif
    }

    bool Tool::pin_read(uint32_t pin)
    {
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        return gpio_get_level(static_cast<gpio_num_t>(pin));
#elif defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
        return digitalRead(pin);
#else
        return false;
#endif
    }

    void Tool::delay_ms(uint32_t value)
    {
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        // 默认状态下vTaskDelay在小于10ms延时的时候不精确
        // vTaskDelay(pdMS_TO_TICKS(value));
        usleep(value * 1000);
#elif defined ARDUINO
        delay(value);
#endif
    }

    void Tool::delay_us(uint32_t value)
    {
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        usleep(value);
#elif defined ARDUINO
        delayMicroseconds(value);
#endif
    }

#if defined DEVELOPMENT_FRAMEWORK_ESPIDF

    int64_t Tool::get_system_time_us(void)
    {
        return esp_timer_get_time();
    }

    int64_t Tool::get_system_time_ms(void)
    {
        return esp_timer_get_time() / 1000;
    }

    bool Tool::create_gpio_interrupt(uint32_t pin, Interrupt_Mode mode, void (*interrupt)(void *), void *args)
    {
        gpio_config_t config;
        config.pin_bit_mask = BIT64(pin);
        config.mode = GPIO_MODE_INPUT;
        switch (mode)
        {
        case Interrupt_Mode::DISABLE:
            config.pull_up_en = GPIO_PULLUP_DISABLE;
            config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            config.intr_type = GPIO_INTR_DISABLE;
            break;
        case Interrupt_Mode::RISING:
            config.pull_up_en = GPIO_PULLUP_DISABLE;
            config.pull_down_en = GPIO_PULLDOWN_ENABLE;
            config.intr_type = GPIO_INTR_POSEDGE;
            break;
        case Interrupt_Mode::FALLING:
            config.pull_up_en = GPIO_PULLUP_ENABLE;
            config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            config.intr_type = GPIO_INTR_NEGEDGE;
            break;
        case Interrupt_Mode::CHANGE:
            config.pull_up_en = GPIO_PULLUP_DISABLE;
            config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            config.intr_type = GPIO_INTR_ANYEDGE;
            break;
        case Interrupt_Mode::ONLOW:
            // 只要 GPIO 引脚保持低电平，就会持续触发中断
            // 需要确保你的中断处理函数能够处理这种情况，或者你的外部信号不会长时间保持低电平
            // 否则系统将崩溃重启
            config.pull_up_en = GPIO_PULLUP_ENABLE;
            config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            config.intr_type = GPIO_INTR_LOW_LEVEL;
            break;
        case Interrupt_Mode::ONHIGH:
            // 只要 GPIO 引脚保持高电平，就会持续触发中断
            // 需要确保你的中断处理函数能够处理这种情况，或者你的外部信号不会长时间保持高电平
            // 否则系统将崩溃重启
            config.pull_up_en = GPIO_PULLUP_DISABLE;
            config.pull_down_en = GPIO_PULLDOWN_ENABLE;
            config.intr_type = GPIO_INTR_HIGH_LEVEL;
            break;

        default:
            break;
        }
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
        config.hys_ctrl_mode = GPIO_HYS_SOFT_ENABLE;
#endif

        esp_err_t assert = gpio_config(&config);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_config fail (error code: %#X)\n", assert);
            return false;
        }

        assert = gpio_install_isr_service(0);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_install_isr_service fail (error code: %#X)\n", assert);
            // return false;
        }

        assert = gpio_isr_handler_add(static_cast<gpio_num_t>(pin), interrupt, args);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_isr_handler_add fail (error code: %#X)\n", assert);
            return false;
        }

        assert = gpio_intr_enable(static_cast<gpio_num_t>(pin));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_intr_enable fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Tool::delete_gpio_interrupt(uint32_t pin)
    {
        esp_err_t assert = gpio_set_intr_type(static_cast<gpio_num_t>(pin), GPIO_INTR_DISABLE);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_set_intr_type fail (error code: %#X)\n", assert);
            return false;
        }

        assert = gpio_isr_handler_remove(static_cast<gpio_num_t>(pin));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_isr_handler_remove fail (error code: %#X)\n", assert);
            return false;
        }

        assert = gpio_intr_disable(static_cast<gpio_num_t>(pin));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_intr_disable fail (error code: %#X)\n", assert);
            return false;
        }

        assert = gpio_reset_pin(static_cast<gpio_num_t>(pin));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_reset_pin fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Tool::create_pwm(int32_t pin, ledc_channel_t channel, uint32_t freq_hz, uint32_t duty, ledc_mode_t speed_mode,
                          ledc_timer_bit_t duty_resolution, ledc_timer_t timer_num, ledc_sleep_mode_t sleep_mode)
    {
        const ledc_timer_config_t buffer_ledc_timer_config =
            {
                .speed_mode = speed_mode,
                .duty_resolution = duty_resolution, // LEDC驱动器占空比精度
                .timer_num = timer_num,             // ledc使用的定时器编号，若需要生成多个频率不同的PWM信号，则需要指定不同的定时器
                .freq_hz = freq_hz,                 // PWM频率
                .clk_cfg = LEDC_AUTO_CLK,           // 自动选择定时器的时钟源
                .deconfigure = false,
            };

        esp_err_t assert = ledc_timer_config(&buffer_ledc_timer_config);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_timer_config fail (error code: %#X)\n", assert);
            return false;
        }

        const ledc_channel_config_t buffer_ledc_channel_config =
            {
                .gpio_num = pin,
                .speed_mode = speed_mode,
                .channel = channel,
                .intr_type = LEDC_INTR_DISABLE,
                .timer_sel = timer_num,
                .duty = duty,
                .hpoint = 0,
                .sleep_mode = sleep_mode,
                .flags =
                    {
                        .output_invert = false,
                    },
            };

        assert = ledc_channel_config(&buffer_ledc_channel_config);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_channel_config fail (error code: %#X)\n", assert);
            return false;
        }

        _pwm.channel = channel;
        _pwm.freq_hz = freq_hz;
        _pwm.duty = duty;
        _pwm.speed_mode = speed_mode;
        _pwm.duty_resolution = duty_resolution;
        _pwm.timer_num = timer_num;
        _pwm.sleep_mode = sleep_mode;

        return true;
    }

    bool Tool::set_pwm_duty(uint8_t duty)
    {
        if (duty > 100)
        {
            duty = 100;
        }

        esp_err_t assert = ledc_set_duty(_pwm.speed_mode, _pwm.channel, (static_cast<float>(duty) / 100.0) * (1 << static_cast<uint8_t>(_pwm.duty_resolution)));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_set_duty fail (error code: %#X)\n", assert);
            return false;
        }

        assert = ledc_update_duty(_pwm.speed_mode, _pwm.channel);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_update_duty fail (error code: %#X)\n", assert);
            return false;
        }

        _pwm.duty = duty;

        return true;
    }

    bool Tool::set_pwm_frequency(uint32_t freq_hz)
    {
        esp_err_t assert = ledc_set_freq(_pwm.speed_mode, _pwm.timer_num, freq_hz);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_set_duty fail (error code: %#X)\n", assert);
            return false;
        }

        _pwm.freq_hz = freq_hz;

        return true;
    }

    bool Tool::start_pwm_gradient_time(uint8_t target_duty, int32_t time_ms)
    {
        if (target_duty > 100)
        {
            target_duty = 100;
        }

        esp_err_t assert = ledc_fade_func_install(false);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_fade_func_install fail (error code: %#X)\n", assert);
            // return false;
        }

        assert = ledc_set_fade_with_time(_pwm.speed_mode, _pwm.channel,
                                         (static_cast<float>(target_duty) / 100.0) * (1 << static_cast<uint8_t>(_pwm.duty_resolution)), time_ms);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_set_fade fail (error code: %#X)\n", assert);
            return false;
        }

        assert = ledc_fade_start(_pwm.speed_mode, _pwm.channel, ledc_fade_mode_t::LEDC_FADE_WAIT_DONE);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_fade_start fail (error code: %#X)\n", assert);
            return false;
        }

        _pwm.duty = target_duty;

        return true;
    }

    bool Tool::stop_pwm(uint32_t idle_level)
    {
        esp_err_t assert = ledc_stop(_pwm.speed_mode, _pwm.channel, idle_level);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_stop fail (error code: %#X)\n", assert);
            return false;
        }

        ledc_fade_func_uninstall();

        return true;
    }
#endif

    /**
     * @brief 安全的字符串转整数
     * @param &input 输入需要转换的字符串
     * @param *output 输出转换后的数据
     * @return
     * @Date 2026-01-21 09:50:30
     */
    bool Tool::safe_stoi(const std::string &input, long *output)
    {
        if (input.empty())
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "safe_stoi input is empty\n");
            return false;
        }

        if (output == nullptr)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "output is nullptr\n");
            return false;
        }

        char *endptr = nullptr;
        // strtol 会尝试转换，并将停止位置存入 endptr
        long val = std::strtol(input.c_str(), &endptr, 10);

        // 检查是否发生了有效转换
        // 如果 endptr 依然指向字符串起始位置，说明输入如 "abc"
        if (endptr == input.c_str())
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "safe_stoi conversion fail (error Input: %s)\n", input.c_str());
            return false;
        }

        *output = val;

        return true;
    }

    /**
     * @brief 安全的字符串转浮点数
     * @param &input 需要转换的字符串
     * @param *output 输出转换后的数据
     * @return
     * @Date 2026-01-21 09:50:30
     */
    bool Tool::safe_stof(const std::string &input, float *output)
    {
        if (input.empty())
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "safe_stof Input is empty\n");
            return false;
        }

        if (output == nullptr)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "output is nullptr\n");
            return false;
        }

        char *endptr = nullptr;
        float val = std::strtof(input.c_str(), &endptr);

        // 如果转换没能开始，或者输入的第一个字符就非法
        if (endptr == input.c_str())
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "safe_stof conversion fail (error Input: %s)\n", input.c_str());
            return false;
        }

        *output = val;

        return true;
    }

    /**
     * @brief 安全的字符串转双精度浮点数
     * @param &input 需要转换的字符串
     * @param *output 输出转换后的数据
     * @return
     * @Date 2026-01-21 11:58:00
     */
    bool Tool::safe_stod(const std::string &input, double *output)
    {
        if (input.empty())
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "safe_stod Input is empty\n");
            return false;
        }

        if (output == nullptr)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "output is nullptr\n");
            return false;
        }

        char *endptr = nullptr;
        // 使用 std::strtod 处理 double 类型
        double val = std::strtod(input.c_str(), &endptr);

        // 如果转换没能开始（例如输入是非数字字符），或者 endptr 指向字符串开头
        if (endptr == input.c_str())
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "safe_stod conversion fail (error Input: %s)\n", input.c_str());
            return false;
        }

        *output = val;

        return true;
    }

    bool Gnss::parse_rmc_info(const uint8_t *data, size_t length, Rmc &rmc)
    {
        assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "parse_rmc_info(length: %d): \n---begin---\n%s\n---end---\n", length, data);

        // 将接收到的数据转为字符串（即使末尾被截断也能处理）
        std::string received_data(reinterpret_cast<const char *>(data), length);
        std::string target = "$GNRMC";
        size_t start_pos = 0;

        bool exit_flag = false;

        // 循环搜索数据中可能存在的所有 $GNRMC 命令
        while ((start_pos = received_data.find(target, start_pos)) != std::string::npos)
        {
            // 寻找当前行的结束符，如果找不到说明被截断了，解析到字符串末尾
            size_t end_pos = received_data.find("\r\n", start_pos);
            std::string line;
            if (end_pos == std::string::npos)
            {
                // 数据被截断
                line = received_data.substr(start_pos);

                assert_log(Log_Level::INFO, __FILE__, __LINE__, "the data has been truncated\n");
            }
            else
            {
                line = received_data.substr(start_pos, end_pos - start_pos);
            }

            start_pos = end_pos + 2;

            // 处理校验和：寻找 '*' 号
            size_t star_pos = line.find('*');
            // 这里暂时不校验逻辑，只按需截取有效内容进行解析
            std::string content = (star_pos != std::string::npos) ? line.substr(0, star_pos) : line;

            std::stringstream ss(content);
            std::string token;
            std::vector<std::string> parts;

            // 按 ',' 分割字符串
            while (std::getline(ss, token, ','))
            {
                parts.push_back(token);
            }

            // 基础安全检查：如果分割出的字段少于 2，跳过（防止只有个头）
            if (parts.size() < 2)
            {
                assert_log(Log_Level::INFO, __FILE__, __LINE__, "insufficient data input\n");
                continue;
            }

            // 开始安全解析各个字段

            // Data 1: UTC Time (hhmmss.ss 或 hhmmss.sss)
            if (parts.size() > 1 && parts[1].length() >= 6)
            {
                long buffer_hour, buffer_minute = 0;
                float buffer_second = 0.0;

                if (safe_stoi(parts[1].substr(0, 2), &buffer_hour) == true)
                {
                    if (safe_stoi(parts[1].substr(2, 2), &buffer_minute) == true)
                    {
                        if (safe_stof(parts[1].substr(4), &buffer_second) == true) // 自动处理 .ss 或 .sss
                        {
                            rmc.utc.hour = buffer_hour;
                            rmc.utc.minute = buffer_minute;
                            rmc.utc.second = buffer_second;

                            rmc.utc.update_flag = true;

                            exit_flag = true;
                        }
                    }
                }
            }

            // Data 2: Status (A=有效, V=无效)
            if (parts.size() > 2 && !parts[2].empty())
            {
                rmc.location_status = parts[2][0];

                exit_flag = true;
            }

            // Data 3 & 4: Lat (ddmm.mmmm) & N/S
            if (parts.size() > 4 && parts[3].length() >= 4)
            {
                long buffer_degrees = 0;
                float buffer_minutes = 0.0;

                if (safe_stoi(parts[3].substr(0, 2), &buffer_degrees) == true)
                {
                    if (safe_stof(parts[3].substr(2), &buffer_minutes) == true)
                    {
                        rmc.location.lat.degrees = buffer_degrees;
                        rmc.location.lat.minutes = buffer_minutes;
                        rmc.location.lat.degrees_minutes = static_cast<double>(rmc.location.lat.degrees) + static_cast<double>(rmc.location.lat.minutes / 60.0);
                        rmc.location.lat.update_flag = true;

                        if (!parts[4].empty())
                        {
                            rmc.location.lat.direction = parts[4][0];
                            rmc.location.lat.direction_update_flag = true;
                        }

                        exit_flag = true;
                    }
                }
            }

            // Data 5 & 6: Lon (dddmm.mmmm) & E/W
            if (parts.size() > 6 && parts[5].length() >= 5)
            {
                long buffer_degrees = 0;
                float buffer_minutes = 0.0;

                if (safe_stoi(parts[5].substr(0, 3), &buffer_degrees) == true)
                {
                    if (safe_stof(parts[5].substr(3), &buffer_minutes) == true)
                    {
                        rmc.location.lon.degrees = buffer_degrees;
                        rmc.location.lon.minutes = buffer_minutes;
                        rmc.location.lon.degrees_minutes = static_cast<double>(rmc.location.lon.degrees) + static_cast<double>(rmc.location.lon.minutes / 60.0);
                        rmc.location.lon.update_flag = true;

                        if (!parts[6].empty())
                        {
                            rmc.location.lon.direction = parts[6][0];
                            rmc.location.lon.direction_update_flag = true;
                        }

                        exit_flag = true;
                    }
                }
            }

            // Data 9: Date (ddmmyy)
            if (parts.size() > 9 && parts[9].length() == 6)
            {
                long buffer_day = 0, buffer_month = 0, buffer_year = 0;

                if (safe_stoi(parts[9].substr(0, 2), &buffer_day) == true)
                {
                    if (safe_stoi(parts[9].substr(2, 2), &buffer_month) == true)
                    {
                        if (safe_stoi(parts[9].substr(4, 2), &buffer_year) == true)
                        {
                            rmc.data.day = buffer_day;
                            rmc.data.month = buffer_month;
                            rmc.data.year = buffer_year;
                            rmc.data.update_flag = true;

                            exit_flag = true;
                        }
                    }
                }
            }

            if (exit_flag == true)
            {
                assert_log(Log_Level::INFO, __FILE__, __LINE__, "parse_rmc_info finish (success parse size: %d)\n", parts.size());
                break;
            }
        }

        return true;
    }

    bool Gnss::parse_gga_info(const uint8_t *data, size_t length, Gga &gga)
    {
        assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "parse_gga_info(length: %d): \n---begin---\n%s\n---end---\n", length, data);

        std::string received_data(reinterpret_cast<const char *>(data), length);
        std::string target = "$GNGGA";
        size_t start_pos = 0;

        bool exit_flag = false;

        while ((start_pos = received_data.find(target, start_pos)) != std::string::npos)
        {
            size_t end_pos = received_data.find("\r\n", start_pos);
            std::string line;
            if (end_pos == std::string::npos)
            {
                // 数据被截断
                line = received_data.substr(start_pos);

                assert_log(Log_Level::INFO, __FILE__, __LINE__, "the data has been truncated\n");
            }
            else
            {
                line = received_data.substr(start_pos, end_pos - start_pos);
            }

            start_pos = end_pos + 2;

            size_t star_pos = line.find('*');
            std::string content = (star_pos != std::string::npos) ? line.substr(0, star_pos) : line;

            std::stringstream ss(content);
            std::string token;
            std::vector<std::string> parts;
            while (std::getline(ss, token, ','))
            {
                parts.push_back(token);
            }

            if (parts.size() < 2)
            {
                assert_log(Log_Level::INFO, __FILE__, __LINE__, "insufficient data input\n");
                continue;
            }

            // Data 1: UTC Time (hhmmss.ss 或 hhmmss.sss)
            if (parts.size() > 1 && parts[1].length() >= 6)
            {
                long buffer_hour = 0, buffer_minute = 0;
                float buffer_second = 0.0;

                if (safe_stoi(parts[1].substr(0, 2), &buffer_hour) == true)
                {
                    if (safe_stoi(parts[1].substr(2, 2), &buffer_minute) == true)
                    {
                        if (safe_stof(parts[1].substr(4), &buffer_second) == true)
                        {
                            gga.utc.hour = buffer_hour;
                            gga.utc.minute = buffer_minute;
                            gga.utc.second = buffer_second;
                            gga.utc.update_flag = true;

                            exit_flag = true;
                        }
                    }
                }
            }

            // Data 2 & 3: Lat (dddmm.mmmm) & N/S
            if (parts.size() > 3 && parts[2].length() >= 4)
            {
                long buffer_degrees = 0;
                float buffer_minutes = 0.0;

                if (safe_stoi(parts[2].substr(0, 2), &buffer_degrees) == true)
                {
                    if (safe_stof(parts[2].substr(2), &buffer_minutes) == true)
                    {
                        gga.location.lat.degrees = buffer_degrees;
                        gga.location.lat.minutes = buffer_minutes;
                        gga.location.lat.degrees_minutes = static_cast<double>(gga.location.lat.degrees) + static_cast<double>(gga.location.lat.minutes / 60.0);
                        gga.location.lat.update_flag = true;

                        if (!parts[3].empty())
                        {
                            gga.location.lat.direction = parts[3][0];
                            gga.location.lat.direction_update_flag = true;
                        }

                        exit_flag = true;
                    }
                }
            }

            // Data 4 & 5: Lon (dddmm.mmmm) & E/W
            if (parts.size() > 5 && parts[4].length() >= 5)
            {
                long buffer_degrees = 0;
                float buffer_minutes = 0.0;

                if (safe_stoi(parts[4].substr(0, 3), &buffer_degrees) == true)
                {
                    if (safe_stof(parts[4].substr(3), &buffer_minutes) == true)
                    {
                        gga.location.lon.degrees = buffer_degrees;
                        gga.location.lon.minutes = buffer_minutes;
                        gga.location.lon.degrees_minutes = static_cast<double>(gga.location.lon.degrees) + static_cast<double>(gga.location.lon.minutes / 60.0);
                        gga.location.lon.update_flag = true;

                        if (!parts[5].empty())
                        {
                            gga.location.lon.direction = parts[5][0];
                            gga.location.lon.direction_update_flag = true;
                        }

                        exit_flag = true;
                    }
                }
            }

            // Data 6: Quality
            if (parts.size() > 6)
            {
                long buffer_gps_mode = 0;

                if (safe_stoi(parts[6], &buffer_gps_mode) == true)
                {
                    gga.gps_mode_status = buffer_gps_mode;
                    exit_flag = true;
                }
            }

            // Data 7: NumSatUsed
            if (parts.size() > 7)
            {
                long buffer_satellite_count = 0;

                if (safe_stoi(parts[7], &buffer_satellite_count) == true)
                {
                    gga.online_satellite_count = buffer_satellite_count;
                    exit_flag = true;
                }
            }

            // Data 8: HDOP
            if (parts.size() > 8)
            {
                float buffer_hdop = 0.0;

                if (safe_stof(parts[8], &buffer_hdop) == true)
                {
                    gga.hdop = buffer_hdop;
                    exit_flag = true;
                }
            }

            if (exit_flag == true)
            {
                assert_log(Log_Level::INFO, __FILE__, __LINE__, "parse_gga_info finish (success parse size: %d)\n", parts.size());
                break;
            }
        }

        return true;
    }
}