#include "tpms_generic.h"
#include <lib/toolbox/stream/stream.h>
#include <lib/flipper_format/flipper_format_i.h>
#include "../helpers/tpms_types.h"

#define TAG "TPMSBlockGeneric"

void tpms_block_generic_get_preset_name(const char* preset_name, FuriString* preset_str) {
    const char* preset_name_temp;
    if(!strcmp(preset_name, "AM270")) {
        preset_name_temp = "FuriHalSubGhzPresetOok270Async";
    } else if(!strcmp(preset_name, "AM650")) {
        preset_name_temp = "FuriHalSubGhzPresetOok650Async";
    } else if(!strcmp(preset_name, "FM238")) {
        preset_name_temp = "FuriHalSubGhzPreset2FSKDev238Async";
    } else if(!strcmp(preset_name, "FM476")) {
        preset_name_temp = "FuriHalSubGhzPreset2FSKDev476Async";
    } else {
        preset_name_temp = "FuriHalSubGhzPresetCustom";
    }
    furi_string_set(preset_str, preset_name_temp);
}

SubGhzProtocolStatus tpms_block_generic_serialize(
    TPMSBlockGeneric* instance,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(instance);
    SubGhzProtocolStatus res = SubGhzProtocolStatusError;
    FuriString* temp_str;
    temp_str = furi_string_alloc();
    do {
        stream_clean(flipper_format_get_raw_stream(flipper_format));
        if(!flipper_format_write_header_cstr(
               flipper_format, TPMS_KEY_FILE_TYPE, TPMS_KEY_FILE_VERSION)) {
            FURI_LOG_E(TAG, "Unable to add header");
            res = SubGhzProtocolStatusErrorParserHeader;
            break;
        }

        if(!flipper_format_write_uint32(flipper_format, "Frequency", &preset->frequency, 1)) {
            FURI_LOG_E(TAG, "Unable to add Frequency");
            res = SubGhzProtocolStatusErrorParserFrequency;
            break;
        }

        tpms_block_generic_get_preset_name(furi_string_get_cstr(preset->name), temp_str);
        if(!flipper_format_write_string_cstr(
               flipper_format, "Preset", furi_string_get_cstr(temp_str))) {
            FURI_LOG_E(TAG, "Unable to add Preset");
            res = SubGhzProtocolStatusErrorParserPreset;
            break;
        }
        if(!strcmp(furi_string_get_cstr(temp_str), "FuriHalSubGhzPresetCustom")) {
            if(!flipper_format_write_string_cstr(
                   flipper_format, "Custom_preset_module", "CC1101")) {
                FURI_LOG_E(TAG, "Unable to add Custom_preset_module");
                res = SubGhzProtocolStatusErrorParserCustomPreset;
                break;
            }
            if(!flipper_format_write_hex(
                   flipper_format, "Custom_preset_data", preset->data, preset->data_size)) {
                FURI_LOG_E(TAG, "Unable to add Custom_preset_data");
                res = SubGhzProtocolStatusErrorParserCustomPreset;
                break;
            }
        }
        if(!flipper_format_write_string_cstr(flipper_format, "Protocol", instance->protocol_name)) {
            FURI_LOG_E(TAG, "Unable to add Protocol");
            res = SubGhzProtocolStatusErrorParserProtocolName;
            break;
        }

        uint32_t temp_data = instance->id;
        if(!flipper_format_write_uint32(flipper_format, "Id", &temp_data, 1)) {
            FURI_LOG_E(TAG, "Unable to add Id");
            res = SubGhzProtocolStatusErrorParserOthers;
            break;
        }

        temp_data = instance->data_count_bit;
        if(!flipper_format_write_uint32(flipper_format, "Bit", &temp_data, 1)) {
            FURI_LOG_E(TAG, "Unable to add Bit");
            res = SubGhzProtocolStatusErrorParserBitCount;
            break;
        }

        uint8_t key_data[sizeof(uint64_t)] = {0};
        for(size_t i = 0; i < sizeof(uint64_t); i++) {
            key_data[sizeof(uint64_t) - i - 1] = (instance->data >> (i * 8)) & 0xFF;
        }

        if(!flipper_format_write_hex(flipper_format, "Data", key_data, sizeof(uint64_t))) {
            FURI_LOG_E(TAG, "Unable to add Data");
            res = SubGhzProtocolStatusErrorParserOthers;
            break;
        }

        temp_data = instance->battery_low;
        if(!flipper_format_write_uint32(flipper_format, "Batt", &temp_data, 1)) {
            FURI_LOG_E(TAG, "Unable to add Battery_low");
            res = SubGhzProtocolStatusErrorParserOthers;
            break;
        }

        float temp = instance->pressure;
        if(!flipper_format_write_float(flipper_format, "Pressure", &temp, 1)) {
            FURI_LOG_E(TAG, "Unable to add Pressure");
            res = SubGhzProtocolStatusErrorParserOthers;
            break;
        }

        //DATE AGE set
        DateTime curr_dt;
        furi_hal_rtc_get_datetime(&curr_dt);
        uint32_t curr_ts = datetime_datetime_to_timestamp(&curr_dt);

        temp_data = curr_ts;
        if(!flipper_format_write_uint32(flipper_format, "Ts", &temp_data, 1)) {
            FURI_LOG_E(TAG, "Unable to add timestamp");
            res = SubGhzProtocolStatusErrorParserOthers;
            break;
        }

        temp = instance->temperature;
        if(!flipper_format_write_float(flipper_format, "Temp", &temp, 1)) {
            FURI_LOG_E(TAG, "Unable to add Temperature");
            res = SubGhzProtocolStatusErrorParserOthers;
            break;
        }

        res = SubGhzProtocolStatusOk;
    } while(false);
    furi_string_free(temp_str);
    return res;
}

