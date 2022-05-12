/* SPDX-License-Identifier: MIT */

#include "simplecborrpc.h"

#define CHECK_CBOR_ENCODE(X) if (X != CborNoError) { return RPC_ENCODE_ERROR; }

static rpc_error_t execute_rpc_call_internal(const rpc_function_entry_t *rpc_functions, size_t rpc_functions_count,
                                             const uint8_t *input_buffer, size_t input_buffer_size,
                                             uint8_t *output_buffer, size_t *output_buffer_size,
                                             uint64_t *transaction_id, const char **error_msg,
                                             void *user_ptr) {
    CborParser parser;
    CborValue outer_it;
    CborValue inner_it, args_it;
    bool optional_transaction_id = false;

    size_t flags = CborValidateMapKeysAreUnique | CborValidateNoIndeterminateLength | CborValidateNoUndefined | CborValidateCompleteData;
    if (cbor_parser_init(input_buffer, input_buffer_size, flags, &parser, &outer_it) != CborNoError)
        return RPC_ERROR_INTERNAL_ERROR;

    size_t handle = rpc_functions_count;
    size_t args_count = 0;

    // process request data
    if (!cbor_value_is_map(&outer_it)) return RPC_ERROR_INVALID_REQUEST;

    if (cbor_value_enter_container(&outer_it, &inner_it) != CborNoError) return RPC_ERROR_PARSER_FAILED;

    while (!cbor_value_at_end(&inner_it)) {
        if (!cbor_value_is_byte_string(&inner_it)) return RPC_ERROR_INVALID_REQUEST;

        bool result = false;

        cbor_value_byte_string_equals(&inner_it, "id", &result);
        if (result) {
            if (cbor_value_advance(&inner_it) != CborNoError) return RPC_ERROR_PARSER_FAILED;

            if (cbor_value_is_unsigned_integer(&inner_it)) {
                cbor_value_get_uint64(&inner_it, transaction_id);
                optional_transaction_id = true;
            } else {
                return RPC_ERROR_PARSE_ERROR;
            }
            if (cbor_value_advance(&inner_it) != CborNoError) return RPC_ERROR_PARSER_FAILED;
            continue;
        }

        cbor_value_byte_string_equals(&inner_it, "m", &result);
        if (result) {
            if (cbor_value_advance(&inner_it) != CborNoError) return RPC_ERROR_PARSER_FAILED;

            int32_t function_index;
            if (cbor_value_is_byte_string(&inner_it)) {
                char tmp[33];
                size_t key_size = 33;
                if (cbor_value_copy_byte_string(&inner_it, tmp, &key_size, NULL) != CborNoError) return RPC_ERROR_PARSER_FAILED;

                if (key_size == 33) return RPC_ERROR_INVALID_REQUEST; // max size is 32 chars + null terminator

                function_index = rpc_lookup_index_by_key(tmp);

                if (function_index < 0) return RPC_ERROR_METHOD_NOT_FOUND;
                else handle = function_index;

            } else if (cbor_value_is_integer(&inner_it)) {
                // access by index
                cbor_value_get_int(&inner_it, (int *)&function_index);

                if (rpc_lookup_key_by_index(function_index) == NULL) return RPC_ERROR_METHOD_NOT_FOUND;

                handle = function_index;
            } else {
                return RPC_ERROR_INVALID_REQUEST;
            }

            if (cbor_value_advance(&inner_it) != CborNoError) return RPC_ERROR_PARSER_FAILED;

            continue;
        }

        cbor_value_byte_string_equals(&inner_it, "p", &result);
        if (result) {
            if (cbor_value_advance(&inner_it) != CborNoError) return RPC_ERROR_PARSER_FAILED;

            if (!cbor_value_is_array(&inner_it)) return RPC_ERROR_INVALID_REQUEST;

            if (cbor_value_get_array_length(&inner_it, &args_count) != CborNoError) return RPC_ERROR_PARSER_FAILED;

            if (cbor_value_enter_container(&inner_it, &args_it) != CborNoError) return RPC_ERROR_PARSER_FAILED;

            if (cbor_value_advance(&inner_it) != CborNoError) return RPC_ERROR_PARSER_FAILED;

            continue;
        }

        return RPC_ERROR_UNEXPECTED_KEY_IN_REQUEST;
    }

    // validate arguments
    if (args_count != rpc_functions[handle].number_of_arguments) return RPC_ERROR_INVALID_ARGS;

    const size_t number_of_arguments = rpc_functions[handle].number_of_arguments;
    if (number_of_arguments > 0) {
        const rpc_argument_type_t *argument_types = rpc_functions[handle].argument_types;
        CborValue args_it_validation;
        memcpy(&args_it_validation, &args_it, sizeof(CborValue));

        size_t i = 0;
        while (!cbor_value_at_end(&args_it_validation)) {
            switch (argument_types[i]) {
                case CBOR_TYPE_NULL:
                    if (!cbor_value_is_null(&args_it_validation)) return RPC_ERROR_INVALID_ARGS;
                    break;

                case CBOR_TYPE_BOOL:
                    if (!cbor_value_is_boolean(&args_it_validation)) return RPC_ERROR_INVALID_ARGS;
                    break;

                case CBOR_TYPE_SIMPLE:
                    if (!cbor_value_is_simple_type(&args_it_validation)) return RPC_ERROR_INVALID_ARGS;
                    break;

                case CBOR_TYPE_SIGNED_INTEGER:
                    if (!cbor_value_is_integer(&args_it_validation)) return RPC_ERROR_INVALID_ARGS;
                    break;

                case CBOR_TYPE_UNSIGNED_INTEGER:
                    if (!cbor_value_is_unsigned_integer(&args_it_validation))
                        return RPC_ERROR_INVALID_ARGS;
                    break;

                case CBOR_TYPE_NEGATIVE_INTEGER:
                    if (!cbor_value_is_negative_integer(&args_it_validation))
                        return RPC_ERROR_INVALID_ARGS;
                    break;

                case CBOR_TYPE_HALF_FLOAT:
                    if (!cbor_value_is_half_float(&args_it_validation)) return RPC_ERROR_INVALID_ARGS;
                    break;

                case CBOR_TYPE_FLOAT:
                    if (!cbor_value_is_float(&args_it_validation)) return RPC_ERROR_INVALID_ARGS;
                    break;

                case CBOR_TYPE_DOUBLE:
                    if (!cbor_value_is_double(&args_it_validation)) return RPC_ERROR_INVALID_ARGS;
                    break;

                case CBOR_TYPE_TEXT_STRING:
                    if (!cbor_value_is_text_string(&args_it_validation)) return RPC_ERROR_INVALID_ARGS;
                    break;

                case CBOR_TYPE_BYTE_STRING:
                    if (!cbor_value_is_byte_string(&args_it_validation)) return RPC_ERROR_INVALID_ARGS;
                    break;

                case CBOR_TYPE_ARRAY:
                    if (!cbor_value_is_array(&args_it_validation)) return RPC_ERROR_INVALID_ARGS;
                    break;

                case CBOR_TYPE_MAP:
                    if (!cbor_value_is_map(&args_it_validation)) return RPC_ERROR_INVALID_ARGS;
                    break;

                default:
                    return RPC_ERROR_INVALID_ARGS;
            }

            if (cbor_value_advance(&args_it_validation) != CborNoError) return RPC_ERROR_PARSER_FAILED;
            i++;
        }
    }

    // execute rpc function
    size_t result_key_count = 1;
    if (true == optional_transaction_id) {
        result_key_count = 2;
    }

    CborEncoder response_encoder, map_encoder;

    cbor_encoder_init(&response_encoder, output_buffer, *output_buffer_size, 0);
    cbor_encoder_create_map(&response_encoder, &map_encoder, result_key_count);

    rpc_error_t rpc_result = rpc_functions[handle].function_ptr(&args_it, &map_encoder, error_msg,
                                                                user_ptr);
    if (true == optional_transaction_id) {
        cbor_encode_byte_string(&map_encoder, "id", 2);
        cbor_encode_uint(&map_encoder, *transaction_id);
    }

    cbor_encoder_close_container(&response_encoder, &map_encoder);
    if (cbor_encoder_get_extra_bytes_needed(&response_encoder) != 0) {
        return RPC_ERROR_ENCODE_ERROR;
    } else {
        *output_buffer_size = cbor_encoder_get_buffer_size(&response_encoder, output_buffer);
        return rpc_result;
    }
}

