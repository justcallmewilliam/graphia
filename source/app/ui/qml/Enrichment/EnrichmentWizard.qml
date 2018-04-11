import QtQuick 2.2
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.1


import com.kajeka 1.0
import "../Controls"
import "../"

Wizard
{
    id: root
    minimumWidth: 640
    minimumHeight: 400

    // Must be set before opening
    property var attributeGroups

    property string selectedAttributeGroupA: ""
    property string selectedAttributeGroupB: ""

    finishEnabled: attributesSelected && (attributeSelectedBExclusiveGroup.current != null)

    function reset()
    {
        // Reset on finish
        goToPage(0);
        for(var i=0; i < attributeSelectARepeater.count; i++)
            attributeSelectARepeater.itemAt(i).checked = false;
    }

    Item
    {
        ColumnLayout
        {
            anchors.left: parent.left
            anchors.right: parent.right
            Text
            {
                text: qsTr("<h2>Enrichment</h2>")
                Layout.alignment: Qt.AlignLeft
                textFormat: Text.StyledText
            }

            RowLayout
            {
                ColumnLayout
                {
                    Text
                    {
                        // Should expand this later
                        text: qsTr("Enrichment identifies the significance of a group makeup versus the null hypothesis<br>" +
                                   "<br>" +
                                   "Two attribute groups will be selected to test for enrichment" +
                                   "<br>" +
                                   "The edges may be filtered using transforms once the graph has been created.")
                        wrapMode: Text.WordWrap
                        textFormat: Text.StyledText
                        Layout.fillWidth: true
                    }
                }

                Image
                {
                    anchors.top: parent.top
                    Layout.minimumWidth: 100
                    Layout.minimumHeight: 100
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: 100
                    source: "qrc:///imagery/bellcurve.svg"
                }
            }
        }
    }


    Item
    {
        ColumnLayout
        {
            anchors.fill: parent

            Text
            {
                text: qsTr("<h2>Enrichment Attribute - A</h2>")
                Layout.alignment: Qt.AlignLeft
                textFormat: Text.StyledText
            }

            ColumnLayout
            {
                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 20

                Text
                {
                    text: qsTr("Please select the first attribute group to test for "+
                               "enrichment")
                    Layout.alignment: Qt.AlignLeft
                    textFormat: Text.StyledText
                    Layout.fillWidth: true
                }

                ScrollView
                {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ColumnLayout
                    {
                        ExclusiveGroup
                        {
                            id: attributeSelectedAExclusiveGroup
                            onCurrentChanged:
                            {
                                selectedAttributeGroupA = current.text;
                                // Disable analysis on selected
                                for(var i=0; i<attributeSelectedBRepeater.count; i++)
                                {
                                    var radioBtn = attributeSelectedBRepeater.itemAt(i);
                                    radioBtn.enabled = radioBtn.text !== current.text;
                                }
                            }
                        }

                        Repeater
                        {
                            id: attributeSelectARepeater
                            model: attributeGroups
                            RadioButton
                            {
                                text: modelData
                                exclusiveGroup: attributeSelectedAExclusiveGroup
                            }
                        }
                    }
                }
            }
        }
    }
    Item
    {
        Layout.fillHeight: true
        ColumnLayout
        {
            anchors.fill: parent

            Text
            {
                text: qsTr("<h2>Enrichment Attribute - B</h2>")
                Layout.alignment: Qt.AlignLeft
                textFormat: Text.StyledText
            }

            ColumnLayout
            {
                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 20

                Text
                {
                    text: qsTr("Please select second attribute group to test for "+
                               "enrichment")
                    Layout.alignment: Qt.AlignLeft
                    textFormat: Text.StyledText
                    Layout.fillWidth: true
                }

                ScrollView
                {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ColumnLayout
                    {
                        ExclusiveGroup
                        {
                            id: attributeSelectedBExclusiveGroup
                            onCurrentChanged:
                            {
                                selectedAttributeGroupB = current.text;
                            }
                        }

                        Repeater
                        {
                            id: attributeSelectedBRepeater
                            model: attributeGroups
                            RadioButton
                            {
                                text: modelData
                                exclusiveGroup: attributeSelectedBExclusiveGroup
                            }
                        }
                    }
                }
            }
        }
    }
}
