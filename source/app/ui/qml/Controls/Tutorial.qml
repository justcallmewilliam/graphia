import QtQuick 2.0

import com.kajeka 1.0

Item
{
    property bool activated: false

    property int visibleHubbleId: -1
    property Item currentHubble:
    {
        if(visibleHubbleId < 0 || visibleHubbleId >= hubbles.length)
            return null;

        return hubbles[visibleHubbleId];
    }

    property bool _hasPreviousHubble:
    {
        return visibleHubbleId > 0;
    }

    property bool _hasNextHubble:
    {
        return visibleHubbleId < hubbles.length - 1;
    }

    Preferences
    {
        id: misc
        section: "misc"
        property bool disableHubbles
    }

    // We unconditionally disable tooltip style Hubbles for the duration of
    // the tutorial, but need to track the state of the user preference so
    // it can be restored to the original value once the tutorial is closed
    property bool _tooltipHubbleOriginalDisableState: false

    default property list<Item> hubbles

    onHubblesChanged:
    {
        initialise();
    }

    Component.onCompleted:
    {
        initialise();
    }

    function reset()
    {
        closeCurrentHubble();
        visibleHubbleId = -1;

        misc.disableHubbles = _tooltipHubbleOriginalDisableState;
    }

    function start()
    {
        _tooltipHubbleOriginalDisableState = misc.disableHubbles;
        reset();
        misc.disableHubbles = true;
        activated = true;

        gotoNextHubble();
    }

    function initialise()
    {
        reset();

        for(var i = 0; i < hubbles.length; i++)
        {
            hubbles[i].parent = parent;
            hubbles[i].opacity = 0.0;
            hubbles[i].visible = false;
        }
    }

    function updateCurrentHubble()
    {
        currentHubble.displayPrevious = _hasPreviousHubble;
        currentHubble.displayNext = _hasNextHubble;
        currentHubble.displayClose = !_hasNextHubble;

        if(currentHubble.displayPrevious)
            currentHubble.previousClicked.connect(gotoPreviousHubble);

        if(currentHubble.displayNext)
            currentHubble.nextClicked.connect(gotoNextHubble);

        if(currentHubble.displayClose)
            currentHubble.closeClicked.connect(reset);

        currentHubble.skipClicked.connect(reset);

        currentHubble.opacity = 1.0;
        currentHubble.visible = true;
    }

    function gotoPreviousHubble()
    {
        closeCurrentHubble();

        if(_hasPreviousHubble)
        {
            visibleHubbleId--;
            updateCurrentHubble();
        }
    }

    function gotoNextHubble()
    {
        closeCurrentHubble();

        if(_hasNextHubble)
        {
            visibleHubbleId++;
            updateCurrentHubble();
        }
    }

    function closeCurrentHubble()
    {
        if(currentHubble !== null)
        {
            currentHubble.previousClicked.disconnect(gotoPreviousHubble);
            currentHubble.nextClicked.disconnect(gotoNextHubble);
            currentHubble.skipClicked.disconnect(closeCurrentHubble);
            currentHubble.opacity = 0.0;
            currentHubble.visible = false;
        }
    }
}
