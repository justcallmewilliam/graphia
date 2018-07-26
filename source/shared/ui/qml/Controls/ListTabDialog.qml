import QtQuick 2.7
import QtQuick.Window 2.2
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3

import "../../shared/ui/qml/Constants.js" as Constants

// This dialog is conceptually similar to a tabview in a dialog however the user
// interface consists of a vertical list of tabs with buttons to OPTIONALLY
// progress through. The transistions are based on Wizard
//
// Child Items should be a ListTab with a title string
// Failing to do so will leave an empty tab title
BaseParameterDialog
{
    id: root
    default property list<Item> listTabs
    property int currentIndex: 0
    property int enableFinishAtIndex: 0
    property bool nextEnabled: true
    property bool finishEnabled: true
    property bool summaryEnabled: true
    property alias animating: numberAnimation.running
    readonly property int _padding: 5
    property Item currentItem:
    {
        return listTabs[currentIndex];
    }

    modality: Qt.ApplicationModal

    //FIXME Set these based on the content tabs
    //minimumWidth: 640
    //minimumHeight: 480

    onWidthChanged: { content.x = currentIndex * -contentContainer.width }
    onHeightChanged: { content.x = currentIndex * -contentContainer.width }

    SystemPalette { id: systemPalette }

    ColumnLayout
    {
        id: containerLayout
        Layout.fillWidth: true
        Layout.fillHeight: true
        anchors.fill: parent
        anchors.margins: Constants.margin
        RowLayout
        {
            anchors.margins: Constants.margin
            Layout.fillHeight: true

            Rectangle
            {
                Layout.fillHeight: true
                Layout.preferredWidth: width
                width:
                {
                    // Adjust width to match text contents
                    var maxWidth = 100;

                    for(var i = 0; i<listTabs.length; i++)
                    {
                        var delegateItem = tabSelector.contentItem.children[i];
                        if(delegateItem === undefined)
                            continue;

                        if(delegateItem.children.length === 0)
                            continue;

                        maxWidth = Math.max(delegateItem.children[0].children[0].contentWidth + 5,
                                 maxWidth);
                    }
                    return maxWidth;
                }

                border.width: 1
                border.color: systemPalette.mid
                z: -1

                ListView
                {
                    id: tabSelector
                    model: listTabs.length
                    z: 100
                    boundsBehavior: Flickable.StopAtBounds
                    anchors.centerIn: parent
                    anchors.margins: 1
                    anchors.fill: parent

                    delegate: Item
                    {
                        width: parent.width
                        height: delegateRectangle.height
                        Rectangle
                        {
                            id: delegateRectangle
                            color: index == currentIndex ? "lightblue" : "#00000000"
                            anchors.centerIn: parent
                            width: parent.width
                            height: children[0].height + (_padding * 2.0)
                            Text
                            {
                                anchors.margins: _padding
                                anchors.centerIn: parent
                                text: listTabs[index].title
                                z: 5
                            }
                            MouseArea
                            {
                                anchors.fill: parent
                                onClicked: goToTab(index);
                            }
                        }
                    }
                }
            }

            Item
            {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Item
                {
                    id: contentContainer
                    anchors.fill: parent
                    anchors.margins: Constants.margin
                    clip: true
                    Item
                    {
                        id: content
                        height: parent.height
                    }
                }
            }
        }

        RowLayout
        {
            Rectangle
            {
                Layout.fillWidth: true
            }

            Button
            {
                id: previousButton
                text: qsTr("Previous")
                onClicked: { root.previous(); }
                enabled: currentIndex > 0
            }

            Button
            {
                id: nextButton
                text: qsTr("Next")
                onClicked: { root.next(); }
                enabled: (currentIndex < listTabs.length - 1) ? nextEnabled : false
            }

            Button
            {
                id: finishButton
                text: qsTr("Finish")
                onClicked: {
                    if(summaryEnabled && currentIndex !== listTabs.length - 1)
                        goToTab(listTabs.length - 1);
                    else
                        root.accepted();
                }
                enabled: (currentIndex >= enableFinishAtIndex) ? finishEnabled : false
            }

            Button
            {
                text: qsTr("Cancel")
                onClicked: { root.close(); }
            }
        }
    }

    function next()
    {
        if(currentIndex < listTabs.length - 1)
        {
            currentIndex++;
            numberAnimation.running = false;
            numberAnimation.running = true;
        }
    }

    function previous()
    {
        if(currentIndex > 0)
        {
            currentIndex--;
            numberAnimation.running = false;
            numberAnimation.running = true;
        }
    }

    function goToTab(index)
    {
        if(index <= listTabs.length - 1 && index >= 0)
        {
            currentIndex = index;
            numberAnimation.running = false;
            numberAnimation.running = true;
        }
    }

    function indexOf(item)
    {
        for(var i = 0; i < listTabs.length; i++)
        {
            if(listTabs[i] === item)
                return i;
        }

        return -1;
    }

    NumberAnimation
    {
        id: numberAnimation
        target: content
        properties: "x"
        to: currentIndex * -(contentContainer.width)
        easing.type: Easing.OutQuad
    }

    Component.onCompleted:
    {
        for(var i = 0; i < listTabs.length; i++)
        {
            listTabs[i].parent = content;
            listTabs[i].x = Qt.binding(function() {
                return (indexOf(this) * contentContainer.width + tabSelector.implicitWidth)  });
            listTabs[i].width = Qt.binding(function() { return (contentContainer.width); });
            listTabs[i].height = Qt.binding(function() { return contentContainer.height; });
        }
    }

    onAccepted:
    {
        close();
    }
}
