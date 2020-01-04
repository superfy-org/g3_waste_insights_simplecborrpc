/* SPDX-License-Identifier: MIT */

#include "simplecborrpc.h"

static rpc_error_t execute_rpc_call_internal(const rpc_function_entry_t *rpc_functions, size_t rpc_functions_count,
                                             const uint8_t *input_buffer, size_t input_buffer_size,
                                             uint8_t *output_buffer, size_t *output_buffer_size,
                                             uint64_t *transaction_id, int64_t *error_code, const char **error_msg,
                                             void *user_ptr) {
    CborParser parser;
    CborValue outer_it;
    CborValue inner_it, args_it;

    if (cbor_parser_init(input_buffer, input_buffer_size, 0, &parser, &outer_it) != CborNoError)
        return RPC_ERROR_FAILED_CBOR_PARSER_INIT;

    uint64_t version = 0;
    size_t handle = rpc_functions_count;
    size_t args_count = 0;

    // process request data
    if (!cbor_value_is_map(&outer_it)) return RPC_ERROR_INVALID_REQUEST;

    CborValue id_it;
    if (cbor_value_map_find_value(&outer_it, "id", &id_it) != CborNoError) return RPC_ERROR_PARSER_FAILED;
    if (cbor_value_is_valid(&id_it)) {
        cbor_value_get_uint64(&id_it, transaction_id);
    } else {
        *transaction_id = 0;
    }

    // reset parser
    if (cbor_parser_init(input_buffer, input_buffer_size, 0, &parser, &outer_it) != CborNoError)
        return RPC_ERROR_FAILED_CBOR_PARSER_INIT;
    if (cbor_value_enter_container(&outer_it, &inner_it) != CborNoError) return RPC_ERROR_PARSER_FAILED;

    while (!cbor_value_at_end(&inner_it)) {
        if (!cbor_value_is_text_string(&inner_it)) return RPC_ERROR_INVALID_REQUEST;

        bool result = false;
        cbor_value_text_string_equals(&inner_it, "v", &result);
        if (result) {
            if (cbor_value_advance(&inner_it) != CborNoError) return RPC_ERROR_PARSER_FAILED;

            if (!cbor_value_is_integer(&inner_it)) return RPC_ERROR_INVALID_REQUEST;

            if (cbor_value_get_uint64(&inner_it, &version) != CborNoError) return RPC_ERROR_PARSER_FAILED;

            if (version != 1) return RPC_ERROR_INVALID_REQUEST;

            if (cbor_value_advance(&inner_it) != CborNoError) return RPC_ERROR_PARSER_FAILED;

            continue;
        }

        cbor_value_text_string_equals(&inner_it, "id", &result);
        if (result) {
            if (cbor_value_advance(&inner_it) != CborNoError) return RPC_ERROR_PARSER_FAILED;

            // id was already collected earlier
            if (cbor_value_advance(&inner_it) != CborNoError) return RPC_ERROR_PARSER_FAILED;
            continue;
        }

        cbor_value_text_string_equals(&inner_it, "func", &result);
        if (result) {
            if (cbor_value_advance(&inner_it) != CborNoError) return RPC_ERROR_PARSER_FAILED;

            if (!cbor_value_is_text_string(&inner_it)) return RPC_ERROR_INVALID_REQUEST;

            bool function_match = false;
            for (size_t i = 0; i < rpc_functions_count; i++) {
                cbor_value_text_string_equals(&inner_it, rpc_functions[i].name, &function_match);
                if (function_match) {
                    handle = i;
                    break;
                }
            }

            if (!function_match) return RPC_ERROR_METHOD_NOT_FOUND;

            if (cbor_value_advance(&inner_it) != CborNoError) return RPC_ERROR_PARSER_FAILED;

            continue;
        }

        cbor_value_text_string_equals(&inner_it, "args", &result);
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
    size_t result_key_count = 2;
    if (*transaction_id != 0) {
        result_key_count = 3;
    }

    CborEncoder response_encoder, map_encoder;

    cbor_encoder_init(&response_encoder, output_buffer, *output_buffer_size, 0);
    cbor_encoder_create_map(&response_encoder, &map_encoder, result_key_count);

    cbor_encode_text_stringz(&map_encoder, "v");
    cbor_encode_uint(&map_encoder, 1);

    if (*transaction_id != 0) {
        cbor_encode_text_stringz(&map_encoder, "id");
        cbor_encode_uint(&map_encoder, *transaction_id);
    }

    cbor_encode_text_stringz(&map_encoder, "res");

    rpc_call_result_t result = rpc_functions[handle].function_ptr(&args_it, &map_encoder, error_code, error_msg,
                                                                  user_ptr);

    if (result == RPC_CALL_ERROR_FORCE_METHOD_NOT_FOUND) return RPC_ERROR_METHOD_NOT_FOUND;

    cbor_encoder_close_container(&response_encoder, &map_encoder);
    *output_buffer_size = cbor_encoder_get_buffer_size(&response_encoder, output_buffer);

    return RPC_OK;
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

        default:
            return "Unknown error";
    }
}

rpc_error_t
execute_rpc_call(const rpc_function_entry_t *rpc_functions, size_t rpc_functions_count, const uint8_t *input_buffer,
                 size_t input_buffer_size, uint8_t *output_buffer, size_t *output_buffer_size,
                 void *user_ptr) {

    size_t saved_buffer_size = *output_buffer_size;
    uint64_t transaction_id = 0;

    int64_t error_code = 0;
    const char *error_msg = NULL;
    rpc_error_t call_error = execute_rpc_call_internal(rpc_functions, rpc_functions_count, input_buffer, input_buffer_size,
                                                  output_buffer, output_buffer_size, &transaction_id, &error_code,
                                                  &error_msg, user_ptr);

    if (call_error != RPC_OK || error_code != 0 || error_msg != NULL) {
        size_t result_key_count = 2;
        if (transaction_id != 0) {
            result_key_count = 3;
        }

        CborEncoder response_encoder, map_encoder;

        cbor_encoder_init(&response_encoder, output_buffer, saved_buffer_size, 0);
        cbor_encoder_create_map(&response_encoder, &map_encoder, result_key_count);

        cbor_encode_text_stringz(&map_encoder, "v");
        cbor_encode_uint(&map_encoder, 1);

        if (transaction_id != 0) {
            cbor_encode_text_stringz(&map_encoder, "id");
            cbor_encode_uint(&map_encoder, transaction_id);
        }

        cbor_encode_text_stringz(&map_encoder, "err");

        CborEncoder error_map_encoder;
        cbor_encoder_create_map(&map_encoder, &error_map_encoder, 2);

        if (error_code != 0 || error_msg != NULL) {
            cbor_encode_text_stringz(&error_map_encoder, "c");
            cbor_encode_int(&error_map_encoder, error_code);

            cbor_encode_text_stringz(&error_map_encoder, "msg");
            cbor_encode_text_stringz(&error_map_encoder, error_msg);
        } else {
            cbor_encode_text_stringz(&error_map_encoder, "c");
            cbor_encode_int(&error_map_encoder, call_error);

            cbor_encode_text_stringz(&error_map_encoder, "msg");
            cbor_encode_text_stringz(&error_map_encoder, error_to_string(call_error));
        }


        cbor_encoder_close_container(&map_encoder, &error_map_encoder);

        cbor_encoder_close_container(&response_encoder, &map_encoder);

        *output_buffer_size = cbor_encoder_get_buffer_size(&response_encoder, output_buffer);
    }

    return call_error;
}