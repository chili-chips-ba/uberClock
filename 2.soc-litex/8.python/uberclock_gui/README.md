# UberClock GUI Starter

This folder contains a minimal PyQt6 + QML scaffold for a desktop control app.

Current scope:

- `src/uberclock_gui/main.py`: Qt entry point
- `src/uberclock_gui/backend/device_controller.py`: backend object exposed to QML
- `src/uberclock_gui/qml/Main.qml`: main window
- `src/uberclock_gui/qml/components/NcoPanel.qml`: starter control panel for `phase_nco`

Run it from `8.python/` with:

```bash
PYTHONPATH=uberclock_gui/src python -m uberclock_gui.main
```

Install GUI dependencies first:

```bash
pip install pyqt6
```

Next step:

- replace the demo backend in `device_controller.py` with a serial/UART transport that sends firmware commands such as `phase_nco 12345`
- or replace it with direct LiteX CSR access if you expose a host-side transport
