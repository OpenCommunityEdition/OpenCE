"""Stand-in system link machines for testing large multiplayer sessions.

Joins a native-build Halo host (port/, built with HALO_LINUX) with many
lightweight machines, each with one player, speaking the game's system link
protocol directly: TCP to the host's port 5150 for the game's messages, UDP for
player input. Each machine binds its own loopback address (127.0.0.2,
127.0.0.3, ...) because the host tells machines apart by address, so run it on
the host's computer (or give it --first-address on a network where the
addresses are yours).

    python tools/system_link_bots.py --machines 127 --start

joins 127 machines to the host at 127.0.0.1, marks the map precached, asks
the host to start once everyone is in, then plays: every machine
acknowledges each tick's update and sends its player's input (standing
still and slowly turning), the way a real client keeps a lockstep game
running. The bots do not simulate the game; they only keep up with the
host's update stream. Ctrl+C leaves.

The protocol (network_messages.c): a message is a 2-byte big-endian header
(length << 4 | type << 2, length including the header), then a packet: a
version byte, the fields big-endian (bytes and raw fields as they are), and
the packet type in a trailing byte. The native builds send the game settings
record in pieces of HALO_PORT_NETWORK_GAME_SETTINGS_FRAGMENT_SIZE bytes.
"""

import argparse
import errno
import ipaddress
import math
import selectors
import socket
import struct
import sys
import time

SERVER_PORT = 0x141E
CLIENT_PORT = 0x141F
MESSAGE_TYPE_PACKET = 3
PACKET_VERSION = 1
JOIN_TOKEN = b"message in a bottle"[:16]  # network_game_generate_join_game_token (DEBUG builds)
SETTINGS_FRAGMENT_SIZE = 0xE00
NONE = -1

# network_game_message_type
CLIENT_BROADCAST_GAME_SEARCH = 0
SERVER_GAME_ADVERTISE = 2
SERVER_MACHINE_ACCEPTED = 4
SERVER_MACHINE_REJECTED = 5
SERVER_GAME_SETTINGS_UPDATE = 6
SERVER_PREGAME_COUNTDOWN = 7
SERVER_BEGIN_GAME = 8
SERVER_GRACEFUL_GAME_EXIT_PREGAME = 9
SERVER_PREGAME_KEEP_ALIVE = 10
SERVER_POSTGAME_KEEP_ALIVE = 11
CLIENT_JOIN_GAME_REQUEST = 12
CLIENT_ADD_PLAYER_REQUEST_PREGAME = 13
CLIENT_SETTINGS_REQUEST = 15
CLIENT_GAME_START_REQUEST = 17
CLIENT_MAP_IS_PRECACHED_PREGAME = 19
SERVER_GAME_UPDATE = 20
SERVER_ADD_PLAYER_INGAME = 21
SERVER_REMOVE_PLAYER_INGAME = 22
SERVER_GAME_OVER = 23
CLIENT_LOADED = 24
CLIENT_GAME_UPDATE = 25
SERVER_SWITCH_TO_PREGAME = 30
SERVER_GRACEFUL_GAME_EXIT_POSTGAME = 31

COUNTDOWN_EVENT_START_IMMEDIATELY = 3

MESSAGE_NAMES = {
    SERVER_GAME_ADVERTISE: "advertise", SERVER_MACHINE_ACCEPTED: "machine_accepted",
    SERVER_MACHINE_REJECTED: "machine_rejected", SERVER_GAME_SETTINGS_UPDATE: "settings",
    SERVER_PREGAME_COUNTDOWN: "countdown", SERVER_BEGIN_GAME: "begin_game",
    SERVER_GRACEFUL_GAME_EXIT_PREGAME: "exit_pregame", SERVER_PREGAME_KEEP_ALIVE: "keep_alive",
    SERVER_POSTGAME_KEEP_ALIVE: "postgame_keep_alive", SERVER_GAME_UPDATE: "game_update",
    SERVER_ADD_PLAYER_INGAME: "add_player_ingame", SERVER_REMOVE_PLAYER_INGAME: "remove_player_ingame",
    SERVER_GAME_OVER: "game_over", SERVER_SWITCH_TO_PREGAME: "switch_to_pregame",
    SERVER_GRACEFUL_GAME_EXIT_POSTGAME: "exit_postgame",
}


