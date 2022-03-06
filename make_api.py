import sys
from api_gen import generate_api, CborTypes

sys.path.append(sys.argv[1])
module = __import__('rpc_funcs_table')
generate_api(sys.argv[1], module.rpc_table_in)