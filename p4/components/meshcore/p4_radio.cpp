/*
 * p4_radio.cpp — SX1262 radio glue for the T-Display P4.
 *
 * Owns the P4SX1262Radio (mesh::Radio) singleton and the LoRa param helpers.
 * The SX1262/XL9535 chip objects themselves are DEFINED in main.cpp (external
 * linkage); we reference them here. Adapted from Meck-P4's target.cpp (GPL).
 */
#include "p4_radio.h"
#include "P4SX1262Radio.h"          // pulls in the extern SX1262 + cpp_bus_driver
#include "t_display_p4_config.h"
#include "esp_random.h"
#include <stdio.h>

// ---- MeshCore radio adapter instance ----
static P4SX1262Radio radio_driver;

mesh::Radio& p4_get_radio() { return radio_driver; }

// XL9535 I2C GPIO expander, defined in main.cpp. Drives the SKY13453 RF switch.
extern std::unique_ptr<Cpp_Bus_Driver::Xl95x5> XL9535;

extern "C" void meck_set_antenna(uint8_t external) {
    if (!XL9535) return;
    XL9535->pin_write(XL9535_SKY13453_VCTL,
                      external ? Cpp_Bus_Driver::Xl95x5::Value::LOW
                               : Cpp_Bus_Driver::Xl95x5::Value::HIGH);
}

extern "C" void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr) {
    Cpp_Bus_Driver::Sx126x::Lora_Bw bw_enum;
    if (bw >= 500.0f)       bw_enum = Cpp_Bus_Driver::Sx126x::Lora_Bw::BW_500000HZ;
    else if (bw >= 250.0f)  bw_enum = Cpp_Bus_Driver::Sx126x::Lora_Bw::BW_250000HZ;
    else if (bw >= 125.0f)  bw_enum = Cpp_Bus_Driver::Sx126x::Lora_Bw::BW_125000HZ;
    else if (bw >= 62.5f)   bw_enum = Cpp_Bus_Driver::Sx126x::Lora_Bw::BW_62500HZ;
    else if (bw >= 41.67f)  bw_enum = Cpp_Bus_Driver::Sx126x::Lora_Bw::BW_41670HZ;
    else if (bw >= 31.25f)  bw_enum = Cpp_Bus_Driver::Sx126x::Lora_Bw::BW_31250HZ;
    else                    bw_enum = Cpp_Bus_Driver::Sx126x::Lora_Bw::BW_15630HZ;

    Cpp_Bus_Driver::Sx126x::Sf sf_enum = (Cpp_Bus_Driver::Sx126x::Sf)sf;

    Cpp_Bus_Driver::Sx126x::Cr cr_enum;
    switch (cr) {
        case 5:  cr_enum = Cpp_Bus_Driver::Sx126x::Cr::CR_4_5; break;
        case 6:  cr_enum = Cpp_Bus_Driver::Sx126x::Cr::CR_4_6; break;
        case 7:  cr_enum = Cpp_Bus_Driver::Sx126x::Cr::CR_4_7; break;
        case 8:  cr_enum = Cpp_Bus_Driver::Sx126x::Cr::CR_4_8; break;
        default: cr_enum = Cpp_Bus_Driver::Sx126x::Cr::CR_4_5; break;
    }

    uint16_t preamble = (sf <= 8) ? 32 : 16;
    uint16_t meshcore_sync_word = 0x1424;  // MeshCore

    if (SX1262) {
        SX1262->config_lora_params(
            freq, bw_enum, 140 /*current limit mA*/, LORA_TX_POWER_DEFAULT,
            sf_enum, cr_enum,
            Cpp_Bus_Driver::Sx126x::Lora_Crc_Type::ON,
            preamble, meshcore_sync_word
        );
    }

    radio_driver.setParams(freq, bw, sf, cr);

    printf("radio_set_params() - freq=%.3f bw=%.1f sf=%u cr=4/%u sync=0x%04X\n",
           (double)freq, (double)bw, (unsigned)sf, (unsigned)cr,
           (unsigned)meshcore_sync_word);
}

extern "C" void radio_set_tx_power(uint8_t dbm) {
    if (dbm > 22) dbm = 22;
    printf("radio_set_tx_power() - %u dBm\n", (unsigned)dbm);
    // Applied via config_lora_params on the next radio_set_params() call.
}

extern "C" uint32_t radio_get_rng_seed(void) {
    return esp_random();
}

extern "C" bool meck_radio_attach(void) {
    if (!SX1262) { printf("meck_radio_attach() - SX1262 null!\n"); return false; }
    printf("meck_radio_attach() - applying MeshCore LoRa preset\n");

    radio_set_params(LORA_FREQ_DEFAULT, LORA_BW_DEFAULT,
                     LORA_SF_DEFAULT, LORA_CR_DEFAULT);
    radio_set_tx_power(LORA_TX_POWER_DEFAULT);

    // Enter RX with a MeshCore-friendly IRQ mask.
    SX1262->clear_buffer();
    SX1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
    SX1262->set_irq_pin_mode(
        Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE,
        Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::DISABLE,
        Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::DISABLE
    );
    SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);

    radio_driver.begin();

    printf("meck_radio_attach() - radio ready on %.3f MHz, SF%u, BW=%.1f kHz\n",
           (double)LORA_FREQ_DEFAULT, (unsigned)LORA_SF_DEFAULT,
           (double)LORA_BW_DEFAULT);
    return true;
}
