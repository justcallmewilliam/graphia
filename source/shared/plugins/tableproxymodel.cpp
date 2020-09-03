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

#include "tableproxymodel.h"
#include "nodeattributetablemodel.h"

bool TableProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    return sourceModel()->data(sourceModel()->index(sourceRow, 0, sourceParent),
                               NodeAttributeTableModel::Roles::NodeSelectedRole).toBool();
}

bool TableProxyModel::filterAcceptsColumn(int sourceColumn, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent)
    return !u::contains(_hiddenColumns, sourceColumn);
}

QVariant TableProxyModel::data(const QModelIndex &index, int role) const
{
    if(role == SubSelectedRole)
        return _subSelectionRows.find(index.row()) != _subSelectionRows.end();

    auto unorderedSourceIndex = mapToSource(index);

    if(_orderedProxyToSourceColumn.size() == static_cast<size_t>(columnCount()))
    {
        auto mappedIndex = sourceModel()->index(unorderedSourceIndex.row(),
                                                _orderedProxyToSourceColumn.at(index.column()));
        return sourceModel()->data(mappedIndex, role);
    }

    auto sourceIndex = sourceModel()->index(unorderedSourceIndex.row(), unorderedSourceIndex.column());
    Q_ASSERT(index.isValid() && sourceIndex.isValid());

    return sourceModel()->data(sourceIndex, role);
}

void TableProxyModel::setSubSelection(const QItemSelection& subSelection, const QItemSelection& subDeSelection)
{
    _subSelection = subSelection;

    // Group selection by rows, no need to keep track of the indices for the model other than
    // emitting signals.
    _subSelectionRows.clear();
    for(auto index : _subSelection.indexes())
        _subSelectionRows.insert(index.row());

    for(const auto& range : _subSelection)
        emit dataChanged(range.topLeft(), range.bottomRight(), { Roles::SubSelectedRole });
    for(const auto& range : subDeSelection)
        emit dataChanged(range.topLeft(), range.bottomRight(), { Roles::SubSelectedRole });
}

QItemSelectionRange TableProxyModel::buildRowSelectionRange(int topRow, int bottomRow)
{
    return QItemSelectionRange(index(topRow, 0), index(bottomRow, columnCount() - 1));
}

int TableProxyModel::mapToSourceRow(int proxyRow) const
{
    QModelIndex proxyIndex = index(proxyRow, 0);
    QModelIndex sourceIndex = mapToSource(proxyIndex);
    return sourceIndex.isValid() ? sourceIndex.row() : -1;
}

int TableProxyModel::mapOrderedToSourceColumn(int proxyColumn) const
{
    if(proxyColumn >= columnCount())
        return -1;

    if(static_cast<int>(_orderedProxyToSourceColumn.size()) != columnCount())
        return -1;

    auto mappedProxyColumn = proxyColumn;
    if(!_orderedProxyToSourceColumn.empty())
        mappedProxyColumn = _orderedProxyToSourceColumn.at(static_cast<size_t>(proxyColumn));

    return mappedProxyColumn;
}

TableProxyModel::TableProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    connect(this, &QAbstractProxyModel::sourceModelChanged, this, &TableProxyModel::updateSourceModelFilter);
    _collator.setNumericMode(true);
}

void TableProxyModel::setHiddenColumns(std::vector<int> hiddenColumns)
{
    std::sort(hiddenColumns.begin(), hiddenColumns.end());
    _hiddenColumns = hiddenColumns;
    invalidateFilter();
    calculateOrderedProxySourceMapping();
}

void TableProxyModel::calculateUnorderedSourceProxyColumnMapping()
{
    auto sourceColumnCount = static_cast<size_t>(sourceModel()->columnCount());
    std::vector<int> proxyToSourceColumns;
    std::vector<int> sourceToProxyColumns(sourceColumnCount, -1);
    proxyToSourceColumns.reserve(sourceColumnCount);
    for(auto i = 0; i < static_cast<int>(sourceColumnCount); ++i)
    {
        if(filterAcceptsColumn(i, {}))
            proxyToSourceColumns.push_back(i);
    }

    for(auto i = 0; i < columnCount(); ++i)
    {
        auto index = static_cast<size_t>(i);
        auto source = static_cast<size_t>(proxyToSourceColumns.at(index));
        sourceToProxyColumns[source] = i;
    }

    _unorderedSourceToProxyColumn = sourceToProxyColumns;
}

