# -*- coding: utf-8 -*-

import struct
import sys
import time


class Enumeration(object):
    def __init__(self, id, name):
        self._id = id
        self._name = name
        setattr(self.__class__, name, self)
        self.map[id] = self

    def __int__(self):
        return self.id

    def __repr__(self):
        return self.name

    @property
    def id(self):
        return self._id

    @property
    def name(self):
        return self._name

    @classmethod
    def create_from_map(cls):
        for id, name in list(cls.map.items()):
            cls(id, name)


class Request(Enumeration):
    map = {
        0: 'DETACH',
        1: 'DNLOAD',
        2: 'UPLOAD',
        3: 'GETSTATUS',
        4: 'CLRSTATUS',
        5: 'GETSTATE',
        6: 'ABORT',
    }


Request.create_from_map()


class State(Enumeration):
    map = {
        0: 'appIDLE',
        1: 'appDETACH',
        2: 'dfuIDLE',
        3: 'dfuDNLOAD_SYNC',
        4: 'dfuDNBUSY',
        5: 'dfuDNLOAD_IDLE',
        6: 'dfuMANIFEST_SYNC',
        7: 'dfuMANIFEST',
        8: 'dfuMANIFEST_WAIT_RESET',
        9: 'dfuUPLOAD_IDLE',
        10: 'dfuERROR',
    }


State.create_from_map()


class Status(Enumeration):
    map = {
        0x00: 'OK',
        0x01: 'errTARGET',
        0x02: 'errFILE',
        0x03: 'errWRITE',
        0x04: 'errERASE',
        0x05: 'errCHECK_ERASED',
        0x06: 'errPROG',
        0x07: 'errVERIFY',
        0x08: 'errADDRESS',
        0x09: 'errNOTDONE',
        0x0A: 'errFIRMWARE',
        0x0B: 'errVENDOR',
        0x0C: 'errUSBR',
        0x0D: 'errPOR',
        0x0E: 'errUNKNOWN',
        0x0F: 'errSTALLEDPKT',
    }


Status.create_from_map()


