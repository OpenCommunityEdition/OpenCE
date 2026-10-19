"""Actual event batching and batch-frame decoding for surface wire tests."""
from . import enum_with, function, read, structure
from .surface_sync import generated as surface_generated


def generated():
    distributed = read('port/linux/game/network_distributed.c')
    header = read('port/linux/game/network_distributed.h')
    coop = read('port/linux/game/network_coop.c')
    types = enum_with(header, '_distributed_message_batch')
    types += enum_with(header, '_distributed_to_clients_reliably')
    types += structure(header, 'distributed_message_header')
    types += structure(distributed, 'distributed_batch')
    types += structure(coop, 'distributed_coop_events_message')
    code = function(read('source/bungie_net/common/message_header.c'), 'build_message_header')
    for name in ('distributed_batch_flush', 'distributed_batch_room', 'distributed_batch_append',
                 'distributed_batch_add', 'distributed_fill_header', 'distributed_entry_size',
                 'distributed_send'):
        code += function(distributed, name)
    for name in ('send_to_clients', 'host_send_events', 'network_coop_event_entry_size'):
        code += function(coop, name)

    # Keep the receiver's real batch-framing branch, including its length
    # checks and recursive delivery. The unbatched dispatch boundary in the
    # fixture admits only correctly sized co-op events to the real consumer.
    receiver = function(distributed, 'network_distributed_handle_message')
    start = receiver.index('if (header.type == _distributed_message_batch)')
    end = receiver.index('{', start) + 1
    depth = 1
    while depth:
        depth += (receiver[end] == '{') - (receiver[end] == '}')
        end += 1
    code += '''
static void surface_decode_batch(long machine_index, word const *message, word size)
{
    struct distributed_message_header header;
    if (size < sizeof(header)) return;
    csmemcpy(&header, message, sizeof(header));
'''
    code += receiver[start:end] + '\n}\n'
    return surface_generated() + (('wire_types.inc', types), ('wire.inc', code))
