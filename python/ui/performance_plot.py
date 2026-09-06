"""
AeroSim - Embedded Performance Visualization
Phase 10B

This module is responsible ONLY for visualization.

All aircraft physics and performance calculations are performed
by the C AeroSim physics engine. Python receives the calculated
results and plots them.
"""

from __future__ import annotations

from typing import Sequence

from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg
from matplotlib.figure import Figure

from PySide6.QtWidgets import (
    QComboBox,
    QVBoxLayout,
    QWidget,
    QSizePolicy,
)

from analysis import PerformancePoint


class PerformancePlotWidget(QWidget):
    """Embedded Matplotlib performance plot."""

    def __init__(self) -> None:
        super().__init__()

        # ------------------------------------------------------------------
        # Simulation data
        # ------------------------------------------------------------------

        self.mach_points: list[PerformancePoint] = []
        self.altitude_points: list[PerformancePoint] = []

        # ------------------------------------------------------------------
        # Matplotlib figure
        # ------------------------------------------------------------------

        self.figure = Figure(
            figsize=(7, 4)
        )

        self.canvas = FigureCanvasQTAgg(
            self.figure
        )

        self.canvas.setMinimumHeight(500)
        self.canvas.setMinimumWidth(600)

        self.canvas.setSizePolicy(
            QSizePolicy.Policy.Expanding,
            QSizePolicy.Policy.Expanding,
        )
        # ------------------------------------------------------------------
        # Plot selector
        # ------------------------------------------------------------------

        self.plot_selector = QComboBox()

        self.plot_selector.addItems(
            [
                "Drag vs Mach",
                "L/D vs Mach",
                "Rate of Climb vs Mach",
                "Thrust vs Mach",
                "Rate of Climb vs Altitude",
                "Thrust vs Altitude",
            ]
        )

        self.plot_selector.currentIndexChanged.connect(
            self._redraw
        )

        # ------------------------------------------------------------------
        # Layout
        # ------------------------------------------------------------------

        layout = QVBoxLayout()

        layout.setContentsMargins(
            5, 5, 5, 5
        )
        
        layout.addWidget(
            self.plot_selector
        )

        layout.addWidget(
            self.canvas,
            1
        )

        self.setLayout(
            layout
        )

    # ======================================================================
    # Public data interface
    # ======================================================================

    def set_data(
        self,
        mach_points: Sequence[PerformancePoint],
        altitude_points: Sequence[PerformancePoint],
    ) -> None:
        """
        Store new performance sweep results.

        The values supplied here originate from the C physics engine.
        This function does not perform aircraft physics calculations.
        """

        if not mach_points:
            raise ValueError(
                "Mach dataset cannot be empty."
            )

        if not altitude_points:
            raise ValueError(
                "Altitude dataset cannot be empty."
            )

        self.mach_points = list(
            mach_points
        )

        self.altitude_points = list(
            altitude_points
        )

        self._redraw()

    # ======================================================================
    # Plot dispatcher
    # ======================================================================

    def _redraw(self) -> None:
        """Redraw the currently selected performance plot."""

        if not self.mach_points:
            return

        if not self.altitude_points:
            return

        mode = self.plot_selector.currentText()

        if mode == "Drag vs Mach":

            self._plot_drag_vs_mach()

        elif mode == "L/D vs Mach":

            self._plot_ld_vs_mach()

        elif mode == "Rate of Climb vs Mach":

            self._plot_roc_vs_mach()

        elif mode == "Thrust vs Mach":

            self._plot_thrust_vs_mach()

        elif mode == "Rate of Climb vs Altitude":

            self._plot_roc_vs_altitude()

        elif mode == "Thrust vs Altitude":

            self._plot_thrust_vs_altitude()

    # ======================================================================
    # Matplotlib helpers
    # ======================================================================

    def _new_axis(self):

        self.figure.clear()

        axis = self.figure.add_subplot(
            111
        )

        return axis

    def _finish(
        self,
        axis,
    ) -> None:

        axis.grid(
            True
        )

        self.figure.tight_layout(
            pad=1.5
        )

        self.canvas.draw()

    # ======================================================================
    # Drag vs Mach
    # ======================================================================

    def _plot_drag_vs_mach(self) -> None:

        axis = self._new_axis()

        x = [
            point.mach
            for point in self.mach_points
        ]

        y = [
            point.drag_n
            for point in self.mach_points
        ]

        axis.plot(
            x,
            y,
            marker="o",
        )

        axis.set_title(
            "Drag vs Mach"
        )

        axis.set_xlabel(
            "Mach"
        )

        axis.set_ylabel(
            "Drag [N]"
        )

        self._finish(
            axis
        )

    # ======================================================================
    # L/D vs Mach
    # ======================================================================

    def _plot_ld_vs_mach(self) -> None:

        axis = self._new_axis()

        x = [
            point.mach
            for point in self.mach_points
        ]

        y = [
            point.lift_to_drag_ratio
            for point in self.mach_points
        ]

        axis.plot(
            x,
            y,
            marker="o",
        )

        axis.set_title(
            "Lift-to-Drag Ratio vs Mach"
        )

        axis.set_xlabel(
            "Mach"
        )

        axis.set_ylabel(
            "L/D [-]"
        )

        self._finish(
            axis
        )

    # ======================================================================
    # Rate of Climb vs Mach
    # ======================================================================

    def _plot_roc_vs_mach(self) -> None:

        axis = self._new_axis()

        x = [
            point.mach
            for point in self.mach_points
        ]

        y = [
            point.rate_of_climb_m_s
            for point in self.mach_points
        ]

        axis.plot(
            x,
            y,
            marker="o",
        )

        axis.set_title(
            "Rate of Climb vs Mach"
        )

        axis.set_xlabel(
            "Mach"
        )

        axis.set_ylabel(
            "Rate of Climb [m/s]"
        )

        self._finish(
            axis
        )

    # ======================================================================
    # Thrust vs Mach
    # ======================================================================

    def _plot_thrust_vs_mach(self) -> None:

        axis = self._new_axis()

        x = [
            point.mach
            for point in self.mach_points
        ]

        y = [
            point.thrust_available_n
            for point in self.mach_points
        ]

        axis.plot(
            x,
            y,
            marker="o",
        )

        axis.set_title(
            "Available Thrust vs Mach"
        )

        axis.set_xlabel(
            "Mach"
        )

        axis.set_ylabel(
            "Thrust [N]"
        )

        self._finish(
            axis
        )

    # ======================================================================
    # Rate of Climb vs Altitude
    # ======================================================================

    def _plot_roc_vs_altitude(self) -> None:

        axis = self._new_axis()

        x = [
            point.altitude_m / 1000.0
            for point in self.altitude_points
        ]

        y = [
            point.rate_of_climb_m_s
            for point in self.altitude_points
        ]

        axis.plot(
            x,
            y,
            marker="o",
        )

        axis.set_title(
            "Rate of Climb vs Altitude"
        )

        axis.set_xlabel(
            "Altitude [km]"
        )

        axis.set_ylabel(
            "Rate of Climb [m/s]"
        )

        self._finish(
            axis
        )

    # ======================================================================
    # Thrust vs Altitude
    # ======================================================================

    def _plot_thrust_vs_altitude(self) -> None:

        axis = self._new_axis()

        x = [
            point.altitude_m / 1000.0
            for point in self.altitude_points
        ]

        y = [
            point.thrust_available_n
            for point in self.altitude_points
        ]

        axis.plot(
            x,
            y,
            marker="o",
        )

        axis.set_title(
            "Available Thrust vs Altitude"
        )

        axis.set_xlabel(
            "Altitude [km]"
        )

        axis.set_ylabel(
            "Thrust [N]"
        )

        self._finish(
            axis
        )