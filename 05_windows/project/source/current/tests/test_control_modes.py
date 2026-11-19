import unittest
from unittest.mock import Mock
from target_ble import BleTargetAdapter

class Modes(unittest.TestCase):
    def setUp(self):
        self.t=BleTargetAdapter(background=False)
        self.t.connected=True
    def test_switch_requires_new_feedback(self):
        t=self.t
        t.set_control_mode('lighting')
        self.assertEqual(t.control_mode, 'stopping')
        with self.assertRaises(ValueError): t.armor_set(1)
        with self.assertRaises(ValueError): t.command('enable')
        self.assertTrue(t.locked)
        self.assertFalse(t.en)
    def test_motion_waits_for_inflight_lamp(self):
        t=self.t
        t.control_mode='lighting'
        t.armor.pending=(1, 1, t.clock(), t.clock())
        t.armor.cycling=True
        with self.assertRaises(ValueError): t.set_control_mode('motion')
        self.assertFalse(t.armor.cycling)
        self.assertEqual(t.control_mode, 'lighting')
        t.armor.pending=None
        t.armor.color=1
        t.set_control_mode('motion')
        self.assertEqual(t.armor.color,1)
        self.assertTrue(t.locked)
        with self.assertRaises(ValueError): t.armor_set(2)
    def test_refuses_disconnected_lighting(self):
        self.t.connected=False
        with self.assertRaises(ValueError): self.t.set_control_mode('lighting')
        self.assertEqual(self.t.control_mode,'motion')
if __name__ == '__main__': unittest.main()