SubGhzProtocolStatus
    tpms_block_generic_deserialize(TPMSBlockGeneric* instance, FlipperFormat* flipper_format) {
    furi_assert(instance);
    SubGhzProtocolStatus res = SubGhzProtocolStatusError;
    uint32_t temp_data = 0;

    do {
        if(!flipper_format_rewind(flipper_format)) {
            FURI_LOG_E(TAG, "Rewind error");
            res = SubGhzProtocolStatusErrorParserOthers;
            break;
        }

        if(!flipper_format_read_uint32(flipper_format, "Id", &temp_data, 1)) {
            FURI_LOG_E(TAG, "Missing Id");
            res = SubGhzProtocolStatusErrorParserOthers;
            break;
        }
        instance->id = temp_data;

        if(!flipper_format_read_uint32(flipper_format, "Bit", &temp_data, 1)) {
            FURI_LOG_E(TAG, "Missing Bit");
            res = SubGhzProtocolStatusErrorParserBitCount;
            break;
        }
        instance->data_count_bit = (uint8_t)temp_data;

        uint8_t key_data[sizeof(uint64_t)] = {0};
        if(!flipper_format_read_hex(flipper_format, "Data", key_data, sizeof(uint64_t))) {
            FURI_LOG_E(TAG, "Missing Data");
            res = SubGhzProtocolStatusErrorParserOthers;
            break;
        }

        for(uint8_t i = 0; i < sizeof(uint64_t); i++) {
            instance->data = instance->data << 8 | key_data[i];
        }

        if(!flipper_format_read_uint32(flipper_format, "Batt", &temp_data, 1)) {
            FURI_LOG_E(TAG, "Missing Battery_low");
            res = SubGhzProtocolStatusErrorParserOthers;
            break;
        }
        instance->battery_low = (uint8_t)temp_data;

        float temp;
        if(!flipper_format_read_float(flipper_format, "Pressure", &temp, 1)) {
            FURI_LOG_E(TAG, "Missing Pressure");
            res = SubGhzProtocolStatusErrorParserOthers;
            break;
        }
        instance->pressure = temp;

        if(!flipper_format_read_uint32(flipper_format, "Ts", &temp_data, 1)) {
            FURI_LOG_E(TAG, "Missing timestamp");
            res = SubGhzProtocolStatusErrorParserOthers;
            break;
        }
        instance->timestamp = temp_data;

        if(!flipper_format_read_float(flipper_format, "Temp", &temp, 1)) {
            FURI_LOG_E(TAG, "Missing Temperature");
            res = SubGhzProtocolStatusErrorParserOthers;
            break;
        }
        instance->temperature = temp;

        res = SubGhzProtocolStatusOk;
    } while(0);

    return res;
}

SubGhzProtocolStatus tpms_block_generic_deserialize_check_count_bit(
    TPMSBlockGeneric* instance,
    FlipperFormat* flipper_format,
    uint16_t count_bit) {
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;
    do {
        ret = tpms_block_generic_deserialize(instance, flipper_format);
        if(ret != SubGhzProtocolStatusOk) {
            break;
        }
        if(instance->data_count_bit != count_bit) {
            FURI_LOG_E(TAG, "Wrong number of bits in key");
            ret = SubGhzProtocolStatusErrorValueBitCount;
            break;
        }
    } while(false);
    return ret;
}