static const char *error_to_string(rpc_error_t error) {
    switch (error) {
        case RPC_ERROR_METHOD_NOT_FOUND:
            return "Method not found";

        case RPC_ERROR_PARSE_ERROR:
            return "Parse error";

        case RPC_ERROR_INVALID_ARGS:
            return "Invalid arguments";

        case RPC_ERROR_UNEXPECTED_KEY_IN_REQUEST:
            return "Unexpected key in request";

        case RPC_ERROR_PARSER_FAILED:
            return "Internal error (parser failed)";

        case RPC_ERROR_INVALID_REQUEST:
            return "Invalid request";

        case RPC_ERROR_INTERNAL_ERROR:
            return "Internal error";

        case RPC_ERROR_ENCODE_ERROR:
            return "Encode error";

        default:
            return "Unknown error";
    }
}

static const uint8_t encode_error_response[] = {0xA2, 0x62, 0x69, 0x64, 0x0D, 0x63, 0x65, 0x72, 0x72, 0xA2, 0x61, 0x63, 0x39, 0x7D, 0x62, 0x63, 0x6D, 0x73, 0x67, 0x6C, 0x65, 0x6E, 0x63, 0x6F, 0x64, 0x65, 0x5F, 0x65, 0x72, 0x72, 0x6F, 0x72};

