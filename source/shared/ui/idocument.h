#ifndef IDOCUMENT_H
#define IDOCUMENT_H

#include "shared/graph/elementid.h"

class IGraphModel;
class ISelectionManager;
class ICommandManager;

class IDocument
{
public:
    virtual ~IDocument() = default;

    virtual const IGraphModel* graphModel() const = 0;
    virtual IGraphModel* graphModel() = 0;

    virtual const ISelectionManager* selectionManager() const = 0;
    virtual ISelectionManager* selectionManager() = 0;

    virtual const ICommandManager* commandManager() const = 0;
    virtual ICommandManager* commandManager() = 0;

    virtual void moveFocusToNode(NodeId nodeId) = 0;
};

#endif // IDOCUMENT_H
