import machine

class CoController():
    def __init__(self):
        self.co_uart = machine.UART(1, 9600) #pins P3 and P4

    def _wait(self):
        """
           We need to let the user know we are working on something.
        """
        self._send('w0')

    def _print(self, _message):
        _msg = 'p' + _message
        self._send(_msg)

    def _finish(self):
        """
           Reward routine to indicate successful wayfinding
        """
        self._send('s0')

    def _update_direction(self, heading):
        _msg = 'h' + str(heading)
        self._send(_msg)

    def _send(self, payload):
        payload = '<'+payload+'>'
        self.co_uart.write(payload)
