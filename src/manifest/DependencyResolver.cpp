#include "shiplua/manifest/DependencyResolver.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>

#include "shiplua/manifest/SemVersion.h"

namespace ShipLua {

namespace {

using ManifestMap = std::map<std::string, const Manifest*>;

void AddEdge(const std::string& before, const std::string& after,
             std::map<std::string, std::set<std::string>>& edges,
             std::map<std::string, std::size_t>& indegree) {
    if (edges[before].insert(after).second) {
        ++indegree[after];
    }
}

} // namespace

Result<ModResolution> ResolveMods(const std::vector<Manifest>& manifests) {
    ModResolution resolution;
    ManifestMap candidates;
    for (const Manifest& manifest : manifests) {
        if (manifest.id.empty()) {
            resolution.rejected["<empty-id>"] = "manifest id is empty";
            continue;
        }
        if (candidates.contains(manifest.id)) {
            resolution.rejected[manifest.id] = "duplicate mod id";
            continue;
        }
        candidates.emplace(manifest.id, &manifest);
    }

    std::map<std::string, SemVersion> versions;
    for (const auto& [id, manifest] : candidates) {
        if (resolution.rejected.contains(id)) {
            continue;
        }
        auto version = SemVersion::Parse(manifest->version);
        if (!version.isOk()) {
            resolution.rejected[id] = "invalid mod version: " + version.message;
        } else {
            versions.emplace(id, std::move(*version.value));
        }
    }

    for (const auto& [id, manifest] : candidates) {
        if (resolution.rejected.contains(id)) {
            continue;
        }
        for (const auto& [dependencyId, rangeText] : manifest->dependencies) {
            const auto dependency = candidates.find(dependencyId);
            if (dependency == candidates.end()) {
                resolution.rejected[id] = "missing dependency '" + dependencyId + "'";
                break;
            }
            if (resolution.rejected.contains(dependencyId)) {
                resolution.rejected[id] = "dependency '" + dependencyId + "' is unavailable";
                break;
            }
            auto range = VersionRange::Parse(rangeText);
            if (!range.isOk()) {
                resolution.rejected[id] = "invalid range for dependency '" + dependencyId + "': " +
                                          range.message;
                break;
            }
            if (!range.value->Contains(versions.at(dependencyId))) {
                resolution.rejected[id] = "dependency '" + dependencyId + "' version is incompatible";
                break;
            }
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& [id, manifest] : candidates) {
            if (resolution.rejected.contains(id)) {
                continue;
            }
            for (const auto& [dependencyId, range] : manifest->dependencies) {
                (void)range;
                if (resolution.rejected.contains(dependencyId)) {
                    resolution.rejected[id] = "dependency '" + dependencyId + "' is unavailable";
                    changed = true;
                    break;
                }
            }
        }
    }

    std::map<std::string, std::set<std::string>> edges;
    std::map<std::string, std::size_t> indegree;
    for (const auto& [id, manifest] : candidates) {
        if (!resolution.rejected.contains(id)) {
            edges[id];
            indegree[id] = 0;
        }
    }
    for (const auto& [id, manifest] : candidates) {
        if (resolution.rejected.contains(id)) {
            continue;
        }
        for (const auto& [dependencyId, range] : manifest->dependencies) {
            (void)range;
            AddEdge(dependencyId, id, edges, indegree);
        }
        for (const std::string& afterId : manifest->loadAfter) {
            if (indegree.contains(afterId)) {
                AddEdge(afterId, id, edges, indegree);
            }
        }
        for (const std::string& beforeId : manifest->loadBefore) {
            if (indegree.contains(beforeId)) {
                AddEdge(id, beforeId, edges, indegree);
            }
        }
    }

    auto readyLess = [&candidates](const std::string& left, const std::string& right) {
        const int leftPriority = candidates.at(left)->loadPriority;
        const int rightPriority = candidates.at(right)->loadPriority;
        return leftPriority != rightPriority ? leftPriority < rightPriority : left < right;
    };
    std::set<std::string, decltype(readyLess)> ready(readyLess);
    for (const auto& [id, count] : indegree) {
        if (count == 0) {
            ready.insert(id);
        }
    }

    while (!ready.empty()) {
        const std::string id = *ready.begin();
        ready.erase(ready.begin());
        resolution.orderedIds.push_back(id);
        for (const std::string& dependent : edges[id]) {
            if (--indegree[dependent] == 0) {
                ready.insert(dependent);
            }
        }
    }

    for (const auto& [id, count] : indegree) {
        if (count != 0) {
            resolution.rejected[id] = "dependency or load-order cycle";
        }
    }
    resolution.orderedIds.erase(
        std::remove_if(resolution.orderedIds.begin(), resolution.orderedIds.end(),
                       [&resolution](const std::string& id) {
                           return resolution.rejected.contains(id);
                       }),
        resolution.orderedIds.end());

    return Result<ModResolution>::ok(std::move(resolution));
}

} // namespace ShipLua
