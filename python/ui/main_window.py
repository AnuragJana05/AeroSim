"""
AeroSim - Phase 10A
Interactive User Interface

The UI contains NO aircraft physics.

All physics calculations are delegated to the C engine through
ctypes_bridge.py.
"""

from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QDoubleSpinBox,
    QFormLayout,
    QGridLayout,
    QGroupBox,
    QLabel,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QScrollArea,
    QVBoxLayout,
    QWidget,
)

from ctypes_bridge import AeroSimAircraft
from analysis import (
    evaluate_point,
    sweep_mach,
    sweep_altitude,
)

from ui.performance_plot import PerformancePlotWidget


class AeroSimMainWindow(QMainWindow):
    """Main AeroSim application window."""

    def __init__(self) -> None:
        super().__init__()

        self.setWindowTitle(
            "AeroSim — Aircraft Performance & Mission Simulator"
        )

        self.resize(1200, 800)

        self._build_ui()

    # ======================================================================
    # UI construction
    # ======================================================================

    def _build_ui(self) -> None:

        central_widget = QWidget()

        main_layout = QVBoxLayout(
            central_widget
        )

        title = QLabel(
            "AeroSim"
        )

        title.setAlignment(
            Qt.AlignmentFlag.AlignCenter
        )

        title.setStyleSheet(
            """
            QLabel {
                font-size: 32px;
                font-weight: bold;
                padding: 15px;
            }
            """
        )

        subtitle = QLabel(
            "Aircraft Performance & Mission Simulator"
        )

        subtitle.setAlignment(
            Qt.AlignmentFlag.AlignCenter
        )

        subtitle.setStyleSheet(
            """
            QLabel {
                font-size: 15px;
                padding-bottom: 15px;
            }
            """
        )

        main_layout.addWidget(title)
        main_layout.addWidget(subtitle)

        content_layout = QGridLayout()

        # --------------------------------------------------------------
        # Aircraft configuration
        # --------------------------------------------------------------

        aircraft_group = QGroupBox(
            "Aircraft Configuration"
        )

        aircraft_layout = QFormLayout()

        self.mass_input = self._create_input(
            10000.0,
            1.0,
            1000000.0,
            1.0,
        )

        self.wing_area_input = self._create_input(
            50.0,
            0.1,
            10000.0,
            1.0,
        )

        self.wingspan_input = self._create_input(
            20.0,
            0.1,
            500.0,
            0.5,
        )

        self.cd0_input = self._create_input(
            0.02,
            0.0001,
            2.0,
            0.001,
        )

        self.oswald_input = self._create_input(
            0.8,
            0.01,
            1.0,
            0.01,
        )

        self.cl_max_input = self._create_input(
            1.5,
            0.1,
            10.0,
            0.1,
        )

        self.thrust_input = self._create_input(
            60000.0,
            1.0,
            5000000.0,
            1000.0,
        )

        self.tsfc_input = self._create_input(
            0.00002,
            0.000001,
            0.001,
            0.000001,
            8,
        )

        aircraft_layout.addRow(
            "Mass [kg]",
            self.mass_input,
        )

        aircraft_layout.addRow(
            "Wing Area [m²]",
            self.wing_area_input,
        )

        aircraft_layout.addRow(
            "Wingspan [m]",
            self.wingspan_input,
        )

        aircraft_layout.addRow(
            "Zero-Lift CD₀",
            self.cd0_input,
        )

        aircraft_layout.addRow(
            "Oswald Efficiency",
            self.oswald_input,
        )

        aircraft_layout.addRow(
            "Maximum CL",
            self.cl_max_input,
        )

        aircraft_layout.addRow(
            "Maximum Static Thrust [N]",
            self.thrust_input,
        )

        aircraft_layout.addRow(
            "TSFC [kg/(N·s)]",
            self.tsfc_input,
        )

        aircraft_group.setLayout(
            aircraft_layout
        )

        # --------------------------------------------------------------
        # Flight condition
        # --------------------------------------------------------------

        flight_group = QGroupBox(
            "Flight Condition"
        )

        flight_layout = QFormLayout()

        self.altitude_input = self._create_input(
            0.0,
            0.0,
            30000.0,
            100.0,
        )

        self.mach_input = self._create_input(
            0.5,
            0.01,
            5.0,
            0.05,
        )

        self.throttle_input = self._create_input(
            1.0,
            0.0,
            1.0,
            0.05,
        )

        flight_layout.addRow(
            "Altitude [m]",
            self.altitude_input,
        )

        flight_layout.addRow(
            "Mach",
            self.mach_input,
        )

        flight_layout.addRow(
            "Throttle",
            self.throttle_input,
        )

        flight_group.setLayout(
            flight_layout
        )

        # --------------------------------------------------------------
        # Run button
        # --------------------------------------------------------------

        self.run_button = QPushButton(
            "RUN SIMULATION"
        )

        self.run_button.setMinimumHeight(
            55
        )

        self.run_button.setStyleSheet(
            """
            QPushButton {
                font-size: 17px;
                font-weight: bold;
            }

            QPushButton:hover {
                padding: 2px;
            }
            """
        )

        self.run_button.clicked.connect(
            self._run_simulation
        )

        flight_layout.addRow(
            self.run_button
        )

        # --------------------------------------------------------------
        # Input layout
        # --------------------------------------------------------------

        content_layout.addWidget(
            aircraft_group,
            0,
            0,
        )

        content_layout.addWidget(
            flight_group,
            0,
            1,
        )

        # --------------------------------------------------------------
        # Results
        # --------------------------------------------------------------

        results_group = QGroupBox(
            "C Engine Performance Results"
        )

        results_layout = QGridLayout()

        self.result_labels: dict[str, QLabel] = {}

        result_definitions = [
            ("Temperature", "K"),
            ("Pressure", "Pa"),
            ("Density", "kg/m³"),
            ("Speed of Sound", "m/s"),
            ("True Airspeed", "m/s"),
            ("Dynamic Pressure", "Pa"),
            ("Lift Coefficient", ""),
            ("Drag Coefficient", ""),
            ("Lift", "N"),
            ("Drag", "N"),
            ("L/D", ""),
            ("Available Thrust", "N"),
            ("Fuel Mass Flow", "kg/s"),
            ("Excess Thrust", "N"),
            ("Excess Power", "W"),
            ("Rate of Climb", "m/s"),
        ]

        for index, (name, unit) in enumerate(
            result_definitions
        ):

            row = index // 2
            column = (index % 2) * 2

            name_label = QLabel(
                name
            )

            value_label = QLabel(
                "--"
            )

            value_label.setStyleSheet(
                """
                QLabel {
                    font-size: 15px;
                    font-weight: bold;
                }
                """
            )

            self.result_labels[name] = value_label

            results_layout.addWidget(
                name_label,
                row,
                column,
            )

            results_layout.addWidget(
                value_label,
                row,
                column + 1,
            )

        results_group.setLayout(
            results_layout
        )

        main_layout.addLayout(
            content_layout
        )

        main_layout.addWidget(
            results_group
        )

        # ============================================================================
        # Embedded performance visualization
        # ============================================================================

        plot_group = QGroupBox(
            "Performance Visualization"
        )

        plot_group.setMinimumHeight(560)

        plot_layout = QVBoxLayout()

        self.performance_plot = PerformancePlotWidget()

        plot_layout.addWidget(
            self.performance_plot
        )

        plot_group.setLayout(
            plot_layout
        )

        main_layout.addWidget(
            plot_group
        )
        # --------------------------------------------------------------
        # Status
        # --------------------------------------------------------------

        self.status_label = QLabel(
            "Ready — C physics engine connected through ctypes."
        )

        self.status_label.setAlignment(
            Qt.AlignmentFlag.AlignCenter
        )

        main_layout.addWidget(
            self.status_label
        )

        # --------------------------------------------------------------
        # Scrollable main application area
        # --------------------------------------------------------------

        scroll_area = QScrollArea()

        scroll_area.setWidgetResizable(
            True
        )

        scroll_area.setHorizontalScrollBarPolicy(
            Qt.ScrollBarPolicy.ScrollBarAlwaysOff
        )

        scroll_area.setVerticalScrollBarPolicy(
            Qt.ScrollBarPolicy.ScrollBarAsNeeded
        )

        scroll_area.setWidget(
            central_widget
        )

        self.setCentralWidget(
            scroll_area
        )

        # ======================================================================
        # Input helper
        # ======================================================================

    @staticmethod
    def _create_input(
        value: float,
        minimum: float,
        maximum: float,
        step: float,
        decimals: int = 3,
    ) -> QDoubleSpinBox:

        widget = QDoubleSpinBox()

        widget.setRange(
            minimum,
            maximum,
    )

        widget.setValue(
            value
    )

        widget.setSingleStep(
            step
    )

        widget.setDecimals(
            decimals
    )

        return widget

    # ======================================================================
    # Build aircraft model
    # ======================================================================

    def _build_aircraft(
        self,
    ) -> AeroSimAircraft:

        return AeroSimAircraft(
            mass_kg=self.mass_input.value(),
            wing_area_m2=self.wing_area_input.value(),
            wingspan_m=self.wingspan_input.value(),
            zero_lift_drag_coefficient=self.cd0_input.value(),
            oswald_efficiency=self.oswald_input.value(),
            maximum_lift_coefficient=self.cl_max_input.value(),
            maximum_static_thrust_n=self.thrust_input.value(),
            tsfc_kg_n_s=self.tsfc_input.value(),
        )

    # ======================================================================
    # Run simulation
    # ======================================================================

    def _run_simulation(self) -> None:

        try:
            aircraft = self._build_aircraft()

            altitude_m = self.altitude_input.value()
            mach = self.mach_input.value()
            throttle = self.throttle_input.value()

            print("DEBUG AIRCRAFT:")
            print("mass =", aircraft.mass_kg)
            print("wing_area =", aircraft.wing_area_m2)
            print("wingspan =", aircraft.wingspan_m)
            print(
                "CD0 =",
                aircraft.zero_lift_drag_coefficient
            )
            print(
                "Oswald =",
                aircraft.oswald_efficiency
            )
            print(
                "CLmax =",
                aircraft.maximum_lift_coefficient
            )
            print(
                "thrust =",
                aircraft.maximum_static_thrust_n
            )
            print(
                "TSFC =",
                aircraft.tsfc_kg_n_s
            )

            print("DEBUG CONDITION:")
            print("altitude =", altitude_m)
            print("mach =", mach)
            print("throttle =", throttle)

            result = evaluate_point(
                aircraft=aircraft,
                altitude_m=altitude_m,
                mach=mach,
                throttle=throttle,
            )

            mach_points = sweep_mach(
                aircraft=aircraft,
                altitude_m=altitude_m,
                mach_values=[
                    0.20,
                    0.25,
                    0.30,
                    0.35,
                    0.40,
                    0.45,
                    0.50,
                    0.55,
                    0.60,
                    0.65,
                    0.70,
                    0.75,
                    0.80,
                    0.85,
                    0.90,
                    0.95,
                    1.00,
                    1.05,
                    1.10,
                    1.15,
                    1.20,
                ],
                throttle=throttle,
            )

            altitude_points = sweep_altitude(
                aircraft=aircraft,
                altitudes_m=[
                    0.0,
                    1000.0,
                    2000.0,
                    3000.0,
                    4000.0,
                    5000.0,
                    6000.0,
                    7000.0,
                    8000.0,
                    9000.0,
                    10000.0,
                    11000.0,
                    12000.0,
                    13000.0,
                    14000.0,
                    15000.0,
                ],
                mach=mach,
                throttle=throttle,
            )

            self.performance_plot.set_data(
                mach_points,
                altitude_points,
            )

            self._display_result(
                result
            )

            self.status_label.setText(
                "Simulation complete — results calculated by C engine."
            )

        except Exception as exc:

            self.status_label.setText(
                "Simulation failed."
            )

            QMessageBox.critical(
                self,
                "AeroSim Error",
                str(exc),
            )

    # ======================================================================
    # Display C-engine result
    # ======================================================================

    def _display_result(
        self,
        result,
    ) -> None:

        values = {
            "Temperature": (
                result.temperature_k,
                ".2f",
            ),

            "Pressure": (
                result.pressure_pa,
                ".2f",
            ),

            "Density": (
                result.density_kg_m3,
                ".5f",
            ),

            "Speed of Sound": (
                result.speed_of_sound_m_s,
                ".2f",
            ),

            "True Airspeed": (
                result.true_airspeed_m_s,
                ".2f",
            ),

            "Dynamic Pressure": (
                result.dynamic_pressure_pa,
                ".2f",
            ),

            "Lift Coefficient": (
                result.lift_coefficient,
                ".5f",
            ),

            "Drag Coefficient": (
                result.drag_coefficient,
                ".5f",
            ),

            "Lift": (
                result.lift_n,
                ".2f",
            ),

            "Drag": (
                result.drag_n,
                ".2f",
            ),

            "L/D": (
                result.lift_to_drag_ratio,
                ".3f",
            ),

            "Available Thrust": (
                result.thrust_available_n,
                ".2f",
            ),

            "Fuel Mass Flow": (
                result.fuel_mass_flow_kg_s,
                ".7f",
            ),

            "Excess Thrust": (
                result.excess_thrust_n,
                ".2f",
            ),

            "Excess Power": (
                result.excess_power_w,
                ".2f",
            ),

            "Rate of Climb": (
                result.rate_of_climb_m_s,
                ".3f",
            ),
        }

        for name, (value, fmt) in values.items():

            self.result_labels[name].setText(
                format(value, fmt)
            )