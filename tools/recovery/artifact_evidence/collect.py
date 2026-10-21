#!/usr/bin/env python3
"""Offline, bounded X0X evidence copier. No device/network/Git operations."""
import datetime, hashlib, json, os, stat, sys, tempfile, zipfile
from pathlib import Path

MAX_ENTRIES = 50000
MAX_FILES = 2500
MAX_BYTES = 200 * 1024 * 1024
MAX_FILE = 8 * 1024 * 1024
MAX_TEXT = 2 * 1024 * 1024
MAX_DEPTH = 12
MAX_TEXT_TOTAL = 40 * 1024 * 1024
PRUNE = {'.git', '.codex', '.ssh', '.aws', '.venv', 'venv', 'node_modules', '__pycache__', 'slot_backups', 'samples', 'refs-oscuro', 'toolchain', 'sdk', 'site-packages', '.config', 'credentials', 'secrets', 'keychains'}
EXACT = {'crc.txt', 'index.md', 'record.json', 'install-record.json', 'install-ledger.json', 'manifest.json', 'checksums.txt', 'sha256sums', 'ble_attempts.md', 'handoff.txt', 'handoff.md', 'smc-device-investigation-2026-10-20.md'}

def safe_open_dir(path):
    """Open absolute path component-by-component; reject all symlinks."""
    path = os.path.abspath(path)
    fd = os.open('/', os.O_RDONLY | os.O_DIRECTORY)
    try:
        for part in path.split('/')[1:]:
            if not part:
                continue
            new = os.open(part, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=fd)
            os.close(fd)
            fd = new
        return fd
    except BaseException:
        os.close(fd)
        raise

def selection(name, rel):
    n, r = name.lower(), rel.lower()
    if any(t in n for t in ('credential', 'secret', 'token', 'password', 'auth', '.env')):
        return None
    if n.endswith('.fwsc') or n == 'x0x.bin' or (n.endswith('.img') and 'ctl' in n):
        return 'binary'
    if n in EXACT or n.endswith(('.log', '.jsonl')):
        return 'text'
    if n.endswith(('.txt', '.md', '.json')) and any(t in r for t in ('gate-results', '.logs/', 'install', 'manifest', 'checksum', 'artifact-comparison', 'validation')):
        return 'text'
    return None

def stamp(s):
    return {'size': s.st_size, 'mtime_ns': s.st_mtime_ns, 'ctime_ns': s.st_ctime_ns,
            'device': s.st_dev, 'inode': s.st_ino, 'mode': oct(stat.S_IMODE(s.st_mode)),
            'birthtime': getattr(s, 'st_birthtime', None)}

def read_stable(fd, name, expected, limit):
    f = os.open(name, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK, dir_fd=fd)
    try:
        before = os.fstat(f)
        if not stat.S_ISREG(before.st_mode) or stamp(before) != stamp(expected):
            raise ValueError('changed before read / not regular')
        if before.st_size > limit:
            raise ValueError('size limit')
        chunks, count = [], 0
        while True:
            b = os.read(f, min(1024 * 1024, limit + 1 - count))
            if not b:
                break
            chunks.append(b); count += len(b)
            if count > limit:
                raise ValueError('grew past limit')
        after = os.fstat(f)
        if stamp(after) != stamp(before) or count != before.st_size:
            raise ValueError('changed during read')
        return b''.join(chunks)
    finally:
        os.close(f)

