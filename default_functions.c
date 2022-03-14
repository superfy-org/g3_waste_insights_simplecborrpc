#include <stddef.h>
#include "cbor.h"
#include "simplecborrpc.h"

rpc_error_t
rpc___ver(const CborValue *args_iterator, CborEncoder *result, const char **error_msg, void *user_ptr) {
    struct CborEncoder arr_enc;

    cbor_encode_byte_string(result, "v", 1);

    cbor_encoder_create_array(result, &arr_enc, 1);

    cbor_encode_int( &arr_enc, 0);

    if (cbor_encoder_close_container(result, &arr_enc) != CborNoError) {
        return RPC_ERROR_ENCODE_ERROR;
    }

    return RPC_OK;
}

rpc_error_t
rpc___ping(const CborValue *args_iterator, CborEncoder *result, const char **error_msg, void *user_ptr) {
    struct CborEncoder arr_enc;

    cbor_encode_byte_string(result, "v", 1);

    cbor_encoder_create_array(result, &arr_enc, 1);
    cbor_encode_byte_string(&arr_enc, "pong", sizeof("pong")-1);
    if (cbor_encoder_close_container(result, &arr_enc) != CborNoError) {
        return RPC_ERROR_ENCODE_ERROR;
    }

    return RPC_OK;
}

rpc_error_t
rpc___lookup(const CborValue *args_iterator, CborEncoder *result, const char **error_msg, void *user_ptr) {
    char func_key[16] = {0};
    size_t func_key_len = sizeof(func_key);
    size_t func_id;
    struct CborEncoder arr_enc;

    if (cbor_value_copy_byte_string(args_iterator, func_key, &func_key_len, NULL) != CborNoError) return RPC_ERROR_PARSER_FAILED;

    func_id = rpc_lookup_index_by_key(func_key);

    cbor_encode_byte_string(result, "v", 1);

    cbor_encoder_create_array(result, &arr_enc, 1);
    cbor_encode_int(&arr_enc, func_id);
    if (cbor_encoder_close_container(result, &arr_enc) != CborNoError) {
        return RPC_ERROR_ENCODE_ERROR;
    }

    return RPC_OK;
}