#define CHECK_CBOR_ENCODE_OR_SET(X, Y) if (X != CborNoError) { Y = false; }

rpc_error_t
execute_rpc_call(const rpc_function_entry_t *rpc_functions, size_t rpc_functions_count, const uint8_t *input_buffer,
                 size_t input_buffer_size, uint8_t *output_buffer, size_t *output_buffer_size,
                 void *user_ptr) {

    size_t saved_buffer_size = *output_buffer_size;
    uint64_t transaction_id = 0;

    const char *error_msg = NULL;
    rpc_error_t err = execute_rpc_call_internal(rpc_functions, rpc_functions_count, input_buffer, input_buffer_size,
                                                output_buffer, output_buffer_size, &transaction_id,
                                                &error_msg, user_ptr);

    if (err != RPC_OK || error_msg != NULL) {
        size_t map_key_count = 2;
        if (transaction_id != 0) {
            map_key_count = 3;
        }

        CborEncoder response_encoder, map_encoder;

        cbor_encoder_init(&response_encoder, output_buffer, saved_buffer_size, 0);
        cbor_encoder_create_map(&response_encoder, &map_encoder, map_key_count);


        cbor_encode_byte_string(&map_encoder, "e", 1);
        cbor_encode_int(&map_encoder, err);

        cbor_encode_byte_string(&map_encoder, "msg", 3);
        if (error_msg != NULL) cbor_encode_byte_string(&map_encoder, error_msg, strlen(error_msg));
        else cbor_encode_byte_string(&map_encoder, error_to_string(err), strlen(error_to_string(err)));

        if (transaction_id != 0) {
            cbor_encode_byte_string(&map_encoder, "id", 2);
            cbor_encode_uint(&map_encoder, transaction_id);
        }

        cbor_encoder_close_container(&response_encoder, &map_encoder);
        if (cbor_encoder_get_extra_bytes_needed(&map_encoder) != 0) {
            if (*output_buffer_size > sizeof(encode_error_response)) {
                memcpy(output_buffer, encode_error_response, sizeof(encode_error_response));
                *output_buffer_size = sizeof(encode_error_response);
            } else {
                *output_buffer_size = 0;
                return RPC_ERROR_ENCODE_ERROR;
            }
        } else {
            *output_buffer_size = cbor_encoder_get_buffer_size(&response_encoder, output_buffer);
        }
    }

    return err;
}
