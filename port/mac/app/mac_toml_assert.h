/* GCC 4 has no C11 static_assert: tomlc17.c's is kept as a typedef (the same shim as
   tools/mac_config_checks.sh makes). Included with -include when tomlc17.c is built. */
#define MAC_TOML_JOIN_(a,b) a##b
#define MAC_TOML_JOIN(a,b) MAC_TOML_JOIN_(a,b)
#define static_assert(expression,message) typedef char MAC_TOML_JOIN(mac_toml_assert_,__LINE__)[(expression)?1:-1]
