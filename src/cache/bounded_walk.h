#pragma once

#include <cstddef>
#include <utility>
#include <vector>

namespace cache
{

	template <typename Node, typename ChildrenOfFn, typename VisitFn>
	int walk_bounded(const Node& root, int max_depth, int budget,
		ChildrenOfFn children_of, VisitFn visit)
	{
		if (!root.address || max_depth <= 0 || budget <= 0)
			return 0;

		int visited = 0;

		std::vector<std::pair<Node, int>> queue;
		queue.reserve(64);
		queue.emplace_back(root, 0);

		for (std::size_t head = 0; head < queue.size(); ++head)
		{
			const Node node  = queue[head].first;
			const int  depth = queue[head].second;
			if (depth >= max_depth)
				continue;

			auto children = children_of(node);
			for (auto& child : children)
			{
				if (visited >= budget)
					return visited;

				++visited;
				visit(child, depth + 1);
				queue.emplace_back(child, depth + 1);
			}
		}

		return visited;
	}
}