void TableProxyModel::calculateOrderedProxySourceMapping()
{
    if(_sourceColumnOrder.size() != static_cast<size_t>(sourceModel()->columnCount()))
    {
        // If ordering doesn't match the sourcemodel size just destroy it
        _sourceColumnOrder = std::vector<int>(static_cast<size_t>(sourceModel()->columnCount()));
        std::iota(_sourceColumnOrder.begin(), _sourceColumnOrder.end(), 0);
    }

    auto filteredOrder = u::setDifference(_sourceColumnOrder, _hiddenColumns);
    _orderedProxyToSourceColumn = filteredOrder;

    _headerModel.clear();
    _headerModel.setRowCount(1);
    _headerModel.setColumnCount(columnCount());
    // Headermodel takes ownership of the Items
    for(int i = 0; i < columnCount(); ++i)
        _headerModel.setItem(0, i, new QStandardItem());

    emit columnOrderChanged();
    emit layoutChanged();
}

void TableProxyModel::setColumnOrder(const std::vector<int>& columnOrder)
{
    _sourceColumnOrder = columnOrder;
    invalidateFilter();
}

int TableProxyModel::sortColumn_() const
{
    if(_sortColumnAndOrders.empty())
        return -1;

    return _sortColumnAndOrders.front().first;
}

void TableProxyModel::setSortColumn(int newSortColumn)
{
    if(newSortColumn < 0 || newSortColumn == sortColumn_())
        return;

    auto currentSortOrder = Qt::AscendingOrder;

    auto existing = std::find_if(_sortColumnAndOrders.begin(), _sortColumnAndOrders.end(),
    [newSortColumn](const auto& value)
    {
        return value.first == newSortColumn;
    });

    // If the column has been sorted on before, remove it so
    // that adding it brings it to the front
    if(existing != _sortColumnAndOrders.end())
    {
        currentSortOrder = existing->second;
        _sortColumnAndOrders.erase(existing);
    }

    _sortColumnAndOrders.emplace_front(newSortColumn, currentSortOrder);

    resort();
    emit sortColumnChanged(newSortColumn);
    emit sortOrderChanged(currentSortOrder);
}

Qt::SortOrder TableProxyModel::sortOrder_() const
{
    if(_sortColumnAndOrders.empty())
        return Qt::DescendingOrder;

    return _sortColumnAndOrders.front().second;
}

void TableProxyModel::setSortOrder(Qt::SortOrder newSortOrder)
{
    if(_sortColumnAndOrders.empty() || newSortOrder == sortOrder_())
        return;

    _sortColumnAndOrders.front().second = newSortOrder;

    resort();
    emit sortOrderChanged(newSortOrder);
}

void TableProxyModel::invalidateFilter()
{
    beginResetModel();
    QSortFilterProxyModel::invalidate();
    QSortFilterProxyModel::invalidateFilter();

    calculateOrderedProxySourceMapping();
    calculateUnorderedSourceProxyColumnMapping();
    endResetModel();
}

void TableProxyModel::updateSourceModelFilter()
{
    if(sourceModel() == nullptr)
        return;

    connect(sourceModel(), &QAbstractItemModel::modelReset, this, &TableProxyModel::invalidateFilter);
    connect(sourceModel(), &QAbstractItemModel::layoutChanged, this, &TableProxyModel::invalidateFilter);
}

void TableProxyModel::resort()
{
    invalidate();

    // The parameters to this don't really matter, because the actual ordering is determined
    // by the implementation of lessThan, in combination with the contents of _sortColumnAndOrders
    sort(0);
}

bool TableProxyModel::lessThan(const QModelIndex& a, const QModelIndex& b) const
{
    auto rowA = a.row();
    auto rowB = b.row();

    for(const auto& sortColumnAndOrder : _sortColumnAndOrders)
    {
        auto column = _unorderedSourceToProxyColumn.at(sortColumnAndOrder.first);
        auto order = sortColumnAndOrder.second;

        auto indexA = sourceModel()->index(rowA, column);
        auto indexB = sourceModel()->index(rowB, column);

        auto valueA = sourceModel()->data(indexA, Qt::DisplayRole);
        auto valueB = sourceModel()->data(indexB, Qt::DisplayRole);

        if(valueA == valueB)
            continue;

        if(static_cast<QMetaType::Type>(valueA.type()) == QMetaType::QString &&
            static_cast<QMetaType::Type>(valueB.type()) == QMetaType::QString)
        {
            return order == Qt::DescendingOrder ?
                _collator.compare(valueB.toString(), valueA.toString()) < 0 :
                _collator.compare(valueA.toString(), valueB.toString()) < 0;
        }

        return order == Qt::DescendingOrder ?
            QSortFilterProxyModel::lessThan(indexB, indexA) :
            QSortFilterProxyModel::lessThan(indexA, indexB);
    }

    return false;
}
