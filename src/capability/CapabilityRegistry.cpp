#include "shiplua/capability/CapabilityRegistry.h"

#include <algorithm>
#include <string_view>

namespace ShipLua {

namespace {

constexpr std::string_view kGameOot = "oot";
constexpr std::string_view kGameMm = "mm";

bool IsIdentifierSegment(std::string_view segment) {
    if (segment.empty() || segment.front() < 'a' || segment.front() > 'z') {
        return false;
    }
    return std::all_of(segment.begin() + 1, segment.end(), [](unsigned char character) {
        return (character >= 'a' && character <= 'z') ||
               (character >= '0' && character <= '9') || character == '_';
    });
}

// Mesma gramática de nomes pontilhados dos schemas públicos:
// [a-z][a-z0-9_]*(\.[a-z][a-z0-9_]*)+
bool IsValidCapabilityId(std::string_view id) {
    std::size_t start = 0;
    std::size_t segments = 0;
    while (start <= id.size()) {
        const std::size_t end = id.find('.', start);
        const std::string_view segment =
            id.substr(start, end == std::string_view::npos ? id.size() - start : end - start);
        if (!IsIdentifierSegment(segment)) {
            return false;
        }
        ++segments;
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return segments >= 2;
}

bool IsValidProviderName(std::string_view name) {
    // Nomes reais de provider podem começar com dígito (ex.: "2ship-native").
    if (name.empty() || name.size() > 64 ||
        !((name.front() >= 'a' && name.front() <= 'z') ||
          (name.front() >= '0' && name.front() <= '9'))) {
        return false;
    }
    return std::all_of(name.begin() + 1, name.end(), [](unsigned char character) {
        return (character >= 'a' && character <= 'z') ||
               (character >= '0' && character <= '9') ||
               character == '_' || character == '-';
    });
}

bool IsKnownGame(const std::string& game) {
    return game == kGameOot || game == kGameMm;
}

bool SupportsGame(const CapabilityProvider& provider, const std::string& game) {
    return game.empty() ||
           std::find(provider.games.begin(), provider.games.end(), game) != provider.games.end();
}

Result<void> ValidateOffer(const std::string& capabilityId, const CapabilityProvider& provider) {
    if (!IsValidCapabilityId(capabilityId)) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "capability id must match [a-z][a-z0-9_]*(\\.[a-z][a-z0-9_]*)+");
    }
    if (!IsValidProviderName(provider.name)) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "provider name must match [a-z0-9][a-z0-9_-]{0,63}");
    }
    if (provider.games.empty()) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "capability offer must declare at least one game");
    }
    for (std::size_t i = 0; i < provider.games.size(); ++i) {
        if (!IsKnownGame(provider.games[i])) {
            return Result<void>::err(ErrorCode::InvalidArgument,
                                     "capability offer game must be 'oot' or 'mm'");
        }
        for (std::size_t j = i + 1; j < provider.games.size(); ++j) {
            if (provider.games[i] == provider.games[j]) {
                return Result<void>::err(ErrorCode::InvalidArgument,
                                         "capability offer games must be unique");
            }
        }
    }
    for (const std::string& permission : provider.permissions) {
        if (!IsValidCapabilityId(permission)) {
            return Result<void>::err(ErrorCode::InvalidArgument,
                                     "capability permission must be a dotted snake_case name");
        }
    }
    return Result<void>::ok();
}

} // namespace

Result<CapabilityStability> ParseCapabilityStability(const std::string& text) {
    if (text == "internal") {
        return Result<CapabilityStability>::ok(CapabilityStability::Internal);
    }
    if (text == "experimental") {
        return Result<CapabilityStability>::ok(CapabilityStability::Experimental);
    }
    if (text == "preview") {
        return Result<CapabilityStability>::ok(CapabilityStability::Preview);
    }
    if (text == "stable") {
        return Result<CapabilityStability>::ok(CapabilityStability::Stable);
    }
    if (text == "deprecated") {
        return Result<CapabilityStability>::ok(CapabilityStability::Deprecated);
    }
    return Result<CapabilityStability>::err(
        ErrorCode::InvalidArgument,
        "stability must be internal, experimental, preview, stable or deprecated");
}

