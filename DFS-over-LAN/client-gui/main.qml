import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Window {
    id: mainWindow
    width: 600
    height: 500
    visible: true
    title: "Client GUI"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Label {
            id: statusLabel
            text: "Connecting..."
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ComboBox {
                id: commandCombo
                model: ["LOOKUP_FILE", "ALLOCATE_CHUNKS", "REGISTER_CHUNK_REPLICA"]
                Layout.preferredWidth: 200
            }

            TextField {
                id: paramsInput
                placeholderText: "Enter parameters"
                Layout.fillWidth: true
            }

            Button {
                id: sendButton
                text: "Send"
                enabled: false
                onClicked: {
                    client.sendCommand(commandCombo.currentText, paramsInput.text)
                    paramsInput.clear()
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            TextArea {
                id: textArea
                readOnly: true
                wrapMode: Text.Wrap
                width: parent.width
                height: contentHeight    
            }
        }
    }

    Connections {
        target: client

        function onResponseReceived(response) {
            textArea.append("Server response: " + response + "\n")
            textArea.forceActiveFocus()
            textArea.positionViewAtEnd()
        }

        function onErrorOccurred(error) {
            textArea.append("Error: " + error + "\n")
            if (error.includes("Connection refused")) {
                statusLabel.text = "Connection failed"
            }
            textArea.forceActiveFocus()
            textArea.positionViewAtEnd()
        }

        function onConnectionStateChanged(connected) {
            if (connected) {
                statusLabel.text = "Connected"
                sendButton.enabled = true
                textArea.append("Connected to server\n")
            }
            else {
                statusLabel.text = "Disconnected"
                sendButton.enabled = false
                textArea.append("Disconnected from server\n")
            }
            textArea.forceActiveFocus()
            textArea.positionViewAtEnd()
        }
    }
}
