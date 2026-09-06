"""
AeroSim application entry point.
"""

import sys

from PySide6.QtWidgets import QApplication

from ui.main_window import AeroSimMainWindow


def main() -> None:

    app = QApplication(sys.argv)

    window = AeroSimMainWindow()

    window.show()

    sys.exit(
        app.exec()
    )


if __name__ == "__main__":
    main()