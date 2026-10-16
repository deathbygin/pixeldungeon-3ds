#pragma once

#include <vector>
#include <algorithm>

namespace pd {

class Graph {
public:
    template<typename NodePtr>
    static void buildDistanceMap(const std::vector<NodePtr>& nodes, NodePtr focus) {
        const int INF = 1000000;
        for (const auto& n : nodes) {
            n->distance = INF;
        }
        if (!focus) return;
        focus->distance = 0;

        std::vector<NodePtr> queue;
        queue.push_back(focus);

        while (!queue.empty()) {
            auto minIt = queue.begin();
            for (auto it = queue.begin() + 1; it != queue.end(); ++it) {
                if ((*it)->distance < (*minIt)->distance) {
                    minIt = it;
                }
            }
            auto curr = *minIt;
            queue.erase(minIt);

            for (auto* edge : curr->neigbours) {
                int newDist = curr->distance + edge->price;
                if (newDist < edge->distance) {
                    edge->distance = newDist;
                    // Find if edge is already in queue
                    bool inQueue = false;
                    for (const auto& qNode : queue) {
                        if (qNode.get() == edge) {
                            inQueue = true;
                            break;
                        }
                    }
                    if (!inQueue) {
                        // Find NodePtr for edge
                        for (const auto& n : nodes) {
                            if (n.get() == edge) {
                                queue.push_back(n);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    template<typename NodePtr>
    static std::vector<NodePtr> buildPath(
        const std::vector<NodePtr>& nodes,
        NodePtr from,
        NodePtr to
    ) {
        std::vector<NodePtr> path;
        if (!from || !to) return path;

        auto curr = from;
        while (curr != to) {
            NodePtr next = nullptr;
            int minDistance = curr->distance;

            for (auto* edge : curr->neigbours) {
                if (edge->distance < minDistance) {
                    minDistance = edge->distance;
                    for (const auto& n : nodes) {
                        if (n.get() == edge) {
                            next = n;
                            break;
                        }
                    }
                }
            }

            if (!next) {
                break;
            }

            path.push_back(next);
            curr = next;
        }

        return path;
    }

    template<typename NodePtr>
    static void setPrice(const std::vector<NodePtr>& nodes, int price) {
        for (const auto& n : nodes) {
            n->price = price;
        }
    }
};

} // namespace pd