def network_game_layout(machines, players):
    """Offsets of struct network_game (port/linux/include/halo_port_limits.h)."""
    player_count = 0x114 + machines * 0x44
    players_offset = player_count + 2
    size = players_offset + players * 0x20 + 0xE
    return {"map_name": 0x24, "machine_count": 0x112, "player_count": player_count,
            "players": players_offset, "size": size}


def wide(text, count):
    units = [ord(c) for c in text[:count - 1]]
    units += [0] * (count - len(units))
    return struct.pack(">%dH" % count, *units)


def message(packet_type, payload):
    packet = bytes([PACKET_VERSION]) + payload + bytes([packet_type])
    length = len(packet) + 2
    assert length <= 0xFFF, "message too long for its header"
    return struct.pack(">H", (length << 4) | (MESSAGE_TYPE_PACKET << 2)) + packet


def network_player(name, machine_index, color):
    # shorts 12 (name), shorts 2 (colour, icon), bytes 4 (machine, controller, team, list index)
    return (wide(name, 12) + struct.pack(">hh", color, 0) +
            struct.pack("bbbb", machine_index, 0, NONE, NONE))


def player_action(yaw):
    # longs 6 (control flags, desired facing yaw and pitch, throttle i and j,
    # primary trigger), shorts 3 (weapon, grenade, zoom level); the pad is not sent
    return struct.pack(">Ifffff", 0, yaw, 0.0, 0.0, 0.0, 0.0) + struct.pack(">hhh", 0, 0, NONE)


