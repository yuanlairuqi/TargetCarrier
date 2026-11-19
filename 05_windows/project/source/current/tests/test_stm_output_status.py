import struct
import unittest

from protocol import Channel, link_description, parse_status


def frame(phase, *, error=0, motion=0, hz=0, online=True, ready=True, ack=7):
    flags=0x81 | (8|32|(16 if ready else 0) if online else 0)
    return struct.pack('<BBBHBBBIIHBB',4,motion,flags,hz,motion,40 if motion else 0,
                       error,ack,ack,0,0,phase)


class OutputStatusTests(unittest.TestCase):
    def test_timer_output_has_separate_direction_and_phase(self):
        s=parse_status(frame(0x82, motion=2, hz=927))
        self.assertTrue(s.output_valid)
        self.assertEqual((s.motion,s.current_hz,s.phase),(2,927,2))

    def test_released_but_latched_and_lost_feedback_are_distinct(self):
        tripped=parse_status(frame(0xe0,error=10,ready=False))
        released=parse_status(frame(0xc0,error=10,ready=False))
        stale=parse_status(frame(0,error=10,online=False,ready=False))
        self.assertEqual((tripped.limit_inputs,released.limit_inputs),(2,0))
        self.assertTrue(released.limit_latched and released.stop_feedback)
        self.assertFalse(released.can_move or stale.output_valid or stale.stop_feedback)
        self.assertIn('反馈失效',link_description(stale)[0])

    def test_reset_ack_requires_current_stop_and_never_queues_movement(self):
        channel=Channel(ramp=True)
        channel.synced=True
        channel.move(1,40,1)
        seq=channel.reset_limit(2)
        channel.take(2)
        channel.status(parse_status(frame(0x80,ack=seq-1)))
        self.assertTrue(channel.barrier)
        self.assertFalse(channel.move(2,40,2.5))
        channel.status(parse_status(frame(0x80,ack=seq)))
        self.assertTrue(channel.synced)
        self.assertFalse(channel.barrier)
        self.assertIsNone(channel.pending)
        self.assertTrue(channel.move(2,40,3)) # a new operator request

    def test_rejects_live_trigger_without_latch(self):
        with self.assertRaises(ValueError):
            parse_status(frame(0x90))
