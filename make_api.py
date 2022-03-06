import os
import sys
from api_gen import generate_api, CborTypes

current_path = os.path.dirname(os.path.realpath(__file__))
sys.path.append(sys.argv[1])
module = __import__('rpc_funcs_table')
generate_api(current_path, module.rpc_table_in)