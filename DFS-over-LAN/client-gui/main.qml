import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Window {
    width: 400
    height: 300
    visible: true
    title: "Client GUI"

    ColumnLayout {
        anchors.fill: parent

        Label {
            id: statusLabel
            text: "Connecting..."
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            ComboBox {
                id: commandCombo
                model: ["LOOKUP_FILE", "ALLOCATE_CHUNKS", "REGISTER_CHUNK_REPLICA"]
                Layout.preferredWidth: 150
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

        TextArea {
            id: textArea
            readOnly: true
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    Connections {
        target: client

        function onResponseReceived(response) {
            textArea.append("Server response: " + response)
        }

        function onErrorOccurred(error) {
            textArea.append("Error: " + error)
            if (error.includes("Connection refused")) {
                statusLabel.text = "Connection failed"
            }
        }

        function onConnectionStateChanged(connected) {
            if (connected) {
                statusLabel.text = "Connected"
                sendButton.enabled = true
                textArea.append("Connected to server")
            }
            else {
                statusLabel.text = "Disconnected"
                sendButton.enabled = false
                textArea.append("Disconnected from server")
            }
        }
    }
}