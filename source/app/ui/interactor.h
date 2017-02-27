#ifndef INTERACTOR_H
#define INTERACTOR_H

#include <QObject>

class QMouseEvent;
class QWheelEvent;
class QNativeGestureEvent;
class QKeyEvent;

class Interactor : public QObject
{
    Q_OBJECT

public:
    explicit Interactor(QObject* parent = nullptr) :
        QObject(parent)
    {}

    virtual ~Interactor() = default;

    virtual void mousePressEvent(QMouseEvent*) = 0;
    virtual void mouseReleaseEvent(QMouseEvent*) = 0;
    virtual void mouseMoveEvent(QMouseEvent*) = 0;
    virtual void mouseDoubleClickEvent(QMouseEvent*) = 0;
    virtual void wheelEvent(QWheelEvent*) = 0;
    virtual void nativeGestureEvent(QNativeGestureEvent*) = 0;

    virtual void keyPressEvent(QKeyEvent*) {}
    virtual void keyReleaseEvent(QKeyEvent*) {}

signals:
    void userInteractionStarted() const;
    void userInteractionFinished() const;
};

#endif // INTERACTOR_H
