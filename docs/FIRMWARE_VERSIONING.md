# X0X37 firmware versioning / Версии прошивки

## English

**Firmware version, build ID and verification status are separate.**

A real firmware version is MAJOR.MINOR.PATCH, with components 0–99. It must be
encoded in the actual .fwsc product identity: for example,
SMK-37 Elite_9000702 means firmware v0.7.2 for the Elite. A filename, Git SHA,
PR number or timestamp is **not** a firmware version.

Historical packages with identity SMK-37 Pro_900 or SMK-37 Elite_900 have **no
numeric firmware version**. Keep their original historical build ID, source
commit and package hash, and label them **Unversioned legacy DEV**. Never
retroactively call them v1.0.0 or claim hardware testing. Historical short
FM-1 identities are likewise not reliable semantic versions.

**Version selection:** MAJOR for incompatible data/storage/MIDI/interface
changes; MINOR for compatible features; PATCH for fixes. The open-beta line
starts at 0.1.0. `tools/release_alloc.py` stores the next number in
`releases/openbeta/ledger.json`. The tree id is the 40-character Git id of
`commit^{tree}`. Issuance checks that the commit exists and that id matches,
then commits the ledger in an isolated `release-records` checkout. It does not
reset the source checkout. A numeric build requires that allocation for the
same commit and Git tree, and it does not require package receipts yet. Catalog
publication requires the bound Elite and Pro receipts. The same version with a
different source, package, or controller is refused. An unallocated number is
refused. Legacy `model_900` packages stay unversioned. A catalog product that is not text is refused instead of being read as a version. Patch 99 rolls to the next minor, and minor 99 rolls
to the next major. The counter stops at 99.99.99. Do not invent a number from a
commit count, a hash, or a date. Rebuilding the same commit and tree keeps its
number. The same tree at a different commit is refused. Different package bytes
cannot reuse a recorded receipt. Elite and Pro share one number and keep
separate package and controller hashes. The readable label is
`X.Y.Z_openbeta`. The `_openbeta` suffix is not written into the package
identity and a renamed catalog file does not change the installed identity.
The preserved source `892bf60d912eca93597dcf03be25a9756540c949` stays
unversioned. Different DEV artifacts at the same version must
have distinct build IDs containing the immutable source SHA plus package
SHA-256, compactly displayed as g[git12]-h[sha256-12]. Preserve full hashes in
records. Never overwrite previously published package bytes.

**Build recording:** tools/dev_validate.py run now supports
--firmware-version X.Y.Z for model-specific Pro/Elite DEV validation. It uses
the existing build.sh --release *numeric identity stamping option* while
retaining the DEV profile (X0X_DEV_PADCAL=1). This does **not** run a release
gate or certify a stable release. Successful DEV evidence includes the
package's actual version, model, product identity, source SHA, SHA-256, build
ID and the explicit DEVICE NOT TESTED label. The verify command rejects
changed package bytes, altered metadata and mismatched version identities.
Legacy no-version DEV workflows remain supported and explicitly unversioned.

The standalone tools/firmware_build_labels.py command inspects the real
package's embedded identity, transport CRCs and update loader. Required inputs
are --model and --source-commit; --expected-version is an optional exact
consistency check. Its result is **PACKAGE INSPECTED**, not DEV CHECKED, and
is never a hardware approval or installation authorization.

**Catalog/display:** Store firmware_version (nullable), version_label,
build_id, model, product, full source_commit, package_sha256, status,
device_tested and installable independently. A version alone is not a unique
build key. Catalog consumers must verify model, provenance and package bytes
instead of trusting labels. Example only, NOT an actual built release:

X0X37 v0.7.2 · SMK-37 Elite · NIGHTLY DEV CHECKED · DEVICE NOT TESTED

Existing legacy example:

X0X37 Unversioned legacy DEV · Pro · dev-20261022-1638-e894804 · DEVICE NOT TESTED

The NIGHTLY catalog retains source-only, failed, superseded and DEV history.
History entries lacking a fully checked installable package have no flash URL.
Only pre-hardware-safety-checked, distribution-safe packages can be offered for
owner-controlled NIGHTLY hardware tests. PRODUCTION requires exact per-model,
same-byte owner physical proof and separate production authorization. Neither
version numbers nor a DEV CHECKED flag grant permission to flash, merge to
production, publish stable releases or redistribute proprietary SDK/controller
binaries.

## Русский

**Версия прошивки, идентификатор сборки и статус проверки — разные поля.**
Настоящая версия MAJOR.MINOR.PATCH записана внутри .fwsc: идентификатор
SMK-37 Elite_9000702 означает v0.7.2 для Elite. Время, имя файла и хеш Git
сами по себе версией не являются.

У старых DEV-пакетов с SMK-37 Pro_900 или SMK-37 Elite_900 **нет числовой
версии**. Их нужно помечать Unversioned legacy DEV и сохранять исходный хеш
Git, SHA-256 пакета и старый build ID. Придумывать им v1.0.0 нельзя.

MAJOR обозначает несовместимые изменения; MINOR — совместимые новые функции;
PATCH — исправления. Следующий номер open-beta берётся из
releases/openbeta/ledger.json, начиная с 0.1.0. Дерево — это 40-символьный
Git-идентификатор `commit^{tree}`. Запись номера идёт в отдельную ветку
release-records и не сбрасывает исходный checkout. После 0.1.99 идёт 0.2.0.
Суффикс `_openbeta` только для показа рядом с ABOUT X0X и не входит в
идентификатор пакета. Тот же коммит и то же дерево сохраняют номер. То же
дерево у другого коммита отклоняется. Другие
байты пакета этот номер повторить не могут. Elite и Pro делят номер, но
хранят разные хеши пакета и контроллера. Исходник
892bf60d912eca93597dcf03be25a9756540c949 остаётся без числовой версии.
Запись каталога, в которой поле продукта не является текстом, отклоняется и
не читается как версия. Переименование файла в каталоге не меняет
установленную прошивку.

tools/dev_validate.py run с --firmware-version X.Y.Z записывает номер в
DEV-прошивку Pro/Elite и проверяет совпадение номера внутри пакета и записи.
Параметр **не делает сборку релизом** и не заменяет тест на устройстве.
Отдельная утилита tools/firmware_build_labels.py проверяет идентификатор,
CRC и SHA-256, но **не** подтверждает DEV CHECKED или аппаратную пригодность.

NIGHTLY показывает полноценную историю, включая неудачные, недоступные и
устаревшие сборки. Непроверенные варианты не должны иметь кнопку прошивки.
Для production нужны доказанные тесты на соответствующем устройстве и
отдельное разрешение владельца. Статус DEVICE NOT TESTED нельзя заменять
на DEVICE VERIFIED по дате, номеру или успешной компиляции.
