/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 17:58:03
 * @LastEditTime: 2026-01-21 12:03:59
 * @License: GPL 3.0
 */

#pragma once

#include "config.h"

namespace Cpp_Bus_Driver
{
    class Tool
    {
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
    private:
        struct Pwm
        {
            ledc_channel_t channel = ledc_channel_t::LEDC_CHANNEL_0;
            uint32_t freq_hz = 2000;
            uint32_t duty = 0;
            ledc_mode_t speed_mode = ledc_mode_t::LEDC_LOW_SPEED_MODE;
            ledc_timer_bit_t duty_resolution = ledc_timer_bit_t::LEDC_TIMER_10_BIT;
            ledc_timer_t timer_num = ledc_timer_t::LEDC_TIMER_0;
            ledc_sleep_mode_t sleep_mode = ledc_sleep_mode_t::LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
        };
#endif

    private:
        static constexpr uint16_t MAX_LOG_BUFFER_SIZE = 1024;

    protected:
        enum class Init_List_Cmd
        {
            DELAY_MS,
            WRITE_DATA,
            WRITE_C8_D8,
            WRITE_C8_R24,
            WRITE_C8_R24_D8,
            WRITE_C16_D8,
        };

        enum class Endian
        {
            BIG,
            LITTLE,
        };

    public:
        enum class Log_Level
        {
            DEBUG, // debug信息
            INFO,  // 普通信息

            BUS,  // 总线错误
            CHIP, // 芯片错误
        };

        enum class Interrupt_Mode
        {
            DISABLE,
            RISING,
            FALLING,
            CHANGE,
            ONLOW,
            ONHIGH,
        };

        enum class Pin_Mode
        {
            DISABLE,
            INPUT,           // input only
            OUTPUT,          // output only mode
            OUTPUT_OD,       // output only with open-drain mode
            INPUT_OUTPUT_OD, // output and input with open-drain mode
            INPUT_OUTPUT,    // output and input mode
        };

        enum class Pin_Status
        {
            DISABLE,
            PULLUP,
            PULLDOWN,
        };

#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        Pwm _pwm;
#endif

        Tool()
        {
        }

        void assert_log(Log_Level level, const char *file_name, size_t line_number, const char *format, ...);

        /**
         * @brief 搜索函数
         * @param *search_library 需要使用的搜索库
         * @param search_library_length 需要使用的搜索库长度
         * @param *search_sample 需要搜索的样本
         * @param sample_length 需要搜索的样本长度
         * @param *search_index 搜索成功的的样本中第一个搜索成功字符位置的引索
         * @return
         * @Date 2025-03-26 16:41:35
         */
        bool search(const uint8_t *search_library, size_t search_library_length, const char *search_sample, size_t sample_length, size_t *search_index = nullptr);
        bool search(const char *search_library, size_t search_library_length, const char *search_sample, size_t sample_length, size_t *search_index = nullptr);

        bool safe_stoi(const std::string &input, long *output);
        bool safe_stof(const std::string &input, float *output);
        bool safe_stod(const std::string &input, double *output);

        bool pin_mode(uint32_t pin, Pin_Mode mode, Pin_Status status = Pin_Status::DISABLE);
        bool pin_write(uint32_t pin, bool value);
        bool pin_read(uint32_t pin);

        void delay_ms(uint32_t value);
        void delay_us(uint32_t value);

#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        int64_t get_system_time_us(void);
        int64_t get_system_time_ms(void);

        bool create_gpio_interrupt(uint32_t pin, Interrupt_Mode mode, void (*interrupt)(void *arg), void *args = nullptr);
        bool delete_gpio_interrupt(uint32_t pin);

        bool create_pwm(int32_t pin, ledc_channel_t channel, uint32_t freq_hz, uint32_t duty = 0, ledc_mode_t speed_mode = ledc_mode_t::LEDC_LOW_SPEED_MODE,
                        ledc_timer_bit_t duty_resolution = ledc_timer_bit_t::LEDC_TIMER_10_BIT, ledc_timer_t timer_num = ledc_timer_t::LEDC_TIMER_0,
                        ledc_sleep_mode_t sleep_mode = ledc_sleep_mode_t::LEDC_SLEEP_MODE_NO_ALIVE_NO_PD);

