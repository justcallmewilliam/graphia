import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

import com.kajeka 1.0

Item
{
    id: root

    default property var content
    readonly property int _padding: 10
    property alias color: backRectangle.color

    property var title
    property var target
    property int alignment: Qt.AlignLeft
    property bool displayNext: false
    property bool displayClose: false
    property bool tooltipMode: false
    property Item _mouseCapture

    height: backRectangle.height
    width: backRectangle.width

    signal nextClicked()
    signal closeClicked()
    signal skipClicked()

    visible: false

    Preferences
    {
        id: misc
        section: "misc"
        property bool disableHubbles
    }

    opacity: 0.0
    z: 99

    onContentChanged: { content.parent = containerLayout; }
    onTargetChanged:
    {
        unlinkFromTarget();
        linkToTarget();
    }

    Behavior on opacity
    {
        PropertyAnimation
        {
            duration: 100
        }
    }

    onVisibleChanged:
    {
        if(misc.disableHubbles && tooltipMode)
            visible = false;
        if(visible)
        {
            if(target)
                positionBubble();

            opacity = 1.0;
        }
        else
            opacity = 0.0;
    }

    Timer
    {
        id: hoverTimer
        interval: 750
        onTriggered: root.visible = true
    }

    Rectangle
    {
        id: backRectangle
        color: Qt.rgba(0.96, 0.96, 0.96, 0.9)
        width: fullHubbleLayout.width + _padding
        height: fullHubbleLayout.height + _padding
        radius: 3

        ColumnLayout
        {
            id: fullHubbleLayout
            anchors.verticalCenter: backRectangle.verticalCenter
            anchors.horizontalCenter: backRectangle.horizontalCenter
            ColumnLayout
            {
                id: containerLayout

                Text
                {
                    text: title
                    font.pointSize: 15
                }
            }
            RowLayout
            {
                visible: displayNext || displayClose
                id: nextSkipButtons
                Text
                {
                    visible: displayNext
                    text: qsTr("Skip")
                    font.underline: true
                    MouseArea
                    {
                        cursorShape: Qt.PointingHandCursor
                        anchors.fill: parent
                        onClicked: skipClicked();
                    }
                }
                Rectangle
                {
                    Layout.fillWidth: true
                }
                Button
                {
                    visible: displayNext
                    id: nextButton
                    text: qsTr("Next")
                    onClicked:
                    {
                        nextClicked();
                    }
                }
                Button
                {
                    id: closeButton
                    visible: displayClose
                    text: qsTr("Close")
                    onClicked:
                    {
                        closeClicked();
                    }
                }
            }
        }
    }

    Component.onCompleted:
    {
        linkToTarget();
    }

    function replaceInParent(replacee, replacer)
    {
        var parent = replacee.parent;

        if(parent === null || parent === undefined)
        {
            console.log("replacee has no parent, giving up");
            return;
        }

        var tail = [];
        for(var index = 0; index < parent.children.length; index++)
        {
            var child = parent.children[index];
            if(child === replacee)
            {
                // Make the replacee an orphan
                child.parent = null;

                // Move all the remaining children to an array
                while(index < parent.children.length)
                {
                    child = parent.children[index];
                    tail.push(child);
                    child.parent = null;
                }

                break;
            }
        }

        if(replacee.parent === null)
        {
            // Parent the replacer
            replacer.parent = parent;

            // Reattach the original children
            tail.forEach(function(child)
            {
                child.parent = parent;
            });
        }
    }

    function unlinkFromTarget()
    {
        // Remove the mouse capture shim
        // Heirarchy was parent->mouseCapture->target
        // Remove mouseCapture and reparent the child items to parent
        // Results in parent->item
        if(_mouseCapture !== null && _mouseCapture.children.length > 0)
        {
            if(_mouseCapture.children.length > 1)
                console.log("HoverMousePassthrough has more than one child; it shouldn't");

            replaceInParent(_mouseCapture, _mouseCapture.children[0]);
        }
    }

    function linkToTarget()
    {
        if(target !== undefined && target !== null)
        {
            if(tooltipMode)
            {
                // Use target's hoverChanged signal
                if(target.hoveredChanged !== undefined)
                {
                    target.hoveredChanged.connect(onHover);
                }
                else
                {
                    // If the target doesn't have a hover signal, we'll
                    // shim it with a HoverMousePassthrough item
                    if(_mouseCapture === undefined || _mouseCapture === null)
                    {
                        _mouseCapture = Qt.createQmlObject("import QtQuick 2.0; import com.kajeka 1.0; " +
                            "HoverMousePassthrough {}",
                            // root as parent is temporary, until it gets set for real below
                            root);
                    }

                    // Insert the mouse capture shim
                    // Heirarchy was parent->target
                    // Add mouseCapture below parent and reparent the child items to mousecapture
                    // results in: parent->mouseCapture->target
                    // This allows mouseCapture to access all mouse hover events!
                    if(target.parent !== _mouseCapture)
                    {
                        // When the target item's visibility is false, our shim should also be invisible,
                        // but we can't change its visible property because that would in turn explicity set the
                        // child's visible property and obviously we don't want that. Instead therefore, we fake
                        // visibility by making the shim's size very small. Unforunately, we can't use 0 for the
                        // size since it means "invalid" in the context of implicit size.
                        //
                        // FIXME: This is a bit of a crap solution really, because it means that:
                        //        a) the shim is never truly invisible, and at best will be a single pixel big
                        //        b) in the case of the item being in a layout, it will potentially add the
                        //           layout's spacing parameter to the sides of said single pixel when really
                        //           the entire thing should be occupying zero space
                        _mouseCapture.implicitWidth = Qt.binding(function()
                            { return target.visible ? target.implicitWidth : 1; });
                        _mouseCapture.implicitHeight = Qt.binding(function()
                            { return target.visible ? target.implicitHeight : 1; });

                        // If we can't see the target, disable the hubble
                        _mouseCapture.enabled = Qt.binding(function() { return target.visible; });

                        replaceInParent(target, _mouseCapture);
                        target.parent = _mouseCapture;

                        _mouseCapture.hoveredChanged.connect(onHover);
                    }
                }
            }
            else
            {
                root.parent = mainWindow.toolBar;
                positionBubble();
            }
        }
    }

    function onHover()
    {
        var hoverTarget = _mouseCapture !== null ? _mouseCapture : target;
        if(hoverTarget.hovered)
        {
            hoverTimer.start();
            root.parent = mainWindow.toolBar;
            positionBubble();
        }
        else
        {
            hoverTimer.stop();
            root.visible = false;
        }
    }

    function positionBubble()
    {
        var point = {};

        switch(alignment)
        {
        case Qt.AlignLeft:
            point = target.mapToItem(parent, 0, target.height * 0.5);
            root.x = point.x - childrenRect.width - _padding;
            root.y = point.y - (childrenRect.height * 0.5);
            break;
        case Qt.AlignRight:
            point = target.mapToItem(parent, target.width, target.height * 0.5);
            root.x = point.x + _padding
            root.y = point.y - (childrenRect.height * 0.5);
            break;
        case Qt.AlignTop:
            point = target.mapToItem(parent, target.width * 0.5, 0);
            root.x = point.x - (childrenRect.width * 0.5);
            root.y = point.y - childrenRect.height - _padding;
            break;
        case Qt.AlignBottom:
            point = target.mapToItem(parent, target.width * 0.5, target.height);
            root.x = point.x - (childrenRect.width * 0.5);
            root.y = point.y + _padding;
            break;
        case Qt.AlignLeft | Qt.AlignTop:
            point = target.mapToItem(parent, 0, 0);
            root.x = point.x - childrenRect.width - _padding;
            root.y = point.y - childrenRect.height - _padding;
            break;
        case Qt.AlignRight | Qt.AlignTop:
            point = target.mapToItem(parent, target.width, 0);
            root.x = point.x + _padding
            root.y = point.y - _padding;
            break;
        case Qt.AlignLeft | Qt.AlignBottom:
            point = target.mapToItem(parent, 0, target.height);
            root.x = point.x - childrenRect.width - _padding;
            root.y = point.y + _padding;
            break;
        case Qt.AlignRight | Qt.AlignBottom:
            point = target.mapToItem(parent, target.width, target.height);
            root.x = point.x + _padding;
            root.y = point.y + _padding;
            break;
        }
    }
}