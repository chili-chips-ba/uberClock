"""uc_gui - PySide6 control panel for the uberClock firmware console.

The GUI talks to the board exclusively through ``litex_term`` (spawned
as a subprocess over a PTY) rather than re-implementing the LiteX BIOS
boot/upload handshake with a raw serial port. This package only adds
structure on top of that text stream:

- ``cmd_catalog``    parses the firmware's ``cmdlist`` dump into typed
                      Python objects. No Qt dependency, so it can be
                      unit-tested without a running application.
- ``widget_factory``  turns one parsed ``Command`` into the right Qt
                      input widget (spin box, dropdown, text field) and
                      a row that sends it.
- ``command_panel``   groups commands and lays out the full scrollable
                      panel shown in the main window.
"""
