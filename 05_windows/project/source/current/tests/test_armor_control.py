import struct
import unittest

from armor_control import ArmorControl
from target_wire import ARMOR_READY, ARMOR_STATUS, ARMOR_SUPPORTED, decode, frame


READY = ARMOR_READY | ARMOR_SUPPORTED


def reply(result, color, ticket):
    return decode(frame(ARMOR_STATUS, 0, struct.pack('<BBI5x', result, color, ticket)))[1]


class ArmorTests(unittest.TestCase):
    def setUp(self):
        self.now = 10.0
        self.lamp = ArmorControl(lambda: self.now)

    def send(self, color):
        self.lamp.set_color(color, READY)
        packet = self.lamp.take(READY)
        self.assertEqual((len(packet), packet[2], packet[4]), (17, 8, color))
        self.assertIsNone(self.lamp.take(READY))
        return struct.unpack_from('<I', packet, 5)[0]

    def test_no_lamp_command_for_old_firmware_or_unavailable_can(self):
        for flags in (None, 0, ARMOR_SUPPORTED):
            self.assertFalse(self.lamp.snapshot(flags)['ready'])
            with self.assertRaises(ValueError):
                self.lamp.set_color(1, flags)

    def test_three_colors_require_a_matching_board_report(self):
        for color in (0, 1, 2):
            ticket = self.send(color)
            self.lamp.receive(reply(1, color, ticket ^ 2 or 1))
            self.lamp.receive(reply(1, (color + 1) % 3, ticket))
            self.assertTrue(self.lamp.snapshot(READY)['pending'])
            self.lamp.receive(reply(1, color, ticket))
            state = self.lamp.snapshot(READY)
            self.assertFalse(state['pending'])
            self.assertEqual(state['color'], color)

    def test_unsolicited_switch_update_does_not_confirm_request(self):
        self.send(1)
        self.lamp.receive(reply(0, 2, 0))
        state = self.lamp.snapshot(READY)
        self.assertTrue(state['pending'])
        self.assertEqual(state['color'], 2)

    def test_timeout_and_reset_do_not_retry_the_command(self):
        ticket = self.send(1)
        self.now += 2
        self.assertIn('超时', self.lamp.snapshot(READY)['message'])
        self.assertIsNone(self.lamp.take(READY))
        self.lamp.reset()
        self.lamp.receive(reply(1, 1, ticket))
        self.assertIsNone(self.lamp.snapshot(READY)['color'])

    def test_invalid_reply_never_looks_like_confirmation(self):
        for result, color, ticket in ((1, 255, 42), (1, 0, 0), (0, 0, 42)):
            with self.assertRaises(ValueError):
                reply(result, color, ticket)

    def test_cycle_waits_for_matching_reply_and_stops_on_timeout(self):
        self.lamp.set_cycle(True, READY)
        self.lamp.cycle_tick(READY)
        packet = self.lamp.take(READY)
        self.assertEqual(packet[4], 1)
        ticket = struct.unpack_from('<I', packet, 5)[0]
        self.now += .4
        self.lamp.cycle_tick(READY)
        self.assertIsNone(self.lamp.take(READY))
        self.lamp.receive(reply(1, 1, ticket))
        self.now += 1
        self.lamp.cycle_tick(READY)
        packet = self.lamp.take(READY)
        self.assertEqual(packet[4], 2)
        self.now += 1.4
        self.lamp.cycle_tick(READY)
        self.assertFalse(self.lamp.snapshot(READY)['cycling'])
        self.assertIn('超时', self.lamp.snapshot(READY)['message'])

    def test_cycle_disconnect_or_manual_stop_does_not_replay(self):
        self.lamp.set_cycle(True, READY)
        self.lamp.cycle_tick(READY)
        self.lamp.cycle_tick(None)
        self.assertFalse(self.lamp.snapshot(READY)['cycling'])
        self.assertIsNone(self.lamp.take(READY))
        self.lamp.set_cycle(True, READY)
        self.lamp.cycle_tick(READY)
        self.lamp.set_cycle(False, READY)
        self.assertIsNone(self.lamp.take(READY))