class Machine:
    def __init__(self, index, address, host, log):
        self.index = index
        self.address = address
        self.host = host
        self.log = log
        self.name = "bot%d" % index
        self.state = "connecting"
        self.machine_index = None
        self.buffer = b""
        self.settings = bytearray()
        self.settings_complete = None
        self.map_name = None
        self.last_update_number = None
        self.updates_received = 0
        self.bytes_received = 0
        self.last_precache_time = 0
        self.player_added = False
        self.player_list_index = None
        self.connect_attempts = 0
        self.retry_time = 0
        self.load_seconds = 1.0
        self.tcp = None
        self.udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # the host's own client holds 0.0.0.0 on this port (with SO_REUSEADDR);
        # Linux lets another socket bind a single address on it only if it
        # sets SO_REUSEADDR too
        self.udp.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.udp.bind((address, CLIENT_PORT))
        self.udp.setblocking(False)

    def connect(self):
        self.connect_attempts += 1
        self.state = "connecting"
        self.tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.tcp.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1 << 20)
        self.tcp.bind((self.address, 0))
        self.tcp.setblocking(False)
        try:
            self.tcp.connect((self.host, SERVER_PORT))
        except BlockingIOError:
            pass
        except OSError as error:
            # a non-blocking connect in progress (Windows says would block)
            if error.errno not in (errno.EINPROGRESS, errno.EWOULDBLOCK, 10035):
                raise

    def close(self, selector):
        """stops using the connection: a connection the host keeps but no one
        reads would fill its send buffer"""
        self.state = "closed"
        if self.tcp:
            try:
                selector.unregister(self.tcp)
            except (KeyError, ValueError):
                pass
            self.tcp.close()
            self.tcp = None

    def send(self, data):
        view = memoryview(data)
        deadline = time.monotonic() + 5
        while view:
            try:
                sent = self.tcp.send(view)
                view = view[sent:]
            except BlockingIOError:
                if time.monotonic() > deadline:
                    raise
                time.sleep(0.001)

    def joined(self):
        self.send(message(CLIENT_JOIN_GAME_REQUEST, wide(self.name, 32) + JOIN_TOKEN))
        self.state = "joining"

    def receive(self):
        try:
            data = self.tcp.recv(1 << 20)
        except BlockingIOError:
            return True
        except ConnectionError:
            self.log("%s: connection lost" % self.name)
            self.state = "closed"
            return False
        if not data:
            self.log("%s: host closed the connection" % self.name)
            self.state = "closed"
            return False
        self.bytes_received += len(data)
        self.buffer += data
        while len(self.buffer) >= 2:
            header = struct.unpack(">H", self.buffer[:2])[0]
            length = header >> 4
            if length < 3:
                self.log("%s: bad message header %04x" % (self.name, header))
                self.state = "closed"
                return False
            if len(self.buffer) < length:
                break
            packet = self.buffer[2:length]
            self.buffer = self.buffer[length:]
            self.handle(packet[-1], packet[1:-1])
        return True

    def handle(self, packet_type, payload):
        if packet_type == SERVER_MACHINE_ACCEPTED:
            random_seed, self.machine_index = struct.unpack(">ih", payload[:6])
            self.state = "pregame"
            self.send(message(CLIENT_SETTINGS_REQUEST, wide(self.name, 32) + bytes([self.machine_index & 0xFF])))
        elif packet_type == SERVER_MACHINE_REJECTED:
            self.log("%s: rejected, reason %d" % (self.name, struct.unpack(">h", payload[:2])[0]))
            self.state = "rejected"
        elif packet_type == SERVER_GAME_SETTINGS_UPDATE:
            total, offset, length = struct.unpack(">HHH", payload[:6])
            data = payload[6:6 + length]
            if offset == 0:
                self.settings = bytearray()
            if offset == len(self.settings):
                self.settings += data
            if len(self.settings) == total:
                self.settings_complete = bytes(self.settings)
                self.map_name = self.settings_complete[0x24:0x24 + 0x80].split(b"\0")[0]
                if not self.player_added and self.machine_index is not None:
                    self.player_added = True
                    color = self.index % 18
                    self.send(message(CLIENT_ADD_PLAYER_REQUEST_PREGAME,
                                      network_player(self.name, self.machine_index, color)))
        elif packet_type == SERVER_BEGIN_GAME:
            self.state = "loading"
            self.loaded_at = time.monotonic() + self.load_seconds
        elif packet_type == SERVER_GAME_UPDATE:
            update_number = struct.unpack(">I", payload[:4])[0]
            self.last_update_number = update_number
            self.updates_received += 1
            if self.state != "ingame":
                self.state = "ingame"
        elif packet_type in (SERVER_GAME_OVER,):
            self.state = "postgame"
        elif packet_type in (SERVER_SWITCH_TO_PREGAME,):
            self.state = "pregame"
            self.last_update_number = None
            self.player_added = True
        elif packet_type in (SERVER_GRACEFUL_GAME_EXIT_PREGAME, SERVER_GRACEFUL_GAME_EXIT_POSTGAME):
            self.log("%s: host ended the game" % self.name)
            self.state = "closed"

    def tick(self, now):
        if self.state == "pregame" and self.map_name and now - self.last_precache_time > 1.0:
            self.last_precache_time = now
            self.send(message(CLIENT_MAP_IS_PRECACHED_PREGAME, self.map_name.ljust(256, b"\0")[:256]))
        if self.state == "loading" and now >= self.loaded_at:
            self.send(message(CLIENT_LOADED, struct.pack(">i", 0)))
            self.state = "loaded"
        if self.state == "ingame" and self.last_update_number is not None:
            # the next update this machine expects (acknowledging the ones it
            # has), then an array of its players' actions: a count byte and one
            # action
            yaw = (now * 0.5 + self.index) % (2 * math.pi)
            payload = (struct.pack(">I", (self.last_update_number + 1) & 0x7FFFFFFF) + bytes([1]) +
                       player_action(yaw))
            try:
                self.udp.sendto(message(CLIENT_GAME_UPDATE, payload), (self.host, SERVER_PORT))
            except OSError:
                pass


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--machines", type=int, default=8)
    parser.add_argument("--first-address", default="127.0.0.2",
                        help="each machine binds this address plus its number")
    parser.add_argument("--start", action="store_true",
                        help="ask the host to start as soon as every machine has a player")
    parser.add_argument("--start-delay", type=float, default=3.0)
    parser.add_argument("--seconds", type=float, default=0, help="leave after this long (0: until Ctrl+C)")
    parser.add_argument("--rate", type=float, default=30.0, help="input messages per second per machine")
    parser.add_argument("--join-rate", type=float, default=20.0, help="machines connecting per second")
    parser.add_argument("--load-seconds", type=float, default=1.0,
                        help="how long each machine takes to load the map once the game begins")
    parser.add_argument("--status-every", type=float, default=5.0)
    options = parser.parse_args()

    started = time.monotonic()

    def log(text):
        print("[%7.2f] %s" % (time.monotonic() - started, text), flush=True)

    try:
        first_address = ipaddress.IPv4Address(options.first_address)
        last_address = ipaddress.IPv4Address(int(first_address) + max(options.machines, 1) - 1)
    except ValueError as error:
        parser.error("--first-address: %s" % error)
    if first_address.is_loopback and not last_address.is_loopback:
        parser.error("--first-address: %d machines from %s run past 127.255.255.255" % (options.machines, first_address))
    if not first_address.is_loopback:
        log("warning: %s is not a loopback address; the machines bind real addresses" % first_address)
    machines = []
    for index in range(options.machines):
        address = str(first_address + index)
        machine = Machine(index + 1, address, options.host, log)
        machine.state = "waiting"
        machine.load_seconds = options.load_seconds
        machines.append(machine)

    # machines connect a few at a time: a host accepts connections from its
    # frame loop, and one whose listen backlog is full refuses the rest
    selector = selectors.DefaultSelector()
    waiting = list(machines)
    next_connect_time = 0
    log("connecting %d machines to %s:%d" % (len(machines), options.host, SERVER_PORT))

    start_requested = False
    all_in_time = None
    last_status = 0
    last_input = 0
    try:
        while True:
            now = time.monotonic()
            if waiting and now >= next_connect_time:
                machine = next((m for m in waiting if m.retry_time <= now), None)
                if machine:
                    waiting.remove(machine)
                    machine.connect()
                    selector.register(machine.tcp, selectors.EVENT_READ | selectors.EVENT_WRITE, machine)
                    next_connect_time = now + 1.0 / options.join_rate
            for key, events in selector.select(timeout=0.005):
                machine = key.data
                if machine.state == "connecting" and events & selectors.EVENT_WRITE:
                    error = machine.tcp.getsockopt(socket.SOL_SOCKET, socket.SO_ERROR)
                    if error:
                        machine.close(selector)
                        if error in (errno.ECONNREFUSED, 10061) and machine.connect_attempts < 20:
                            machine.state = "waiting"
                            machine.retry_time = now + 0.5
                            waiting.append(machine)
                        else:
                            log("%s: connect failed (%d)" % (machine.name, error))
                            machine.state = "closed"
                        continue
                    selector.modify(machine.tcp, selectors.EVENT_READ, machine)
                    machine.joined()
                if events & selectors.EVENT_READ and machine.state != "closed":
                    try:
                        if not machine.receive() or machine.state == "closed":
                            machine.close(selector)
                    except OSError as error:
                        # a reply the host stopped reading, or a reset
                        log("%s: %s" % (machine.name, error))
                        machine.close(selector)
            if now - last_input >= 1.0 / options.rate:
                last_input = now
                for machine in machines:
                    if machine.state != "closed":
                        try:
                            machine.tick(now)
                        except OSError as error:
                            log("%s: %s" % (machine.name, error))
                            machine.close(selector)
            if options.start and not start_requested:
                ready = [m for m in machines if m.state == "pregame" and m.player_added and m.settings_complete]
                if len(ready) == len(machines) and all_in_time is None:
                    all_in_time = now
                    log("all %d machines are in the lobby" % len(machines))
                if all_in_time is not None and now - all_in_time >= options.start_delay:
                    start_requested = True
                    log("asking the host to start the game")
                    try:
                        machines[0].send(message(CLIENT_GAME_START_REQUEST,
                                                 struct.pack(">h", COUNTDOWN_EVENT_START_IMMEDIATELY)))
                    except OSError as error:
                        log("%s: %s" % (machines[0].name, error))
                        machines[0].close(selector)
            if now - last_status >= options.status_every:
                last_status = now
                states = {}
                for machine in machines:
                    states[machine.state] = states.get(machine.state, 0) + 1
                sample = next((m for m in machines if m.settings_complete), None)
                players = machines_in_game = None
                if sample:
                    for machine_slots in (128, 4):
                        layout = network_game_layout(machine_slots, 128 if machine_slots == 128 else 16)
                        if len(sample.settings_complete) == layout["size"]:
                            machines_in_game = struct.unpack_from("<h", sample.settings_complete, layout["machine_count"])[0]
                            players = struct.unpack_from("<h", sample.settings_complete, layout["player_count"])[0]
                updates = [m.last_update_number for m in machines if m.last_update_number is not None]
                log("states %s; host game: %s machines, %s players; latest update %s; received %.1f MB" % (
                    states, machines_in_game, players, max(updates) if updates else None,
                    sum(m.bytes_received for m in machines) / 1e6))
            if options.seconds and now - started >= options.seconds:
                break
            if all(m.state in ("closed", "rejected") for m in machines):
                log("every machine has left")
                break
    except KeyboardInterrupt:
        pass
    for machine in machines:
        if machine.tcp:
            machine.tcp.close()
        machine.udp.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
