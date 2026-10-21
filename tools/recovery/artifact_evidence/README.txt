SMC original-build recovery — offline collector

ENGLISH
1. Extract this entire folder. Double-click Collect-SMC-Evidence.command.
   Python 3 must already be installed; nothing is downloaded or installed.
   If macOS blocks it, stop and send the exact message; do not disable security.
2. Finder reveals SMC-Recovery-Evidence.zip inside a new SMC-Recovery-* folder
   on your Desktop (home folder if Desktop is missing).
3. Review MANIFEST.json and the ZIP locally, then upload the ZIP here if you
   agree to share it. Nothing is transmitted automatically.

Only existing files are read. No firmware installation, MIDI/device access,
network requests, builds, Git commands, or source/cache changes are performed.
Scope: ~/Documents/x0x37, ~/.cache/x0x37-gate, and ONLY ctl.img inside
~/.config/x0x. Symlinks are never followed. SDK/toolchain, .git, .codex,
credentials/config directories and sample-backup directories are excluded.
The collector copies selected install/build logs, evidence manifests and
firmware files (*.fwsc, x0x.bin, controller *.img). It does not collect every
source file. Inside the gate-cache root, only the exact .config/x0x/ctl.img path is read
from private config folders; no other config entries are listed or read.

Limits: 50,000 directory entries, depth 12, 2,500 files, 200 MiB total original
contents (at most 40 MiB / 1000 text files); each binary at most 8 MiB, text file 2 MiB.
Old named paths and firmware files are prioritized; not every old artifact is
guaranteed to fit. Missing/excluded/changed/oversized files and budget omissions
are recorded. Check scan_complete; an incomplete scan is NOT proof of absence.
SHA-256 hashes and file metadata identify copied bytes. Modification dates do
NOT prove which build was installed. Files changing during collection are
rejected where detected. Keep the originals and do not rebuild/delete caches.

Privacy: logs may contain your username, local paths, device identifiers,
private source details or embedded secrets. Filename filtering is not complete
secret redaction. Review before sharing; firmware may include vendor bytes.
No Codex conversation/session storage is collected. No conversation export
is required to run this collector.

РУССКИЙ
1. Распакуйте всю папку и дважды нажмите Collect-SMC-Evidence.command.
   Нужен уже установленный Python 3; программа ничего не устанавливает.
   Если macOS блокирует запуск, пришлите текст ошибки. Не отключайте защиту.
2. Finder покажет SMC-Recovery-Evidence.zip в новой папке SMC-Recovery-* на
   рабочем столе (или в домашней папке, если рабочего стола нет).
3. Просмотрите MANIFEST.json и ZIP, затем загрузите ZIP в этот чат, если
   согласны поделиться данными. Автоматической отправки нет.

Исходные файлы только читаются. Нет прошивки, доступа к MIDI/устройствам,
сети, сборки, команд Git или изменения исходников/кэша. Читаются выбранные
журналы, манифесты и файлы прошивки из ~/Documents/x0x37,
~/.cache/x0x37-gate и только ctl.img из ~/.config/x0x.
Символические ссылки, SDK/компиляторы, .git, .codex, папки с настройками,
учётными данными и резервными копиями сэмплов исключены. Внутри кэша gate
из папок .config читается только точный путь x0x/ctl.img, без просмотра
остального содержимого настроек.

Ограничения: 50 000 записей каталогов, глубина 12, 2500 файлов, суммарно
200 МиБ исходных данных, из них не более 40 МиБ текста; до 8 МиБ на бинарный
файл и 2 МиБ на текстовый. Пропуски отмечаются в MANIFEST.json. Дата файла
не доказывает, что именно его установили. Не удаляйте оригиналы и не
пересобирайте прошивку до проверки сохранённых артефактов.

Журналы могут содержать имя пользователя, пути, идентификаторы устройств,
закрытые сведения или секреты. Фильтрация имён не гарантирует удаления
секретов: проверьте архив перед отправкой. История сессий Codex не читается.
Для запуска сборщика экспорт переписки не требуется.
