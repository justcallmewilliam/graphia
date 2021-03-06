/* Copyright © 2013-2020 Graphia Technologies Ltd.
 *
 * This file is part of Graphia.
 *
 * Graphia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Graphia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Graphia.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qcpcolumnannotations.h"

#include "shared/utils/color.h"

QCPColumnAnnotations::QCPColumnAnnotations(QCPAxis* keyAxis, QCPAxis* valueAxis) :
    QCPAbstractPlottable(keyAxis, valueAxis)
{}

double QCPColumnAnnotations::selectTest(const QPointF&, bool, QVariant*) const
{
    // We don't care about selection
    return -1.0;
}

QCPRange QCPColumnAnnotations::getKeyRange(bool& foundRange, QCP::SignDomain) const
{
    foundRange = true;
    return mKeyAxis->range();
}

QCPRange QCPColumnAnnotations::getValueRange(bool& foundRange, QCP::SignDomain, const QCPRange&) const
{
    foundRange = true;
    return mValueAxis->range();
}

void QCPColumnAnnotations::setData(size_t y, std::vector<size_t> indices,
    bool selected, const ColumnAnnotation* columnAnnotation)
{
    _rows.emplace(y, Row(std::move(indices), selected, columnAnnotation));
}

void QCPColumnAnnotations::renderRect(QCPPainter* painter, size_t x, size_t y,
    size_t w, const QString& value, bool selected, std::map<QString, int> valueWidths)
{
    auto xPixel = mKeyAxis->coordToPixel(static_cast<double>(x));
    auto yPixel = mValueAxis->coordToPixel(static_cast<double>(y));

    QRectF rect(xPixel - _halfCellWidth, yPixel - _cellHeight, (w * _cellWidth), _cellHeight);

    // Don't continue if the rectangle is outside the clipping bounds
    if(!rect.intersects(painter->clipBoundingRect()))
        return;

    auto color = u::colorForString(value);

    if(value.isEmpty())
        color = Qt::transparent;
    else if(!selected)
        color = QColor::fromHsl(color.hue(), 20, std::max(color.lightness(), 150));

    painter->setPen(color);
    painter->setBrush(color);
    painter->drawRect(rect);

    if(selected)
    {
        // Always keep the text onscreen if possible
        rect = rect.intersected(painter->clipBoundingRect());

        const auto textMargin = 1.0;
        rect.setLeft(rect.left() + textMargin);
        rect.setRight(rect.right() - textMargin);

        if(static_cast<qreal>(valueWidths.at(value)) <= rect.width())
        {
            auto textColor = u::contrastingColor(color);
            painter->setPen(textColor);

            painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, value, &rect);
        }
    }
}

void QCPColumnAnnotations::draw(QCPPainter* painter)
{
    _cellWidth = mKeyAxis->coordToPixel(1.0) -  mKeyAxis->coordToPixel(0.0);
    _cellHeight = mValueAxis->coordToPixel(0.0) - mValueAxis->coordToPixel(1.0);
    _halfCellWidth = _cellWidth * 0.5;

    auto font = painter->font();
    font.setPixelSize(static_cast<int>(_cellHeight * 0.7));
    painter->setFont(font);

    for(const auto& [y, row] : _rows)
    {
        if(row._indices.empty())
            continue;

        size_t left = 0;
        size_t right = 0;
        size_t width = 0;

        auto currentValue = row._columnAnnotation->valueAt(row._indices.at(0));

        std::map<QString, int> _valueWidths;
        for(const auto& uniqueValue : row._columnAnnotation->uniqueValues())
        {
            const auto& fontMetrics = painter->fontMetrics();
            _valueWidths.emplace(uniqueValue, fontMetrics.boundingRect(uniqueValue).width());
        }

        for(auto index : row._indices)
        {
            const auto& value = row._columnAnnotation->valueAt(index);

            if(value != currentValue)
            {
                renderRect(painter, left, y, width, currentValue, row._selected, _valueWidths);

                left = right;
                currentValue = value;
            }

            right++;
            width = right - left;
        }

        renderRect(painter, left, y, width, currentValue, row._selected, _valueWidths);
    }
}

void QCPColumnAnnotations::drawLegendIcon(QCPPainter*, const QRectF&) const
{
    // Don't need a legend
}
