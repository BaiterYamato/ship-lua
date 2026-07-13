#include <iostream>
#include <string>

#include "shiplua/world/WorldAssetCatalog.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

ShipLua::WorldAssetProbe ValidProbe(const ShipLua::AssetReference& asset, ShipLua::WorldId host) {
    return {
        .asset = asset,
        .host = host,
        .factoryContract = "fast.display_list.bundle",
        .contractVersion = 1,
        .archiveValidated = true,
        .namespaceIsolated = true,
        .bundleLoaded = true,
    };
}

void TestNativeAndForeignFastBundles() {
    auto catalog = ShipLua::CreateDefaultWorldAssetCatalog();
    Check(catalog.DescriptorCount() == 6, "catálogo inicial deve conter seis swords");

    const ShipLua::AssetReference master{ShipLua::WorldId::Oot, "oot.player.sword.master"};
    auto native = catalog.RecordProbe(ValidProbe(master, ShipLua::WorldId::Oot));
    Check(native.isOk(), "host OoT deve provar seu bundle nativo");
    Check(catalog.CanResolve(master, ShipLua::WorldId::Oot),
          "prova nativa válida deve resolver asset");

    auto foreign = catalog.RecordProbe(ValidProbe(master, ShipLua::WorldId::Mm));
    Check(foreign.isOk(), "host MM deve aceitar bundle Fast estrangeiro validado e isolado");
    Check(catalog.CanResolve(master, ShipLua::WorldId::Mm),
          "prova estrangeira válida deve resolver somente no host provador");
    Check(catalog.ResolutionCount() == 2, "resoluções devem ser separadas por host");
}

void TestEveryProofGateIsRequired() {
    const ShipLua::AssetReference gilded{ShipLua::WorldId::Mm, "mm.player.sword.gilded"};

    auto checkRejected = [&](ShipLua::WorldAssetProbe probe, const std::string& message) {
        auto catalog = ShipLua::CreateDefaultWorldAssetCatalog();
        const auto result = catalog.RecordProbe(probe);
        Check(!result.isOk() && result.code == ShipLua::ErrorCode::Unsupported, message);
        Check(!catalog.CanResolve(gilded, ShipLua::WorldId::Oot),
              "prova rejeitada não pode deixar resolução ativa");
    };

    auto archive = ValidProbe(gilded, ShipLua::WorldId::Oot);
    archive.archiveValidated = false;
    checkRejected(archive, "archive não validado deve ser rejeitado");

    auto namespaced = ValidProbe(gilded, ShipLua::WorldId::Oot);
    namespaced.namespaceIsolated = false;
    checkRejected(namespaced, "namespace não isolado deve ser rejeitado");

    auto loaded = ValidProbe(gilded, ShipLua::WorldId::Oot);
    loaded.bundleLoaded = false;
    checkRejected(loaded, "arquivo presente sem carga da factory deve ser rejeitado");

    auto contract = ValidProbe(gilded, ShipLua::WorldId::Oot);
    contract.factoryContract = "soh.scene.mm";
    checkRejected(contract, "factory divergente deve ser rejeitada");

    auto version = ValidProbe(gilded, ShipLua::WorldId::Oot);
    version.contractVersion = 2;
    checkRejected(version, "versão divergente deve ser rejeitada");
}

void TestNamespaceAndHostRestrictions() {
    ShipLua::WorldAssetCatalog catalog;
    ShipLua::WorldAssetDescriptor invalid{
        .asset = {ShipLua::WorldId::Oot, "mm.player.sword.invalid"},
        .kind = ShipLua::WorldAssetKind::EquipmentVisual,
        .factoryContract = "fast.display_list.bundle",
        .contractVersion = 1,
        .compatibleHosts = {ShipLua::WorldId::Oot},
    };
    Check(!catalog.Register(invalid).isOk(), "owner e namespace divergentes devem ser rejeitados");

    ShipLua::WorldAssetDescriptor scene{
        .asset = {ShipLua::WorldId::Oot, "oot.scene.kakariko"},
        .kind = ShipLua::WorldAssetKind::Scene,
        .factoryContract = "soh.scene.oot",
        .contractVersion = 1,
        .compatibleHosts = {ShipLua::WorldId::Oot},
    };
    Check(catalog.Register(scene).isOk(), "scene OoT válida deve registrar");
    Check(!catalog.Register(scene).isOk(), "descritor duplicado deve ser fatal");

    ShipLua::WorldAssetProbe foreign{
        .asset = scene.asset,
        .host = ShipLua::WorldId::Mm,
        .factoryContract = scene.factoryContract,
        .contractVersion = 1,
        .archiveValidated = true,
        .namespaceIsolated = true,
        .bundleLoaded = true,
    };
    Check(!catalog.RecordProbe(foreign).isOk(),
          "scene OoT não deve fingir compatibilidade com factory MM");
}

void TestReloadInvalidatesOnlyOneHost() {
    auto catalog = ShipLua::CreateDefaultWorldAssetCatalog();
    const ShipLua::AssetReference kokiri{ShipLua::WorldId::Oot, "oot.player.sword.kokiri"};
    Check(catalog.RecordProbe(ValidProbe(kokiri, ShipLua::WorldId::Oot)).isOk(),
          "prova OoT deve registrar");
    Check(catalog.RecordProbe(ValidProbe(kokiri, ShipLua::WorldId::Mm)).isOk(),
          "prova MM deve registrar");

    catalog.ClearHost(ShipLua::WorldId::Mm);
    Check(catalog.CanResolve(kokiri, ShipLua::WorldId::Oot), "reload MM não deve apagar prova OoT");
    Check(!catalog.CanResolve(kokiri, ShipLua::WorldId::Mm), "reload MM deve apagar provas MM");

    auto failedReprobe = ValidProbe(kokiri, ShipLua::WorldId::Oot);
    failedReprobe.bundleLoaded = false;
    Check(!catalog.RecordProbe(failedReprobe).isOk(), "reprobe inválido deve falhar");
    Check(!catalog.CanResolve(kokiri, ShipLua::WorldId::Oot),
          "reprobe inválido deve remover prova obsoleta do mesmo host");
}

} // namespace

int main() {
    TestNativeAndForeignFastBundles();
    TestEveryProofGateIsRequired();
    TestNamespaceAndHostRestrictions();
    TestReloadInvalidatesOnlyOneHost();
    return failures == 0 ? 0 : 1;
}
