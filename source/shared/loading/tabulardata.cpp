#include "tabulardata.h"

TabularData::TabularData(TabularData&& other) noexcept :
    _data(std::move(other._data)),
    _columns(other._columns),
    _rows(other._rows),
    _transposed(other._transposed)

{
    other.reset();
}

TabularData& TabularData::operator=(TabularData&& other) noexcept
{
    if(this != &other)
    {
        _data = std::move(other._data);
        _columns = other._columns;
        _rows = other._rows;
        _transposed = other._transposed;

        other.reset();
    }

    return *this;
}

void TabularData::reserve(size_t columns, size_t rows)
{
    _data.reserve(columns * rows);
}

bool TabularData::empty() const
{
    return _data.empty();
}

size_t TabularData::index(size_t column, size_t row) const
{
    Q_ASSERT(column < numColumns());
    Q_ASSERT(row < numRows());

    auto index = !_transposed ?
        column + (row * _columns) :
        row + (column * _columns);

    return index;
}

size_t TabularData::numColumns() const
{
    return !_transposed ? _columns : _rows;
}

size_t TabularData::numRows() const
{
    return !_transposed ? _rows : _columns;
}

void TabularData::setValueAt(size_t column, size_t row, QString&& value, int progressHint)
{
    size_t columns = column >= _columns ? column + 1 : _columns;
    size_t rows = row >= _rows ? row + 1 : _rows;
    auto newSize = columns * rows;

    // If the column count is increasing, jiggle all the existing rows around,
    // taking into account the new row width
    if(_rows > 0 && rows > 1 && columns > _columns)
    {
        _data.resize(newSize);

        for(size_t offset = _rows - 1; offset > 0; offset--)
        {
            auto oldPosition = _data.begin() + (offset * _columns);
            auto newPosition = _data.begin() + (offset * columns);

            std::move_backward(oldPosition,
                oldPosition + _columns,
                newPosition + _columns);
        }
    }

    _columns = columns;
    _rows = rows;

    if(newSize > _data.capacity())
    {
        size_t reserveSize = newSize;

        if(progressHint >= 10)
        {
            // If we've made it some significant way through the input, we can be
            // reasonably confident of the total memory requirement...

            // We over-allocate ever so slightly to avoid the case where our estimate
            // is on the small side. Otherwise, when we hit 100, we would default to
            // reallocating for each new element -- exactly what we're trying to avoid
            const auto extraFudgeFactor = 2;
            auto estimate = ((100 + extraFudgeFactor) * newSize) / progressHint;

            reserveSize = std::max(reserveSize, estimate);
        }
        else
        {
            // ...otherwise just double the reservation each time we need more space
            reserveSize = newSize * 2;
        }

        _data.reserve(reserveSize);
    }

    _data.resize(newSize);
    _data.at(index(column, row)) = value.trimmed();
}

void TabularData::shrinkToFit()
{
    _data.shrink_to_fit();
}

void TabularData::reset()
{
    _data.clear();
    _columns = 0;
    _rows = 0;
    _transposed = false;
}

const QString& TabularData::valueAt(size_t column, size_t row) const
{
    return _data.at(index(column, row));
}
