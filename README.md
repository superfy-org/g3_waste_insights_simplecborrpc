# simplecborrpc
A simple, embedded friendly RPC library based on CBOR


## Generating files
This repo contains scripts that generates neccessary cbor-rpc files based on pythons script that contains table with rpc api functions

To generate files script has to be executed as below
```
python3 make_api.py 'dir_with_script_with_rpc_funcs_table'
```

Above will generate 2 files `rpc_api.h` and `rpc_api_common.c` in the same directory as `dir_with_script_with_rpc_funcs_table`

## Integrating repo as submodule in project
To make repo a submodule of project and make use of it functionality below step are necessary:
1. Make a dependencies folder (e.g. name it as g3_waste_insights_simplecborrpc) and clone repo into this folder. Add this folder to .gitmodules like below
   ```
   [submodule "dependencies/tinycbor/tinycbor"]
	   path = dependencies/tinycbor/tinycbor
	   url = git@github.com:taoglas-iot/tinycbor.git
   [submodule "dependencies/pb_schemas/pb_schemas"]
	   path = dependencies/pb_schemas/pb_schemas
	   url = git@github.com:taoglas-iot/g3_waste_insights_protobuf_schemas.git
   [submodule "dependencies/g3_waste_insights_simplehdlc/simplehdlc"]
	   path = dependencies/g3_waste_insights_simplehdlc/simplehdlc
	   url = git@github.com:taoglas-iot/g3_waste_insights_simplehdlc.git
   [submodule "dependencies/g3_waste_insights_simplecborrpc/simplecborrpc"]
	   path = dependencies/g3_waste_insights_simplecborrpc/simplecborrpc
	   url = git@github.com:taoglas-iot/g3_waste_insights_simplecborrpc.git
   ```
2. Add cmake file into g3_waste_insights_simplecborrpc folder
   ```
   set(SIMPLECBORRPC_DIR ${CMAKE_CURRENT_LIST_DIR}/simplecborrpc)

   include(${CMAKE_CURRENT_LIST_DIR}/../tinycbor/CMakeLists.txt)
   execute_process(COMMAND python3 ${SIMPLECBORRPC_DIR}/make_api.py ${RPC_FUNCS_TABLE_DIR})
   target_include_directories(app PRIVATE ${SIMPLECBORRPC_DIR}/)
   target_sources(app PRIVATE
                       ${SIMPLECBORRPC_DIR}/simplecborrpc.c
                       ${SIMPLECBORRPC_DIR}/default_functions.c)
   ```
3. In module's cmake where specific cbor rpc is implemented:

   a. set variable to point dir where rpc_funcs_table.py is
   ```
   set(RPC_FUNCS_TABLE_DIR ${RPC_SERVICE_DIR}/src/)
   ```
   b. include cmake g3_waste_insights_simplecborrpc
   ```
   include(${CMAKE_CURRENT_LIST_DIR}/../../../../dependencies/g3_waste_insights_simplecborrpc/CMakeLists.txt)
   ```

## RPC API FUNCTIONS TABLE
rpc_funcs_table.py should look like below
```
from api_gen import CborTypes
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
```