#include "louvaintransform.h"

#include "transform/transformedgraph.h"

#include "shared/graph/grapharray.h"
#include "shared/utils/container.h"

#include "graph/graphmodel.h"

#include <map>
#include <vector>
#include <algorithm>
#include <cmath>

// https://arxiv.org/abs/0803.0476

void LouvainTransform::apply(TransformedGraph& target) const
{
    auto resolution = 1.0 - std::get<double>(
        config().parameterByName(QStringLiteral("Granularity"))->_value);

    const auto minResolution = 0.5;
    const auto maxResolution = 30.0;

    const auto logMin = std::log10(minResolution);
    const auto logMax = std::log10(maxResolution);
    const auto logRange = logMax - logMin;

    resolution = std::pow(10.0f, logMin + (resolution * logRange));

    using CommunityId = NodeId; // Slight Hack, be careful with this

    const auto& edgeIds = target.edgeIds();
    EdgeArray<double> weights(target, 1.0);

    if(_weighted)
    {
        if(config().attributeNames().empty())
        {
            addAlert(AlertType::Error, QObject::tr("Invalid parameter"));
            return;
        }

        auto attribute = _graphModel->attributeValueByName(
            config().attributeNames().front());

        for(auto edgeId : edgeIds)
            weights[edgeId] = attribute.numericValueOf(edgeId);
    }

    double totalWeight = std::accumulate(edgeIds.begin(), edgeIds.end(), 0.0,
    [&weights](double d, EdgeId edgeId)
    {
        return d + weights[edgeId];
    });

    std::vector<NodeArray<CommunityId>> iterations;
    size_t progressIteration = 1;
    target.setPhase(QStringLiteral("Louvain Initialising"));

    NodeArray<CommunityId> communities(target);
    NodeArray<double> weightedDegrees(target);
    std::map<CommunityId, int> communityDegrees;

    auto add = [&](CommunityId community, NodeId nodeId)
    {
        communities[nodeId] = community;
        communityDegrees[community] += weightedDegrees[nodeId];
    };

    auto remove = [&](CommunityId community, NodeId nodeId)
    {
        communities[nodeId].setToNull();
        communityDegrees[community] -= weightedDegrees[nodeId];
    };

    auto relabel = [&](MutableGraph& graph)
    {
        std::map<CommunityId, CommunityId> idMap;
        CommunityId nextCommunityId = 0;

        for(auto nodeId : graph.nodeIds())
        {
            if(graph.typeOf(nodeId) == MultiElementType::Tail)
                continue;

            auto oldCommunityId = communities[nodeId];

            if(!u::contains(idMap, oldCommunityId))
                communities[nodeId] = idMap[oldCommunityId] = nextCommunityId++;
            else
                communities[nodeId] = idMap.at(oldCommunityId);
        }
    };

    auto coarsen = [&](MutableGraph& graph)
    {
        MutableGraph coarseGraph;
        EdgeArray<double> newWeights(target, 0.0);

        coarseGraph.performTransaction([&](IMutableGraph&)
        {
            // Create a node for each community
            for(auto nodeId : graph.nodeIds())
            {
                if(graph.typeOf(nodeId) == MultiElementType::Tail)
                    continue;

                auto newNodeId = communities[nodeId];

                if(coarseGraph.containsNodeId(newNodeId))
                    continue;

                coarseGraph.reserveNodeId(newNodeId);
                auto assignedNodeId = coarseGraph.addNode(newNodeId);
                Q_ASSERT(assignedNodeId == newNodeId);
            }

            uint64_t edgeIndex = 0;

            // Create an edge between community nodes for each
            // pair of connected communities in the base graph
            for(auto edgeId : graph.edgeIds())
            {
                target.setProgress(static_cast<int>((edgeIndex++ * 100) / graph.numEdges()));

                if(cancelled())
                    break;

                const auto& edge = graph.edgeById(edgeId);
                auto sourceId = communities.at(edge.sourceId());
                auto targetId = communities.at(edge.targetId());
                double newWeight = weights[edgeId];

                auto newEdgeId = coarseGraph.firstEdgeIdBetween(sourceId, targetId);
                if(newEdgeId.isNull())
                    newEdgeId = coarseGraph.addEdge(sourceId, targetId);

                newWeights[newEdgeId] += newWeight;
            }
        });

        graph = coarseGraph;
        weights = newWeights;
    };

    auto moveNodes = [&](MutableGraph& graph)
    {
        if(cancelled())
            return false;

        CommunityId nextCommunityId = 0;
        for(auto nodeId : graph.nodeIds())
        {
            if(graph.typeOf(nodeId) == MultiElementType::Tail)
                continue;

            for(auto edgeId : graph.nodeById(nodeId).edgeIds())
                weightedDegrees[nodeId] += weights[edgeId];

            add(nextCommunityId++, nodeId);
        }

        size_t subProgressIteration = 1;
        bool modified = false;
        bool improved = false;
        do
        {
            improved = false;
            target.setProgress(0);
            uint64_t nodeIndex = 0;

            target.setPhase(QStringLiteral("Louvain Iteration %1.%2")
                .arg(QString::number(progressIteration), QString::number(subProgressIteration++)));

            for(auto nodeId : graph.nodeIds())
            {
                target.setProgress(static_cast<int>((nodeIndex++ * 100) / graph.numNodes()));

                if(graph.typeOf(nodeId) == MultiElementType::Tail)
                    continue;

                std::map<CommunityId, double> neighbourCommunityWeights;
                for(auto edgeId : graph.nodeById(nodeId).edgeIds())
                {
                    auto neighbourNodeId = graph.edgeById(edgeId).oppositeId(nodeId);

                    // Skip loop edges
                    if(neighbourNodeId == nodeId)
                        continue;

                    if(graph.typeOf(neighbourNodeId) == MultiElementType::Tail)
                        continue;

                    auto neighbourCommunityId = communities.at(neighbourNodeId);

                    neighbourCommunityWeights[neighbourCommunityId] += weights[edgeId];
                }

                auto communityId = communities[nodeId];
                remove(communityId, nodeId);

                double maxDeltaQ = 0.0;
                auto newCommunityId = communityId;

                for(auto [neighbourCommunityId, weight] : neighbourCommunityWeights)
                {
                    auto communityWeight = communityDegrees.at(neighbourCommunityId);
                    auto nodeWeight = weightedDegrees[nodeId];

                    auto deltaQ = (resolution * weight) -
                        ((communityWeight * nodeWeight) / totalWeight);

                    if(deltaQ > maxDeltaQ)
                    {
                        maxDeltaQ = deltaQ;
                        newCommunityId = neighbourCommunityId;
                    }
                }

                add(newCommunityId, nodeId);

                if(newCommunityId != communityId)
                    improved = modified = true;

                if(cancelled())
                    break;
            }

            target.setProgress(-1);
        }
        while(improved && !cancelled());

        return modified;
    };

    MutableGraph graph(target.mutableGraph());

    bool finished = false;
    do
    {
        target.setProgress(-1);

        communities.resetElements();
        weightedDegrees.resetElements();
        communityDegrees.clear();

        finished = !moveNodes(graph);

        if(!finished && !cancelled())
        {
            relabel(graph);
            iterations.emplace_back(communities);

            target.setPhase(QStringLiteral("Louvain Iteration %1 Coarsening")
                .arg(QString::number(progressIteration)));
            coarsen(graph);
        }

        progressIteration++;
    }
    while(!finished && !cancelled());

    if(cancelled())
        return;

    target.setPhase(QStringLiteral("Louvain Finalising"));

    // Set each CommunityId to be the same as the NodeId, initially
    for(auto nodeId : target.nodeIds())
    {
        if(graph.typeOf(nodeId) == MultiElementType::Tail)
            continue;

        communities[nodeId] = nodeId;
    }

    // Walk back over our iterations to build the final communities
    for(auto nodeId : target.nodeIds())
    {
        for(const auto& iteration : iterations)
        {
            auto communityId = communities[nodeId];

            if(!communityId.isNull())
                communities[nodeId] = iteration.at(communities[nodeId]);

            if(cancelled())
                return;
        }
    }

    // Sort communities by size
    std::map<CommunityId, size_t> communityHistogram;
    for(auto nodeId : target.nodeIds())
        communityHistogram[communities[nodeId]]++;
    std::vector<std::pair<CommunityId, size_t>> sortedCommunityHistogram;
    std::copy(communityHistogram.begin(), communityHistogram.end(),
        std::back_inserter(sortedCommunityHistogram));
    std::sort(sortedCommunityHistogram.begin(), sortedCommunityHistogram.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // Assign cluster numbers to each community
    std::map<CommunityId, size_t> clusterNumbers;
    size_t clusterNumber = 1;
    for(auto [communityId, size] : sortedCommunityHistogram)
    {
        if(!communityId.isNull())
            clusterNumbers[communityId] = clusterNumber++;
    }

    NodeArray<QString> clusterNames(target);

    for(auto nodeId : target.nodeIds())
    {
        auto communityId = communities[nodeId];
        if(communityId.isNull())
            continue;

        clusterNumber = clusterNumbers[communityId];
        clusterNames[nodeId] = QObject::tr("Cluster %1").arg(clusterNumber);
    }

    _graphModel->createAttribute(QObject::tr(_weighted ? "Weighted Louvain Cluster" : "Louvain Cluster"))
        .setDescription(QObject::tr("The Louvain-calculated cluster in which the node resides."))
        .setStringValueFn([clusterNames](NodeId nodeId) { return clusterNames[nodeId]; })
        .setValueMissingFn([clusterNames](NodeId nodeId) { return clusterNames[nodeId].isEmpty(); })
        .setFlag(AttributeFlag::FindShared)
        .setFlag(AttributeFlag::Searchable);
}