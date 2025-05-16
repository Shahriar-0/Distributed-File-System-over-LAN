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
            TextField {
                id: commandInput
                placeholderText: "Enter command"
                Layout.fillWidth: true
                enabled: false
            }

            Button {
                id: sendButton
                text: "Send"
                enabled: false
                onClicked: {
                    client.sendCommand(commandInput.text)
                    commandInput.clear()
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
                commandInput.enabled = true
                sendButton.enabled = true
                textArea.append("Connected to server")
            }
            else {
                statusLabel.text = "Disconnected"
                commandInput.enabled = false
                sendButton.enabled = false
                textArea.append("Disconnected from server")
            }
        }
    }
}