std::string_view CapabilityStabilityName(CapabilityStability stability) {
    switch (stability) {
        case CapabilityStability::Internal: return "internal";
        case CapabilityStability::Experimental: return "experimental";
        case CapabilityStability::Preview: return "preview";
        case CapabilityStability::Stable: return "stable";
        case CapabilityStability::Deprecated: return "deprecated";
    }
    return "preview";
}

Result<void> CapabilityRegistry::Register(const std::string& capabilityId,
                                          CapabilityProvider provider) {
    const auto valid = ValidateOffer(capabilityId, provider);
    if (!valid.isOk()) {
        return valid;
    }
    auto& offers = mEntries[capabilityId];
    if (offers.find(provider.name) != offers.end()) {
        return Result<void>::err(ErrorCode::InvalidState,
                                 "capability offer already registered for this provider");
    }
    offers.emplace(provider.name, std::move(provider));
    return Result<void>::ok();
}

Result<void> CapabilityRegistry::Unregister(const std::string& capabilityId,
                                            const std::string& providerName) {
    const auto found = mEntries.find(capabilityId);
    if (found == mEntries.end() || found->second.erase(providerName) == 0) {
        return Result<void>::err(ErrorCode::InvalidHandle,
                                 "capability offer not registered for this provider");
    }
    if (found->second.empty()) {
        mEntries.erase(found);
    }
    return Result<void>::ok();
}

const CapabilityProvider* CapabilityRegistry::Select(const std::string& id,
                                                     const std::string& game) const {
    const auto found = mEntries.find(id);
    if (found == mEntries.end()) {
        return nullptr;
    }
    const CapabilityProvider* best = nullptr;
    for (const auto& [name, offer] : found->second) {
        if (!SupportsGame(offer, game)) {
            continue;
        }
        if (best == nullptr ||
            offer.capabilityVersion.Compare(best->capabilityVersion) > 0 ||
            (offer.capabilityVersion == best->capabilityVersion &&
             offer.providerVersion.Compare(best->providerVersion) > 0) ||
            (offer.capabilityVersion == best->capabilityVersion &&
             offer.providerVersion == best->providerVersion && name < best->name)) {
            best = &offer;
        }
    }
    return best;
}

bool CapabilityRegistry::Has(const std::string& id, const std::string& game) const {
    return Select(id, game) != nullptr;
}

bool CapabilityRegistry::Has(const std::string& id, const VersionRange& range,
                             const std::string& game) const {
    const CapabilityProvider* selected = Select(id, game);
    return selected != nullptr && range.Contains(selected->capabilityVersion);
}

std::optional<CapabilityDescriptor> CapabilityRegistry::Info(const std::string& id,
                                                             const std::string& game) const {
    const CapabilityProvider* selected = Select(id, game);
    if (selected == nullptr) {
        return std::nullopt;
    }
    CapabilityDescriptor descriptor;
    descriptor.id = id;
    descriptor.version = selected->capabilityVersion;
    descriptor.provider = selected->name;
    descriptor.providerVersion = selected->providerVersion;
    descriptor.games = selected->games;
    descriptor.stability = selected->stability;
    descriptor.permissions = selected->permissions;
    descriptor.limits = selected->limits;
    descriptor.description = selected->description;
    return descriptor;
}

std::vector<std::string> CapabilityRegistry::Providers(const std::string& id,
                                                       const std::string& game) const {
    std::vector<std::string> names;
    const auto found = mEntries.find(id);
    if (found == mEntries.end()) {
        return names;
    }
    for (const auto& [name, offer] : found->second) {
        if (SupportsGame(offer, game)) {
            names.push_back(name);
        }
    }
    return names;
}

std::vector<std::string>
CapabilityRegistry::List(const std::string& game,
                         std::optional<CapabilityStability> stability) const {
    std::vector<std::string> ids;
    for (const auto& [id, offers] : mEntries) {
        const bool listed = std::any_of(offers.begin(), offers.end(), [&](const auto& entry) {
            const CapabilityProvider& offer = entry.second;
            return SupportsGame(offer, game) &&
                   (!stability.has_value() || offer.stability == *stability);
        });
        if (listed) {
            ids.push_back(id);
        }
    }
    return ids;
}

std::size_t CapabilityRegistry::Count() const {
    return mEntries.size();
}

} // namespace ShipLua
