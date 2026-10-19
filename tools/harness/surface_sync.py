"""Production-source units shared by the 32-bit and native surface tests."""
from . import enum_with, function, read, structure


def generated():
    coop = read('port/linux/game/network_coop.c')
    surfaces = read('source/physics/breakable_surfaces.c')
    header = read('source/physics/breakable_surfaces.h')
    types = enum_with(coop, '_coop_event_reverted')
    types += structure(coop, 'distributed_coop_event')
    for name in ('breakable_surface_datum', 'structure_breakable_surface'):
        types += structure(header, name)
    types += structure(surfaces, 'breakable_surface_globals')
    code = ''
    for name in ('breakable_surface_indices_valid', 'breakable_surface_flags_get',
                 'breakable_surface_get', 'breakable_surface_extant',
                 'breakable_surface_port_break', 'breakable_surface_port_restore'):
        code += function(surfaces, name)
    for name in ('event_new', 'network_coop_reverted', 'client_apply_surface_state',
                 'client_apply_event', 'network_coop_handle_events', 'host_send_broken_surfaces'):
        code += function(coop, name)
    return (('types.inc', types), ('under_test.inc', code))
