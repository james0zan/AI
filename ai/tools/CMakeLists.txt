project(TOOLS)
include_directories(..)
add_executable(trace2bset trace2bset.cc)
add_executable(sanitize-bset sanitize-bset.cc)
add_executable(read-violations read-violations.cc)
add_executable(ins-count ins-count.cc)
add_executable(read-ins-group read-ins-group.cc)
add_executable(id-server id-server.cc)
add_executable(relax-bset relax-bset.cc)
add_executable(read-whitelist read-whitelist.cc)
add_executable(custom-trace2bset custom-trace2bset.cc)