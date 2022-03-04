import os
from api_gen import generate_api, CborTypes

current_path = os.path.dirname(os.path.realpath(__file__))

rpc_table_in = {
    "uplink_enqueue": [CborTypes.CBOR_TYPE_BYTE_STRING],
    "uplink_enqueue_confirmed": [CborTypes.CBOR_TYPE_BYTE_STRING, CborTypes.CBOR_TYPE_UNSIGNED_INTEGER],
    "uplink_flush": [],
    "uplink_count": [],
    "downlink_pop": [CborTypes.CBOR_TYPE_UNSIGNED_INTEGER],
    "downlink_push": [CborTypes.CBOR_TYPE_BYTE_STRING],
    "datetime_get": [],
    "datetime_set": [CborTypes.CBOR_TYPE_UNSIGNED_INTEGER, CborTypes.CBOR_TYPE_UNSIGNED_INTEGER],
    "alarm_set": [CborTypes.CBOR_TYPE_UNSIGNED_INTEGER, CborTypes.CBOR_TYPE_BOOL],
    "alarm_set_delta": [CborTypes.CBOR_TYPE_UNSIGNED_INTEGER],
    "alarm_clear": [],
    "gnss_acquire": [],
    "gnss_is_active": [],
    "gnss_get_location": [],
    "bootloader_radio_version": [],
    "bootloader_start": [],
    "bootloader_write": [CborTypes.CBOR_TYPE_BYTE_STRING],
    "bootloader_finish": [],
    "bootloader_count": [],
    "sensor_update_version": [],
    "sensor_update_read": [CborTypes.CBOR_TYPE_UNSIGNED_INTEGER, CborTypes.CBOR_TYPE_UNSIGNED_INTEGER],
    "radio_state_get": [],
    "radio_state_reset": [],
    "radio_counters_get": [],
    "radio_counters_reset": [],
    "eui64_get": [],
    "identifiers_get": [],
    "log_get": [CborTypes.CBOR_TYPE_UNSIGNED_INTEGER],
    "reboot": [],
    "radio_api_version": []
}
generate_api(current_path, rpc_table_in)