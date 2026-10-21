#!/bin/bash
set -eu
umask 077
cd -- "$(dirname -- "$0")"
echo 'SMC evidence recovery / Сбор сохранённых данных SMC'
echo 'Read-only collection; no firmware changes or automatic upload.'
echo 'Только чтение исходных файлов; без прошивки и автоматической отправки.'
if ! command -v python3 >/dev/null 2>&1; then
  echo 'Python 3 is required. Nothing was collected. / Требуется Python 3.'
else
  python3 -B collect.py || echo 'Collection failed. Preserve any partial output. / Ошибка сбора; сохраните частичный результат.'
fi
read -r -p 'Press Return to close / Нажмите Enter для выхода: ' _ || true
