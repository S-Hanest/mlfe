file(GLOB op_hdrs "*.hpp")
file(GLOB op_srcs "*.cpp")
list(APPEND mlfe_source_files ${op_srcs})
list(APPEND mlfe_header_files ${op_hdrs})
set(mlfe_header_files ${mlfe_header_files} PARENT_SCOPE)
set(mlfe_source_files ${mlfe_source_files} PARENT_SCOPE)
