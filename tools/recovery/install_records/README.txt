SMC INSTALL RECORDS — READ-ONLY / ТОЛЬКО ЧТЕНИЕ

EN: Unzip this folder, then double-click Collect-SMC-Install-Records.command.
Requires Python 3. If macOS blocks the launcher, do not disable system security;
report the message instead. The script reads only ~/Documents/x0x37, prioritizing
previously omitted native install logs. It inventories top-level directories,
then scans narrowly named install/SMC/integration/Oct19 evidence directories.
It excludes source, docs, tests, SDKs, firmware binaries, hidden/config/Git folders
and symlinks. No builds, flashing, device commands, network or automatic upload.
Only a new Desktop output folder is written (home folder if Desktop is absent).
Caps: 256 files, 32 MiB total, 2 MiB/file, 12000 entries, depth 5; omissions recorded.
Finder reveals SMC-Install-Records.zip. Review logs and MANIFEST.json: private
paths/device identifiers or secrets may be present. Send that ZIP only when ready.
Conversation export is not required. Original project files are not modified.

RU: Распакуйте папку и дважды нажмите Collect-SMC-Install-Records.command.
Нужен Python 3. Если macOS блокирует запуск, не отключайте защиту — сообщите ошибку.
Скрипт читает только ~/Documents/x0x37: сначала пропущенные журналы установки,
затем узко выбранные папки SMC/установки/интеграции/19 октября. Исходники,
документация, тесты, SDK, прошивки, скрытые папки и симлинки исключены.
Без сборки, прошивки устройства, сети и автоматической отправки. Создаётся только
новая папка на Рабочем столе (или в домашней папке). Лимиты: 256 файлов, 32 МиБ,
2 МиБ на файл, 12000 записей, глубина 5. Пропуски перечислены в MANIFEST.json.
Finder покажет SMC-Install-Records.zip. Перед отправкой проверьте журналы:
возможны личные пути, идентификаторы устройства или секреты. Отправьте ZIP,
когда будете готовы. Экспорт разговора не нужен. Исходные файлы не изменяются.
