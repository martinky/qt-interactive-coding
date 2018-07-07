#include <QLabel>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <qicentry.h>
#include <qiccontext.h>

QImage paint()
{
    QImage img(160, 160, QImage::Format_RGB32);
    QPainter p(&img);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 200, 100));
    p.drawRect(0, 0, 160, 160);

    p.setBrush(QColor(200, 0, 0));
    //p.drawRect(40, 40, 20, 20);
    //p.drawRect(100, 40, 20, 20);
    //p.drawRect(40, 100, 80, 20);

    return img;
}

extern "C" QIC_ENTRY_EXPORT void qic_entry(qicContext *ctx)
{
    //
    // Get the application widget and paint something on it.
    //
    QLabel *const label = static_cast<QLabel*>(ctx->get("label"));
    label->setPixmap(QPixmap::fromImage(paint()));
}
