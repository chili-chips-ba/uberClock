import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "components"

ApplicationWindow {
    visible: true
    width: 960
    height: 640
    title: "UberClock GUI"
    color: "#f5f2ea"

    header: ToolBar {
        contentHeight: 52

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            Label {
                text: "UberClock"
                font.pixelSize: 24
                font.bold: true
                color: "#1e2a39"
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: deviceController.statusText
                color: "#4d5b6a"
            }

            Button {
                text: "Connect Demo"
                onClicked: deviceController.connectDemo()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#f5f2ea" }
            GradientStop { position: 1.0; color: "#d9e6f2" }
        }

        ScrollView {
            anchors.fill: parent
            anchors.margins: 24

            ColumnLayout {
                width: parent.width
                spacing: 20

                NcoPanel {
                    Layout.fillWidth: true
                }

                GroupBox {
                    title: "Console Log"
                    Layout.fillWidth: true

                    TextArea {
                        readOnly: true
                        text: deviceController.logText
                        wrapMode: TextEdit.Wrap
                        implicitHeight: 240
                    }
                }
            }
        }
    }
}