def collect(roots, output):
    manifest = {'format': 1, 'collected_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
      'limits': {'entries': MAX_ENTRIES, 'files': MAX_FILES, 'total_uncompressed_bytes': MAX_BYTES, 'binary_bytes': MAX_FILE, 'text_bytes': MAX_TEXT, 'depth': MAX_DEPTH, 'text_total_bytes': MAX_TEXT_TOTAL},
      'warning': 'Timestamps are provenance, NOT proof of installation. No upload performed. Logs may contain private paths, device IDs, source details, or secrets. Review before sharing.',
      'roots': [], 'inventory': [], 'scan_complete': True, 'skipped_non_candidates': 0}
    counts = {'entries': 0, 'files': 0, 'bytes': 0, 'text_bytes': 0, 'text_files': 0}
    with zipfile.ZipFile(output, 'x', compression=zipfile.ZIP_DEFLATED) as z:
        def walk(fd, label, rel='', depth=0):
            if counts['entries'] >= MAX_ENTRIES:
                manifest['scan_complete'] = False
                return
            try:
                names = []
                with os.scandir(fd) as entries:
                    for entry in entries:
                        if len(names) >= MAX_ENTRIES - counts['entries']:
                            manifest['scan_complete'] = False
                            manifest['inventory'].append({'path': label+'/'+rel, 'status': 'directory_listing_limit'})
                            break
                        names.append(entry.name)
                names.sort(key=lambda n: (0 if any(t in n.lower() for t in ('flash_review', 'gate-results', '20261019', '2026-10-19', '53d5654', '9869d40', '4346c87')) else 1, 0 if n.endswith('.fwsc') or 'ctl.img' in n or n == 'x0x.bin' else 1, n))
            except OSError as e:
                manifest['inventory'].append({'path': label+'/'+rel, 'status': 'directory_error', 'error': str(e)})
                return
            for name in names:
                if counts['entries'] >= MAX_ENTRIES:
                    manifest['scan_complete'] = False
                    return
                counts['entries'] += 1
                child = rel+'/'+name if rel else name
                row = {'path': label+'/'+child}
                try:
                    s = os.stat(name, dir_fd=fd, follow_symlinks=False)
                    if stat.S_ISLNK(s.st_mode):
                        row['status'] = 'symlink_not_followed'
                    elif stat.S_ISDIR(s.st_mode):
                        low = name.lower()
                        if label == 'gate-cache' and low == '.config':
                            # Never list private config: open ONLY .config/x0x/ctl.img.
                            row['status'] = 'private_config_excluded_except_exact_controller'
                            cache_row = {'path': label+'/'+child+'/x0x/ctl.img', 'kind': 'binary'}
                            config_fd = x0x_fd = None
                            try:
                                config_fd = os.open(name, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=fd)
                                x0x_fd = os.open('x0x', os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=config_fd)
                                cs = os.stat('ctl.img', dir_fd=x0x_fd, follow_symlinks=False)
                                cache_row.update(stamp(cs))
                                if not stat.S_ISREG(cs.st_mode): cache_row['status'] = 'nonregular_not_read'
                                elif cs.st_size > MAX_FILE: cache_row['status'] = 'file_size_limit'
                                elif counts['files'] >= MAX_FILES or counts['bytes']+cs.st_size > MAX_BYTES: cache_row['status'] = 'archive_budget_limit'
                                else:
                                    data = read_stable(x0x_fd, 'ctl.img', cs, MAX_FILE)
                                    cache_row.update(status='copied', sha256=hashlib.sha256(data).hexdigest(), archive_member='evidence/'+cache_row['path'])
                                    z.writestr(cache_row['archive_member'], data)
                                    counts['files'] += 1; counts['bytes'] += len(data)
                            except (OSError, ValueError) as e:
                                cache_row.update(status='missing_unreadable_or_changed_private_ctl', error=str(e))
                            finally:
                                if x0x_fd is not None: os.close(x0x_fd)
                                if config_fd is not None: os.close(config_fd)
                            manifest['inventory'].append(cache_row)
                        elif low in PRUNE or any(t in low for t in ('ac79', 'jieli', 'toolchain', 'sdk', 'slot-backup', 'slot_backup')):
                            row['status'] = 'excluded_directory'
                        elif depth >= MAX_DEPTH:
                            row['status'] = 'depth_limit'; manifest['scan_complete'] = False
                        else:
                            sub = os.open(name, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=fd)
                            try: walk(sub, label, child, depth+1)
                            finally: os.close(sub)
                            continue
                    elif stat.S_ISREG(s.st_mode):
                        kind = selection(name, child)
                        if label == 'controller-cache' and child != 'ctl.img':
                            kind = None
                        if not kind:
                            manifest['skipped_non_candidates'] += 1
                            continue
                        row.update(stamp(s)); row['kind'] = kind
                        limit = MAX_FILE if kind == 'binary' else MAX_TEXT
                        if s.st_size > limit:
                            row['status'] = 'file_size_limit'
                        elif counts['files'] >= MAX_FILES or counts['bytes']+s.st_size > MAX_BYTES or (kind == 'text' and (counts['text_bytes']+s.st_size > MAX_TEXT_TOTAL or counts['text_files'] >= 1000)):
                            row['status'] = 'archive_budget_limit'
                        else:
                            data = read_stable(fd, name, s, limit)
                            row['sha256'] = hashlib.sha256(data).hexdigest()
                            row['archive_member'] = 'evidence/'+label+'/'+child
                            z.writestr(row['archive_member'], data)
                            counts['files'] += 1; counts['bytes'] += len(data)
                            if kind == 'text':
                                counts['text_bytes'] += len(data); counts['text_files'] += 1
                            row['status'] = 'copied'
                    else:
                        row['status'] = 'special_file_not_read'
                except (OSError, ValueError) as e:
                    row['status'] = 'read_error'; row['error'] = str(e)
                manifest['inventory'].append(row)
        for label, path in roots:
            entry = {'label': label, 'path': str(path)}
            manifest['roots'].append(entry)
            try:
                fd = safe_open_dir(path)
                try:
                    entry['status'] = 'opened'
                    if label == 'controller-cache':
                        # Only ctl.img; never enumerate neighboring personal config files.
                        try:
                            s = os.stat('ctl.img', dir_fd=fd, follow_symlinks=False)
                            row = {'path': label+'/ctl.img', **stamp(s)}
                            if not stat.S_ISREG(s.st_mode): row['status'] = 'nonregular_not_read'
                            elif s.st_size > MAX_FILE or counts['bytes']+s.st_size > MAX_BYTES: row['status'] = 'size_or_budget_limit'
                            else:
                                data = read_stable(fd, 'ctl.img', s, MAX_FILE)
                                row.update(status='copied', sha256=hashlib.sha256(data).hexdigest(), archive_member='evidence/controller-cache/ctl.img')
                                z.writestr(row['archive_member'], data); counts['bytes'] += len(data); counts['files'] += 1
                            manifest['inventory'].append(row)
                        except (OSError, ValueError) as e: entry.update(status='missing_unreadable_or_changed_ctl', error=str(e))
                    else: walk(fd, label)
                finally: os.close(fd)
            except OSError as e:
                entry.update(status='missing_unreadable_or_symlink', error=str(e))
        manifest['counts'] = counts
        statuses = {}
        for row in manifest['inventory']:
            key = row['status']; statuses[key] = statuses.get(key, 0) + 1
        manifest['status_counts'] = statuses
        manifest['all_selected_candidates_copied'] = manifest['scan_complete'] and all(r['status'] == 'opened' for r in manifest['roots']) and not any(k in statuses for k in ('read_error', 'directory_error', 'file_size_limit', 'archive_budget_limit', 'depth_limit', 'directory_listing_limit', 'size_or_budget_limit', 'nonregular_not_read', 'missing_unreadable_or_changed_private_ctl'))
        z.writestr('MANIFEST.json', json.dumps(manifest, indent=2, ensure_ascii=True))
        z.writestr('READ-ME.txt', manifest['warning']+'\nMissing, excluded, changed and oversized files are listed in MANIFEST.json. Not all scanned files are collected. No installation identity is inferred.\n')
    return manifest

