"""The actual join packet table and codec reject pre-agreement/truncated joins."""
from pathlib import Path
import re

from harness import constant, enum_with, function, structure
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]


def test_join_codec_requires_complete_shared_agreement(tmp_path):
    client = (ROOT / "source/networking/network_client_manager.c").read_text()
    server = (ROOT / "source/networking/network_server_message_handler.c").read_text()
    messages = (ROOT / "source/networking/network_messages.c").read_text()
    packets = (ROOT / "source/memory/data_packets.c").read_text()
    encoding = (ROOT / "source/memory/data_encoding.c").read_text()
    swapping = (ROOT / "source/memory/byte_swapping.c").read_text()
    swap_header = (ROOT / "source/memory/byte_swapping.h").read_text()
    groups = (ROOT / "source/memory/data_packet_groups.h").read_text()
    client_join = structure(client, "message_client_join_game_request")
    server_join = structure(server, "message_client_join_game_request")
    normalize = lambda s: re.sub(r"\s+", " ", re.sub(r"/\*.*?\*/", "", s, flags=re.S)).strip()
    assert constant(server, "JOIN_GAME_TOKEN_LENGTH") == 16
    assert normalize(client_join.replace("join_token", "join_game_token")) == normalize(
        server_join.replace("JOIN_GAME_TOKEN_LENGTH", "0x10"))
    assert constant(client, "MAXIMUM_MACHINE_NAME_LENGTH") == constant(server, "MAXIMUM_MACHINE_NAME_LENGTH")
    fields = next(fields for fields, name in re.findall(
        r"\t\{\n((?:\t\t.*\n)*?)\t\},\n\tNETWORK_GAME_MESSAGE_DEFINITION\((\w+),", messages)
        if name == "client_join_game_request")
    field_count = re.search(r"client_join_game_request_fields\[(\d+)\]", messages).group(1)
    macros = re.search(r"#define DATA_PACKET_FIELD\(.*?\n\n", messages, re.S).group(0)
    swap_macros = "\n".join(line for line in swap_header.splitlines() if line.startswith("#define SWAP"))
    swap_macros += "\n" + re.search(r"#define SWAP8\(.*?\n\n", swapping, re.S).group(0).replace("ui64", "ULL")
    code = r'''
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte;
typedef unsigned short word;
typedef int boolean;
typedef long byte_swap_code;
#define __int64 long long
#define TRUE 1
#define FALSE 0
#define NONE -1
#define UNSIGNED_CHAR_MAX UCHAR_MAX
#define UNSIGNED_SHORT_MAX USHRT_MAX
#define csmemcpy memcpy
#define csmemset memset
#define csstrcpy strcpy
#define csstrncpy strncpy
#define match_assert(file,line,condition) assert(condition)
#define display_assert(...) assert(0)
#define system_exit exit
#define csprintf(buffer,...) (snprintf(buffer,sizeof(temporary),__VA_ARGS__),buffer)
static char temporary[256];
''' + enum_with(swap_header, "_1byte") + "\n" + swap_macros + "\n"
    code += function(swapping, "byte_swap_memory") + "\n"
    code += structure((ROOT / "source/memory/data_encoding.h").read_text(), "data_encoding_state") + "\n"
    code += enum_with(groups, "_data_packet_field_pad") + "\n"
    code += structure(groups, "data_packet_field") + "\n" + structure(groups, "data_packet_definition") + "\n"
    for name in ("data_encode_new", "data_encode_memory", "data_encode_integer", "data_encode_string",
                 "data_decode_new", "data_decode_memory", "data_decode_byte", "data_decode_short",
                 "data_decode_long", "data_decode_integer", "data_decode_string"):
        code += function(encoding, name) + "\n"
    for name in ("_data_packet_verify", "data_packet_verify", "_data_packet_encode", "data_packet_encode",
                 "_data_packet_decode", "data_packet_decode"):
        code += function(packets, name) + "\n"
    # The game uses two-byte wchar_t on every platform. Keep the real structs
    # and field list, with only that type spelling adapted for the host harness.
    code += "#define wchar_t unsigned short\n#define MAXIMUM_MACHINE_NAME_LENGTH " + str(constant(client, "MAXIMUM_MACHINE_NAME_LENGTH")) + "\n"
    code += client_join + "\n#undef wchar_t\n"
    code += "typedef struct message_client_join_game_request message_client_join_game_request;\n" + macros
    code += "static struct {struct data_packet_field client_join_game_request_fields[" + field_count + "];} data_0030aa68 = {{\n" + fields + "}};\n"
    code += 'static struct data_packet_definition definition = NETWORK_GAME_MESSAGE_DEFINITION(client_join_game_request,"join",message_client_join_game_request);\n'
    code += r'''
int main(void) {
    struct message_client_join_game_request input;
    struct {byte before[16];struct message_client_join_game_request value;byte after[16];} output;
    byte wire[512],copy[512];short size,version;int i,j;
    assert(sizeof(long)==4 && sizeof(input)==348);
    memset(&input,0x39,sizeof(input));input.network_version=32730;
    assert(data_packet_encode(&definition,1,&input,wire,&size,sizeof(wire)));
    assert(size==349);
    memcpy(copy,wire,size);memset(&output,0,sizeof(output));
    assert(data_packet_decode(&definition,copy,size,&output.value,&version,NULL));
    assert(version==1 && !memcmp(&input,&output.value,sizeof(input)));
    /* The previous join has 346 bytes plus the codec version. Every missing
     * suffix, including either agreement byte, must fail the actual decoder. */
    for(i=0;i<size;i++) {
        memcpy(copy,wire,size);memset(&output,0xa5,sizeof(output));
        assert(!data_packet_decode(&definition,copy,(short)i,&output.value,&version,NULL));
        for(j=0;j<16;j++)assert(output.before[j]==0xa5 && output.after[j]==0xa5);
    }
    /* The codec permits old codec revisions; these cannot bypass the required
     * field. Agreement value validation belongs to the real admission test. */
    memcpy(copy,wire,size);copy[0]=0;
    assert(!data_packet_decode(&definition,copy,(short)(size-2),&output.value,&version,NULL));
    return 0;
}
'''
    run_c(tmp_path, code)