        /**
         * @brief 设置占空比
         * @param duty 值范围0~100
         * @return
         * @Date 2025-03-19 10:02:18
         */
        bool set_pwm_duty(uint8_t duty);
        bool set_pwm_frequency(uint32_t freq_hz);

        /**
         * @brief 设置渐变占空比
         * @param target_duty 值范围0~100
         * @param time_ms 渐变花费时间
         * @return
         * @Date 2025-03-19 10:03:07
         */
        bool start_pwm_gradient_time(uint8_t target_duty, int32_t time_ms);

        bool stop_pwm(uint32_t idle_level = 0);

#endif
    };

    class Gnss : public Tool
    {
    public:
        struct Rmc
        {
            struct
            {
                uint8_t hour = -1;   // 小时
                uint8_t minute = -1; // 分钟
                float second = -1;   // 秒

                bool update_flag = false;
            } utc;

            // 定位系统状态。
            // A = 数据有效
            // V = 无效
            // D = 差分
            std::string location_status;

            struct
            {
                struct
                {
                    uint8_t degrees = -1;        // 度
                    float minutes = -1;         // 分
                    double degrees_minutes = -1; // 带小数的度分转度

                    // 纬度方向
                    // N = 北
                    // S = 南
                    std::string direction;

                    bool update_flag = false;           // 度分更新标志
                    bool direction_update_flag = false; // 方向更新标志
                } lat;

                struct
                {
                    uint8_t degrees = -1;
                    float minutes = -1;
                    double degrees_minutes = -1;

                    // 经度方向
                    // E = 东
                    // W = 西
                    std::string direction;

                    bool update_flag = false;
                    bool direction_update_flag = false;
                } lon;

            } location;

            struct
            {
                uint8_t day = -1;   // 日
                uint8_t month = -1; // 月
                uint8_t year = -1;  // 年

                bool update_flag = false;
            } data;
        };

        struct Gga
        {
            struct
            {
                uint8_t hour = -1;   // 小时
                uint8_t minute = -1; // 分钟
                float second = -1;   // 秒

                bool update_flag = false;
            } utc;

            struct
            {
                struct
                {
                    uint8_t degrees = -1;       // 度
                    float minutes = -1;         // 分
                    double degrees_minutes = -1; // 带小数的度分转度

                    // 纬度方向
                    // N = 北
                    // S = 南
                    std::string direction;

                    bool update_flag = false;           // 度分更新标志
                    bool direction_update_flag = false; // 方向更新标志
                } lat;

                struct
                {
                    uint8_t degrees = -1;
                    float minutes = -1;
                    double degrees_minutes = -1;

                    // 经度方向
                    // E = 东
                    // W = 西
                    std::string direction;

                    bool update_flag = false;
                    bool direction_update_flag = false;
                } lon;

            } location;

            // GPS 定位模式/状态指示
            // 0 = 定位不可用或无效
            // 1 = GPS SPS 模式，定位有效
            // 2 = 差分 GPS、SPS 模式或 SBAS定位有效
            // 6 = 估算（航位推算）模式
            uint8_t gps_mode_status = -1;
            uint8_t online_satellite_count = -1; // 在线的卫星数量

            float hdop = -1;
        };

        Gnss()
        {
        }

        /**
         * @brief 解析rmc信息
         * @param data 要解析的数据
         * @param length 要解析的数据长度
         * @param &rmc 返回的解析结构体
         * @return
         * @Date 2025-02-18 11:54:34
         */
        bool parse_rmc_info(const uint8_t *data, size_t length, Rmc &rmc);

        /**
         * @brief 解析gga信息
         * @param data 要解析的数据
         * @param length 要解析的数据长度
         * @param &gga 返回的解析结构体
         * @return
         * @Date 2025-02-18 11:54:34
         */
        bool parse_gga_info(const uint8_t *data, size_t length, Gga &gga);
    };

}