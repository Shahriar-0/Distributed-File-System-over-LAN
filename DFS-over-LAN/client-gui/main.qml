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

        Label {
            text: "Uploading:"
            visible: uploadProgressBar.visible
        }

        ProgressBar {
            id: uploadProgressBar
            Layout.fillWidth: true
            visible: false
            from: 0
            to: 1
            value: 0
        }

        Label {
            text: "Downloading:"
            visible: downloadProgressBar.visible
        }

        ProgressBar {
            id: downloadProgressBar
            Layout.fillWidth: true
            visible: false
            from: 0
            to: 1
            value: 0
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
            if (response.includes("corrupt")) {
                textArea.append("Warning: Data corruption detected\n")
            }
            textArea.forceActiveFocus()
            textArea.positionViewAtEnd()
        }

        function onLogReceived(log) {
            textArea.append("Log: " + log + "\n")
            textArea.forceActiveFocus()
            textArea.positionViewAtEnd()
        }

        function onChunkAckReceived(chunkId, nextIp, nextPort, corrupted) {
            textArea.append("Chunk " + chunkId + " acknowledged\n")
            if (corrupted) {
                textArea.append("Warning: Data corruption detected\n")
            }
            textArea.forceActiveFocus()
            textArea.positionViewAtEnd()
        }

        function onChunkDataReceived(chunkId, data, corrupted) {
            textArea.append("Chunk " + chunkId + " received\n")
            if (corrupted) {
                textArea.append("Warning: Data corruption detected\n")
            }
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
            } else {
                statusLabel.text = "Disconnected"
                sendButton.enabled = false
                textArea.append("Disconnected from server\n")
            }
            textArea.forceActiveFocus()
            textArea.positionViewAtEnd()
        }

        function onUploadProgress(current, total) {
            uploadProgressBar.value = current
            uploadProgressBar.to = total
            uploadProgressBar.visible = true
        }

        function onDownloadProgress(current, total) {
            downloadProgressBar.value = current
            downloadProgressBar.to = total
            downloadProgressBar.visible = true
        }

        function onUploadFinished(fileId) {
            textArea.append("Upload finished: " + fileId + "\n")
            uploadProgressBar.value = 0
            uploadProgressBar.to = 1
            uploadProgressBar.visible = false
            textArea.forceActiveFocus()
            textArea.positionViewAtEnd()
        }

        function onDownloadFinished(fileId, filePath, fileSize) {
            textArea.append("Download finished: " + fileId + " to " + filePath + " (" + fileSize + " bytes)\n")
            downloadProgressBar.value = 0
            downloadProgressBar.to = 1
            downloadProgressBar.visible = false
            textArea.forceActiveFocus()
            textArea.positionViewAtEnd()
        }
    }
}
