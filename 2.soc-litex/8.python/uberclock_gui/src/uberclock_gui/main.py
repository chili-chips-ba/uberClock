from __future__ import annotations

import sys
from pathlib import Path

from PyQt6.QtGui import QGuiApplication
from PyQt6.QtQml import QQmlApplicationEngine

from .backend.device_controller import DeviceController


def main() -> int:
    app = QGuiApplication(sys.argv)
    engine = QQmlApplicationEngine()

    controller = DeviceController()
    engine.rootContext().setContextProperty("deviceController", controller)

    qml_file = Path(__file__).resolve().parent / "qml" / "Main.qml"
    engine.load(str(qml_file))
    if not engine.rootObjects():
        return 1
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())

