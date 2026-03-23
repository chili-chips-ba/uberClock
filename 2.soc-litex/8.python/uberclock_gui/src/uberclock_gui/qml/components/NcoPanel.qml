import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    title: "NCO Control"

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            text: "phase_nco maps to the existing firmware command and CSR field."
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: "Phase Increment"
                Layout.preferredWidth: 160
            }

            SpinBox {
                id: phaseSpin
                from: 0
                to: 67108863
                value: deviceController.phaseNco
                editable: true
                Layout.fillWidth: true
            }

            Button {
                text: "Apply"
                onClicked: deviceController.setPhaseNco(phaseSpin.value)
            }
        }

        Label {
            text: "Current value: " + deviceController.phaseNco
            color: "#4d5b6a"
        }
    }
}

