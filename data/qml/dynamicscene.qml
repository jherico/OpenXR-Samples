import QtQuick 2.0

Item {
    width: 800
    height: 600
    Rectangle {
        id: window
        color: "red"
        width: 100
        height: 100
        //anchors.fill: parent
        anchors.margins: 50
        ColorAnimation on color { loops: Animation.Infinite; from: "blue"; to: "yellow"; duration: 1000 }
    }
}
