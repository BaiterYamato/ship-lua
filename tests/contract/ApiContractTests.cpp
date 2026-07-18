// Executa generated/tests/api_contract.lua dentro do runtime real (ModHost)
// em contextos OoT e MM. O script é derivado da IDL (schema/*.yml) por
// tools/generate_api_contracts.py e aborta a carga do mod em qualquer falha.

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "shiplua/generated/ApiBindings.h"
#include "shiplua/host/ModHost.h"
#include "shiplua/input/HotkeyRegistry.h"

#ifndef SHIPLUA_GENERATED_DIR
#define SHIPLUA_GENERATED_DIR "generated"
#endif

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

struct FakeHotkeyRegistry : ShipLua::HotkeyRegistry {
    bool Register(const ShipLua::HotkeyBinding& binding, std::function<void()> onFire) override {
        for (auto& existing : registered) {
            if (existing.modId == binding.modId && existing.id == binding.id) {
                static_cast<ShipLua::HotkeyBinding&>(existing) = binding;
                existing.onFire = std::move(onFire);
                return true;
            }
        }
        FakeHotkey entry;
        static_cast<ShipLua::HotkeyBinding&>(entry) = binding;
        entry.onFire = std::move(onFire);
        registered.push_back(std::move(entry));
        return true;
    }
    void UnregisterMod(const std::string& modId) override {
        registered.erase(
            std::remove_if(registered.begin(), registered.end(), [&](const FakeHotkey& hotkey) {
                return hotkey.modId == modId;
            }),
            registered.end());
    }
    void Fire(const std::string& modId, const std::string& id) override {
        for (auto& hotkey : registered) {
            if (hotkey.modId == modId && hotkey.id == id && hotkey.onFire) {
                hotkey.onFire();
            }
        }
    }
    std::vector<ShipLua::HotkeyBinding> Registered() const override {
        std::vector<ShipLua::HotkeyBinding> out;
        for (const auto& hotkey : registered) {
            out.push_back(hotkey);
        }
        return out;
    }
    struct FakeHotkey : ShipLua::HotkeyBinding {
        std::function<void()> onFire;
    };
    std::vector<FakeHotkey> registered;
};

ShipLua::Manifest MakeManifest(std::string id) {
    ShipLua::Manifest manifest;
    manifest.id = std::move(id);
    manifest.name = "IDL contract probe";
    manifest.version = "0.1.0";
    manifest.apiRange = ">=0.1 <0.3";
    manifest.entrypoint = "main.lua";
    manifest.loadPriority = 50;
    return manifest;
}

// Capabilities com status 'contract' suportadas pelo host, ordenadas para a
// busca binária de ship.capabilities.has — mesma leitura da IDL que o script
// de contrato faz via CONTRACT_CAPABILITIES.
std::vector<std::string> ContractCapabilities(const std::string& game) {
    std::vector<std::string> granted;
    for (const ShipLua::Generated::CapabilityBinding& capability :
         ShipLua::Generated::kCapabilities) {
        const bool supported = game == "oot" ? capability.supportsOot : capability.supportsMm;
        if (supported && capability.status == "contract") {
            granted.emplace_back(capability.name);
        }
    }
    std::sort(granted.begin(), granted.end());
    return granted;
}

void RunContractOnHost(const std::string& game, const std::string& source) {
    auto registry = std::make_shared<FakeHotkeyRegistry>();
    ShipLua::LuaApiHostContext context{game, "contract-test", "0.2.0",
                                       ContractCapabilities(game), registry};
    ShipLua::ModHost host(context, ShipLua::Logger{});
    const std::string modId = "contract.api_" + game;
    const auto loaded = host.LoadModFromManifestAndSource(MakeManifest(modId), source);
    if (!loaded.isOk()) {
        Check(false, "contract script should load on " + game + ": " + loaded.message);
        return;
    }
    Check(host.IsLoaded(modId), "contract mod should stay loaded on " + game);
    Check(registry->registered.size() == 2,
          "contract script should register two hotkeys on " + game);
}

} // namespace

int main() {
    const std::string path = std::string(SHIPLUA_GENERATED_DIR) + "/tests/api_contract.lua";
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        std::cerr << "FAIL: cannot read generated contract script: " << path << '\n';
        return 1;
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    const std::string source = buffer.str();

    RunContractOnHost("oot", source);
    RunContractOnHost("mm", source);

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All API contract checks passed on both hosts\n";
    return 0;
}