def main():
    home = Path.home()
    dest = home/'Desktop'
    if not dest.is_dir(): dest = home
    output_dir = Path(tempfile.mkdtemp(prefix='SMC-Recovery-', dir=str(dest)))
    os.chmod(output_dir, 0o700)
    archive = output_dir/'SMC-Recovery-Evidence.zip'
    roots = [('controller-cache', home/'.config/x0x'), ('project', home/'Documents/x0x37'), ('gate-cache', home/'.cache/x0x37-gate')]
    print('Collecting existing X0X evidence only. No firmware, network or device access.', flush=True)
    m = collect(roots, archive)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    (output_dir/'ARCHIVE-SHA256.txt').write_text(digest+'  '+archive.name+'\n')
    (output_dir/'MANIFEST.json').write_text(json.dumps(m, indent=2))
    print('\nInventory statuses: '+json.dumps(m['status_counts'])+'\nAll selected candidates copied: '+str(m['all_selected_candidates_copied']))
    print('\nCreated: '+str(archive)+'\nSHA-256: '+digest+'\nFiles copied: '+str(m['counts']['files'])+'\nReview before uploading. Nothing was sent automatically.', flush=True)
    if sys.platform == 'darwin':
        import subprocess
        subprocess.run(['/usr/bin/open', '-R', str(archive)], check=False)

if __name__ == '__main__':
    main()
