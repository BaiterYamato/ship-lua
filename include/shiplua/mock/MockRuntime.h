#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "shiplua/events/EventDispatcher.h"
#include "shiplua/host/ModHost.h"
#include "shiplua/input/HotkeyRegistry.h"
#include "shiplua/manifest/Manifest.h"
#include "shiplua/runtime/Logger.h"
#include "shiplua/runtime/Result.h"
#include "shiplua/storage/KeyValueStorage.h"
#include "shiplua/timer/FrameTimerScheduler.h"

struct lua_State;

namespace ShipLua {

struct MockLogEntry {
    LogLevel level;
    std::string modId;
    std::string message;
};

struct MockHostOptions {
    std::string gameId = "oot";                  // 'oot' ou 'mm' (contrato do host)
    std::string hostVersion = "0.0.0-mock";
    std::string runtimeVersion = "0.2.0";
    std::vector<std::string> extraCapabilities;  // além das core.* do mock
    std::size_t memoryLimitBytes = 0;            // 0 = sem teto de memória Lua
};

// HotkeyRegistry em memória usado pelo mock: aceita todos os registros e
// permite disparar hotkeys por tecla default (simulando input) ou por id.
class MockHotkeyRegistry final : public HotkeyRegistry {
  public:
    bool Register(const HotkeyBinding& binding, std::function<void()> onFire) override;
    void UnregisterMod(const std::string& modId) override;
    void Fire(const std::string& modId, const std::string& id) override;
    std::vector<HotkeyBinding> Registered() const override;

    // Dispara todos os bindings cuja tecla default é `key` (comparação exata).
    // Retorna os bindings disparados, em ordem determinística.
    std::vector<HotkeyBinding> PressKey(const std::string& key);

    std::optional<HotkeyBinding> BindingFor(const std::string& modId,
                                            const std::string& id) const;
    std::size_t Count() const;

  private:
    struct Entry {
        HotkeyBinding binding;
        std::function<void()> onFire;
    };

    std::map<std::string, std::map<std::string, Entry>> mEntries;  // modId -> id -> Entry
};

// Runtime de teste sem jogo/ROM (plan-sdk.md §16 'Mock runtime'). Fornece
// providers de teste para as capabilities centrais — eventos, timers, log,
// storage, capabilities e input — anunciando core.events, core.timers,
// core.storage e core.input ao mod.
//
// Um MockRuntime hospeda um único mod por vez: cada arquivo de teste deve
// carregar uma instância nova, garantindo isolamento entre arquivos.
// Confinado à thread que o criou, como os demais componentes do núcleo.
class MockRuntime {
  public:
    // Valida as opções (game id e capabilities contra o schema contratado)
    // antes de construir o host.
    static Result<MockRuntime> Create(MockHostOptions options = {});

    MockRuntime(MockRuntime&& other) noexcept = default;
    MockRuntime& operator=(MockRuntime&& other) noexcept = default;
    MockRuntime(const MockRuntime&) = delete;
    MockRuntime& operator=(const MockRuntime&) = delete;
    ~MockRuntime() = default;

    // Capabilities anunciadas pelo mock além das extraCapabilities.
    static std::vector<std::string> CoreCapabilities();

    // --- Carregamento -------------------------------------------------------
    Result<void> LoadModFromDirectory(const std::string& dir);
    Result<void> LoadModFromManifestAndSource(const Manifest& manifest,
                                              const std::string& luaSource);
    Result<void> UnloadMod();
    bool IsLoaded() const;
    const std::string& ModId() const;
    lua_State* ModState();  // nullptr quando não há mod carregado

    // --- Lifecycle ----------------------------------------------------------
    Result<DispatchOutcome> EmitGameReady();
    Result<DispatchOutcome> EmitGameShutdown();

    // --- Simulação ----------------------------------------------------------
    Result<DispatchOutcome> EmitEvent(const std::string& name, EventPayload payload = {});

    // Avança `frames` frames: cada frame dispara os timers vencidos e emite
    // game.frame com o número do novo frame. Retorna o frame atual.
    Result<std::uint64_t> AdvanceFrames(std::uint64_t frames = 1);
    std::uint64_t CurrentFrame() const;

    // Dispara as hotkeys cuja tecla default é `key` e emite input.hotkey para
    // cada uma. Retorna quantas hotkeys foram disparadas.
    std::size_t PressKey(const std::string& key);

    // Dispara a hotkey `id` do mod carregado e emite input.hotkey.
    bool TriggerHotkey(const std::string& id);

    // Limpa os logs capturados e o namespace de storage do mod. Não cancela
    // timers nem inscrições de eventos/hotkeys (fiação do entrypoint).
    void ResetSimulation();

    // --- Inspeção -----------------------------------------------------------
    const std::vector<MockLogEntry>& Logs() const;
    void ClearLogs();
    KeyValueStorage& Storage();
    const KeyValueStorage& Storage() const;
    FrameTimerScheduler& Timers();
    const FrameTimerScheduler& Timers() const;
    MockHotkeyRegistry& Hotkeys();
    const MockHotkeyRegistry& Hotkeys() const;
    ModHost& Host();
    const MockHostOptions& Options() const;

  private:
    explicit MockRuntime(MockHostOptions options);

    // Ordem de declaração define a ordem de construção: o buffer de log e o
    // logger precisam existir antes do ModHost, e o sink captura o buffer
    // compartilhado (estável entre moves).
    MockHostOptions mOptions;
    std::shared_ptr<std::vector<MockLogEntry>> mLogs;
    Logger mLogger;
    std::shared_ptr<FrameTimerScheduler> mTimers;
    std::shared_ptr<KeyValueStorage> mStorage;
    std::shared_ptr<MockHotkeyRegistry> mHotkeys;
    ModHost mHost;
    std::string mModId;
};

} // namespace ShipLua