class DFU(object):
    verbose = False

    def __init__(self, device, alt):
        device.set_interface_altsetting(interface=0, alternate_setting=alt)
        self._device = device

    def detach(self):
        """Detaches from the DFU target."""
        self._device.ctrl_transfer(0x21, Request.DETACH, 0, 0, None)

    def get_string(self, i=0):
        """Gets a USB descriptor string, to distinguish firmware types."""
        import usb

        # Linux and Mac have different calling conventions for usb.util.get_string(),
        # so we'll try each of them and hope for the best.
        try:
            # Mac calling convention.
            return usb.util.get_string(self._device, 255, i, None)
        except:
            # Linux calling convention.
            return usb.util.get_string(self._device, i, None)

    def bcd(self, b):
        """Converts a byte from BCD to integer."""
        return int("%02x" % b)

    def get_time(self):
        """Returns a datetime object for the radio's current time."""
        self.md380_custom(0x91, 0x01)  # Programming Mode
        self.md380_custom(0xA2, 0x08)  # Access the clock memory.
        time = self.upload(0, 7)  # Read the time bytes as BCD.
        # hexdump("Time is: "+time);
        from datetime import datetime
        dt = datetime(self.bcd(time[0]) * 100 + self.bcd(time[1]),
                      self.bcd(time[2]),
                      self.bcd(time[3]),
                      self.bcd(time[4]),
                      self.bcd(time[5]),
                      self.bcd(time[6]))
        return dt

    def set_time(self):
        from datetime import datetime
        if len(sys.argv) == 3:
            try:
                time_to_set = datetime.strptime(sys.argv[2], '%m/%d/%Y %H:%M:%S')
            except ValueError:
                print("Usage: md380-dfu settime \"mm/dd/yyyy HH:MM:SS\" (with quotes)")
                exit()
        else:
            time_to_set = datetime.now()
        dt = datetime.strftime(time_to_set, '%Y%m%d%H%M%S').decode("hex")
        self.md380_custom(0x91, 0x02)
        self.download(0, b"\xb5" + dt)
        self.wait_till_ready()
        self.md380_reboot()

    def download(self, block_number, data):
        self._device.ctrl_transfer(0x21, Request.DNLOAD, block_number, 0, data)
        # time.sleep(0.1);

    def set_address(self, address):
        a = address & 0xFF
        b = (address >> 8) & 0xFF
        c = (address >> 16) & 0xFF
        d = (address >> 24) & 0xFF
        self._device.ctrl_transfer(0x21, Request.DNLOAD, 0, 0, [0x21, a, b, c, d])
        self.get_status()  # this changes state
        status = self.get_status()  # this gets the status
        if status[2] == State.dfuDNLOAD_IDLE:
            if self.verbose:
                print("Set pointer to 0x%08x." % address)
            self.enter_dfu_mode()
        else:
            if self.verbose:
                print("Failed to set pointer.")
            return False
        return True

    def erase_block(self, address):
        a = address & 0xFF
        b = (address >> 8) & 0xFF
        c = (address >> 16) & 0xFF
        d = (address >> 24) & 0xFF
        self._device.ctrl_transfer(0x21, Request.DNLOAD, 0, 0, [0x41, a, b, c, d])
        # time.sleep(0.5);
        self.get_status()  # this changes state
        status = self.get_status()  # this gets the status
        if status[2] == State.dfuDNLOAD_IDLE:
            if self.verbose:
                print("Erased 0x%08x." % address)
            self.enter_dfu_mode()
        else:
            if self.verbose:
                print("Failed to erase block.")
            return False
        return True

    def md380_custom(self, a, b):
        """Sends a secret MD380 command."""
        a &= 0xFF
        b &= 0xFF
        self._device.ctrl_transfer(0x21, Request.DNLOAD, 0, 0, [a, b])
        self.get_status()  # this changes state
        time.sleep(0.1)
        status = self.get_status()  # this gets the status
        if status[2] == State.dfuDNLOAD_IDLE:
            if self.verbose:
                print("Sent custom %02x %02x." % (a, b))
            self.enter_dfu_mode()
        else:
            print("Failed to send custom %02x %02x." % (a, b))
            return False
        return True

    def md380_reboot(self):
        """Sends the MD380's secret reboot command."""
        a = 0x91
        b = 0x05
        self._device.ctrl_transfer(0x21, Request.DNLOAD, 0, 0, [a, b])
        try:
            self.get_status()  # this changes state
        except:
            pass
        return True

    def upload(self, block_number, length, index=0):
        if self.verbose:
            print("Fetching block 0x%x." % block_number)
        data = self._device.ctrl_transfer(0xA1,  # request type
                                          Request.UPLOAD,  # request
                                          block_number,  # wValue
                                          index,  # index
                                          length)  # length
        return data

    def get_command(self):
        data = self._device.ctrl_transfer(0xA1,  # request type
                                          Request.UPLOAD,  # request
                                          0,  # wValue
                                          0,  # index
                                          32)  # length
        self.get_status()
        return data

    def get_status(self):
        status_packed = self._device.ctrl_transfer(0xA1, Request.GETSTATUS, 0, 0, 6)
        status = struct.unpack('<BBBBBB', status_packed)
        return (Status.map[status[0]], (((status[1] << 8) | status[2]) << 8) | status[3],
                State.map[status[4]], status[5])

    def clear_status(self):
        self._device.ctrl_transfer(0x21, Request.CLRSTATUS, 0, 0, None)

    def get_state(self):
        state_packed = self._device.ctrl_transfer(0xA1, Request.GETSTATE, 0, 0, 1)
        return State.map[struct.unpack('<B', state_packed)[0]]

    def abort(self):
        self._device.ctrl_transfer(0x21, Request.ABORT, 0, 0, None)

    def wait_till_ready(self, desired_state=State.dfuIDLE):
        state = 11
        status, timeout, state, discarded = self.get_status()
        while state != State.dfuIDLE:
            self.clear_status()
            status, timeout, state, discarded = self.get_status()
        return

    def enter_dfu_mode(self):
        action_map = {
            State.dfuDNLOAD_SYNC: self.abort,
            State.dfuDNLOAD_IDLE: self.abort,
            State.dfuMANIFEST_SYNC: self.abort,
            State.dfuUPLOAD_IDLE: self.abort,
            State.dfuERROR: self.clear_status,
            State.appIDLE: self.detach,
            State.appDETACH: self._wait,
            State.dfuDNBUSY: self._wait,
            State.dfuMANIFEST: self.abort,
            State.dfuMANIFEST_WAIT_RESET: self._wait,
            State.dfuIDLE: self._wait
        }

        while True:
            state = self.get_state()
            if state == State.dfuIDLE:
                break
            action = action_map[state]
            action()

    def _wait(self):
        time.sleep(0.1)

    def widestr(self, str):
        tr = ""
        for c in str:
            tr = tr + c + "\0"
        return tr + "\0\0"
