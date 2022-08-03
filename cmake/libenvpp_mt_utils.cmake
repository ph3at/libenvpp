set(libenvpp_target_mt $<STREQUAL:$<GENEX_EVAL:$<TARGET_PROPERTY:MSVC_RUNTIME_LIBRARY>>,MultiThreaded>)
set(libenvpp_target_mtd $<STREQUAL:$<GENEX_EVAL:$<TARGET_PROPERTY:MSVC_RUNTIME_LIBRARY>>,MultiThreadedDebug>)
set(libenvpp_target_static_runtime $<OR:${libenvpp_target_mt},${libenvpp_target_mtd}>)
set(libenvpp_mt_suffix $<${libenvpp_target_static_runtime}:_mt>)
