"""One independent lamp transaction over the existing target connection."""
import secrets

from target_wire import ARMOR_READY, ARMOR_SUPPORTED, armor_set

_RESULTS = {
    1: '装甲板已回报请求颜色',
    2: '装甲板回报超时',
    3: '灯控 CAN 发送失败或总线故障',
    4: '灯控 CAN 正忙，请重试',
}


class ArmorControl:
    def __init__(self, clock):
        self.clock = clock
        self.reset()

    def reset(self):
        self.pending = None
        self.color = None
        self.requested = None
        self.message = ''
        self.cycling = False
        self.next_cycle_at = 0
        self.next_cycle_color = 1

    def _expire(self):
        if self.pending is None:
            return
        ticket, color, queued_at, sent_at = self.pending
        if sent_at is None and self.clock() - queued_at >= .35:
            self.pending = None
            self.cycling = False
            self.message = '灯控命令排队超时，请重试'
        elif sent_at is not None and self.clock() - sent_at >= 1.3:
            self.pending = None
            self.cycling = False
            self.message = '装甲板回报超时'

    def set_color(self, color, flags):
        self._expire()
        if flags is None or not flags & ARMOR_SUPPORTED or not flags & ARMOR_READY:
            raise ValueError('灯控 CAN 尚未就绪')
        if self.pending is not None:
            raise ValueError('正在等待上一条灯控命令的回报')
        ticket = secrets.randbelow(0xffffffff) + 1
        armor_set(ticket, color)
        self.pending = (ticket, color, self.clock(), None)
        self.requested = color
        self.message = '等待装甲板回报'

    def set_cycle(self, enabled, flags):
        if type(enabled) is not bool:
            raise ValueError('循环状态无效')
        self._expire()
        if enabled:
            if flags is None or flags & (ARMOR_SUPPORTED | ARMOR_READY) != (ARMOR_SUPPORTED | ARMOR_READY):
                raise ValueError('灯控 CAN 尚未就绪')
            if self.pending is not None:
                raise ValueError('正在等待上一条灯控命令的回报')
            self.cycling = True
            self.next_cycle_color = 1
            self.next_cycle_at = self.clock()
        else:
            self.cycling = False
            if self.pending is not None and self.pending[3] is None:
                self.pending = None
            self.message = '循环已停止'

    def cycle_tick(self, flags):
        self._expire()
        if not self.cycling:
            return
        if flags is None or flags & (ARMOR_SUPPORTED | ARMOR_READY) != (ARMOR_SUPPORTED | ARMOR_READY):
            self.cycling = False
            if self.pending is not None and self.pending[3] is None:
                self.pending = None
            self.message = '灯控连接中断，循环已停止'
            return
        if self.pending is None and self.clock() >= self.next_cycle_at:
            self.set_color(self.next_cycle_color, flags)

    def take(self, flags):
        self._expire()
        if flags is None or flags & (ARMOR_SUPPORTED | ARMOR_READY) != (ARMOR_SUPPORTED | ARMOR_READY):
            return None
        if self.pending is None:
            return None
        ticket, color, queued_at, sent_at = self.pending
        if sent_at is not None:
            return None
        self.pending = (ticket, color, queued_at, self.clock())
        return armor_set(ticket, color)

    def receive(self, value):
        self._expire()
        if value['result'] == 0:
            self.color = value['color']
            return
        if self.pending is None:
            return
        ticket, color, _, sent_at = self.pending
        if sent_at is None or ticket != value['ticket']:
            return
        if value['result'] == 1 and value['color'] != color:
            return
        self.pending = None
        self.color = value['color']
        self.message = _RESULTS[value['result']]
        if self.cycling:
            if value['result'] == 1:
                self.next_cycle_color = 2 if color == 1 else 1
                self.next_cycle_at = self.clock() + 1.0
            else:
                self.cycling = False

    def snapshot(self, flags):
        self._expire()
        if flags is None:
            status = '请先在主窗口连接靶车'
        elif not flags & ARMOR_SUPPORTED:
            status = '当前靶车 ESP 固件未提供灯控'
        elif not flags & ARMOR_READY:
            status = '靶车 ESP 的灯控 CAN 未就绪'
        else:
            status = self.message or '请选择灯色'
        ready = flags is not None and bool(flags & ARMOR_SUPPORTED and flags & ARMOR_READY)
        return dict(ready=ready, pending=self.pending is not None, message=status,
                    color=self.color if ready else None, requested=self.requested,
                    cycling=self.cycling)
