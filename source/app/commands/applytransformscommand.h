#ifndef APPLYTRANSFORMSCOMMAND_H
#define APPLYTRANSFORMSCOMMAND_H

#include "shared/commands/icommand.h"

#include "shared/graph/elementid.h"
#include "shared/graph/elementid_containers.h"

#include <QStringList>

class GraphModel;
class SelectionManager;
class Document;

class ApplyTransformsCommand : public ICommand
{
private:
    GraphModel* _graphModel = nullptr;
    SelectionManager* _selectionManager = nullptr;
    Document* _document = nullptr;

    QStringList _previousTransformations;
    QStringList _transformations;

    const NodeIdSet _selectedNodeIds;

    void doTransform(const QStringList& transformations,
                     const QStringList& previousTransformations);

public:
    ApplyTransformsCommand(GraphModel* graphModel,
                           SelectionManager* selectionManager,
                           Document* document,
                           QStringList previousTransformations,
                           QStringList transformations);

    QString description() const override;
    QString verb() const override;

    bool execute() override;
    void undo() override;

    void cancel() override;
    bool cancellable() const override { return true; }
};

#endif // APPLYTRANSFORMSCOMMAND_H
