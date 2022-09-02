if(CONFIG_SIMPLECBORRPC)
  zephyr_include_directories(
    ${ZEPHYR_SIMPLECBORRPC_MODULE_DIR}/
  )

  zephyr_library()
  zephyr_library_sources(
    ${ZEPHYR_SIMPLECBORRPC_MODULE_DIR}/simplecborrpc.c
    ${ZEPHYR_SIMPLECBORRPC_MODULE_DIR}/default_functions.c
  )
endif()
