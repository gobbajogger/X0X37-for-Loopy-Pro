#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Reuse generated headers after content and execution-dependency checks.

Unknown execution dependencies disable reuse. Cache errors never disable generation.
Each operation uses private output files. Only complete entries become visible.
"""
import hashlib
import itertools
import glob
import struct
import stat
import fcntl
from contextlib import ExitStack
import json
import os
from pathlib import Path
import platform
import re
import runpy
import shlex
import shutil
import subprocess
import sys
import tempfile

OUTPUTS = {'font': ('felucca_font.h',),
           'drums': ('x0x_drum_samples.h', 'x0x_drum_tables.h'),
           'breaks': ('x0x_break_bank.h',)}


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def encoded(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':')).encode()


def file_identity(path, root):
    path = Path(path).absolute()
    try:
        name = '$SRC/' + str(path.relative_to(root))
    except ValueError:
        name = str(path)
    return name, digest(path)


def cache_directory(root):
    try:
        common = subprocess.check_output(['git', '-C', str(root), 'rev-parse',
                                          '--git-common-dir'], stderr=subprocess.DEVNULL,
                                         text=True).strip()
        return (root / common).resolve() / 'x0x37-asset-cache' / 'v1'
    except (OSError, subprocess.SubprocessError):
        # Archives have no shared Git identity. Keep their cache outside source inputs.
        return root / 'build' / 'asset-cache' / 'v1'


def python_command(script, *args, scratch):
    # Ignore existing timestamp-based bytecode. Hash and execute the source files.
    return [sys.executable, '-B', '-X', 'pycache_prefix=' + str(scratch / 'pycache'),
            str(script), *map(str, args)]


def path_selection(path, root, memo=None, active=()):
    """Keep aliases and every link in a path, including links in parent folders.

    Memoization lasts for one snapshot only. Each verification starts afresh.
    """
    memo = {} if memo is None else memo
    path = Path(path).absolute()
    if path in active or len(active) >= 128:
        raise ValueError('dependency symlink loop or unsupported path depth')
    if path in memo:
        return memo[path]
    if path == path.parent:
        result = {'target': str(path), 'links': []}
    else:
        active = (*active, path)
        parent = path_selection(path.parent, root, memo, active)
        selected = identity_path(parent['target'], root) / path.name
        if path.name == '..':
            selected = identity_path(parent['target'], root).parent
        links = parent['links']
        if selected.is_symlink():
            target = os.readlink(selected)
            destination = path_selection(selected.parent / target, root, memo, active)
            links = [*links, [file_identity_name(selected, root), target], *destination['links']]
            selected = identity_path(destination['target'], root)
        if len(links) > 40:
            raise ValueError('dependency symlink chain too long')
        result = {'target': file_identity_name(selected, root), 'links': links}
    memo[path] = result
    return result


def lookup_directories(paths, root, recursive=()):
    result, selections, listings = {}, {}, {}
    for directory in sorted(set(Path(p).absolute() for p in paths)):
        key = file_identity_name(directory, root)
        selection = path_selection(directory, root, selections)
        canonical = identity_path(selection['target'], root)
        listing_key = (canonical, directory in recursive, directory == root / 'build/gen')
        if listing_key not in listings:
            if not canonical.is_dir():
                listings[listing_key] = None
            else:
                values = {}
                pending = [(canonical, frozenset())]
                while pending:
                    folder, ancestors = pending.pop()
                    info = folder.stat()
                    inode = (info.st_dev, info.st_ino)
                    if inode in ancestors:
                        raise ValueError('recursive dependency directory cycle')
                    with os.scandir(folder) as entries:
                        for entry in entries:
                            path = Path(entry.path)
                            if ('__pycache__' in path.parts or path.name.endswith('.pyc')
                                    or (directory == root / 'build/gen' and path.name == 'x0x_break_bank.h')):
                                continue
                            mode = entry.stat(follow_symlinks=False).st_mode
                            value = [stat.S_IFMT(mode), stat.S_IMODE(mode) & 0o111]
                            if stat.S_ISLNK(mode):
                                value.append(path_selection(path, root, selections))
                            values[str(path.relative_to(canonical))] = value
                            # Include search trees reached through directory aliases.
                            if directory in recursive and entry.is_dir():
                                pending.append((path, ancestors | {inode}))
                listings[listing_key] = values
        result[key] = {'selection': selection, 'entries': listings[listing_key]}
    return result


def execution_selector(command, root):
    """Resolve exactly as subprocess does with cwd=root; retain the chosen alias."""
    search = os.environ.get('PATH', os.defpath).split(os.pathsep)
    search = [str((root / p).absolute()) for p in search]
    request = str(root / command) if os.path.dirname(command) else command
    selected = shutil.which(request, path=os.pathsep.join(search))
    if selected is None:
        raise ValueError('unresolved execution dependency: ' + command)
    return Path(selected).absolute()


def resolution_inputs(files, root):
    memo = {}
    return {file_identity_name(p.absolute(), root): path_selection(p, root, memo) for p in files}


def file_identity_name(path, root):
    try:
        return '$SRC/' + str(Path(path).relative_to(root))
    except ValueError:
        return str(path)


def identity_path(name, root):
    return root / name[5:] if name.startswith('$SRC/') else Path(name)


def cpu_identity():
    path = Path('/proc/cpuinfo')
    if not path.is_file():
        return {'machine': platform.machine()}
    names = {'vendor_id', 'cpu family', 'model', 'model name', 'stepping', 'flags',
             'Features', 'CPU architecture', 'CPU implementer', 'CPU part', 'CPU revision'}
    values = {}
    for line in path.read_text().splitlines():
        name, separator, value = line.partition(':')
        if separator and name.strip() in names:
            values.setdefault(name.strip(), set()).add(value.strip())
    return {name: sorted(items) for name, items in sorted(values.items())}


def environment():
    return {k: v for k, v in sorted(os.environ.items())
            if k.startswith(('PYTHON', 'LD_', 'LC_', 'DYLD_', 'X0X_FONT', 'GCC_', 'COLLECT_', 'QEMU_', 'GLIBC_'))
            or k in ('SOURCE_DATE_EPOCH', 'PATH', 'LANG', 'TZ', 'CC', 'CPATH',
                     'C_INCLUDE_PATH', 'LIBRARY_PATH', 'COMPILER_PATH', 'SDKROOT')}


def verify_identity(before, root):
    """Rehash the discovered closure and its lookup directories, without execution."""
    if before.get('resolution_schema') != 2:
        return False
    if before['environment'] != environment() or before.get('cpu') != cpu_identity():
        return False
    if any(digest(identity_path(name, root)) != value for name, value in before['files'].items()):
        return False
    if resolution_inputs([identity_path(name, root) for name in before['resolutions']], root) != before['resolutions']:
        return False
    for command, selected in before.get('executables', {}).items():
        if file_identity_name(execution_selector(command, root), root) != selected:
            return False
    paths = [identity_path(name, root) for name in before['lookups']]
    recursive = {identity_path(name, root) for name in before.get('recursive', [])}
    if lookup_directories(paths, root, recursive) != before['lookups']:
        return False
    for name, value in before.get('candidates', {}).items():
        path = identity_path(name, root)
        if (digest(path) if path.is_file() else None) != value:
            return False
    return True


def loader_inputs(files, cwd=None, candidates=None):
    """Capture supported glibc selectors, including absent and hardware paths."""
    cwd = Path.cwd() if cwd is None else cwd
    if any(os.environ.get(k) for k in ('LD_PRELOAD', 'LD_AUDIT')):
        raise ValueError('injected loader dependency discovery is unavailable')
    library_path = os.environ.get('LD_LIBRARY_PATH')
    if library_path is not None and '$' in library_path:
        raise ValueError('unknown loader environment search-path variable')
    directories = {Path('/lib'), Path('/usr/lib')}
    if candidates is not None:
        for path in (Path('/etc/ld.so.conf'), Path('/etc/ld.so.cache')):
            candidates[str(path)] = digest(path) if path.is_file() else None
    if library_path is not None:
        directories.update((cwd / p).absolute() for p in library_path.split(':'))
    interpreters = set()
    pending = [Path('/etc/ld.so.conf')]
    visited = set()
    while pending:
        config = pending.pop()
        if config in visited or not config.is_file():
            continue
        visited.add(config)
        files.add(config)
        for line in config.read_text().splitlines():
            line = line.split('#', 1)[0].strip()
            if not line:
                continue
            if line.startswith('include '):
                pattern = line[8:].strip()
                if not Path(pattern).is_absolute():
                    pattern = str(config.parent / pattern)
                directories.add(Path(pattern).parent)
                pending.extend(Path(p) for p in glob.glob(pattern))
            elif line.startswith('/') and len(line.split()) == 1:
                directories.add(Path(line))
            else:
                raise ValueError('unknown loader configuration')
    cache = Path('/etc/ld.so.cache')
    if cache.is_file():
        files.add(cache)
    for path in tuple(files):
        data = path.read_bytes()
        if data[:4] != b'\x7fELF':
            continue
        if data[4:6] != b'\x02\x01':
            raise ValueError('unsupported ELF dependency discovery')
        offset = struct.unpack_from('<Q', data, 32)[0]
        size, count = struct.unpack_from('<HH', data, 54)
        headers = [struct.unpack_from('<IIQQQQQQ', data, offset + i * size) for i in range(count)]
        for header in headers:
            if header[0] == 3:  # PT_INTERP is the kernel-selected loader, not an ldd alias.
                interpreters.add(Path(data[header[2]:header[2] + header[5]].rstrip(b'\0').decode()))
        dynamic = next((h for h in headers if h[0] == 2), None)
        if not dynamic:
            continue
        tags = {}
        for i in range(dynamic[2], dynamic[2] + dynamic[5], 16):
            tag, value = struct.unpack_from('<qQ', data, i)
            if not tag:
                break
            tags[tag] = value
        if not any(tag in tags for tag in (15, 29)):
            continue
        address = tags[5]
        segment = next(h for h in headers if h[0] == 1 and h[3] <= address < h[3] + h[5])
        strings = segment[2] + address - segment[3]
        for tag in (15, 29):
            if tag not in tags:
                continue
            start = strings + tags[tag]
            value = data[start:data.index(b'\0', start)].decode()
            for directory in value.split(':'):
                directory = directory.replace('${ORIGIN}', str(path.parent)).replace('$ORIGIN', str(path.parent))
                if '$' in directory:
                    raise ValueError('unknown ELF search-path variable')
                directories.add((cwd / directory).absolute())
    if not interpreters:
        raise ValueError('unknown native loader')
    legacy = None
    for interpreter in interpreters:
        files.add(interpreter)
        help_text = run_checked([str(interpreter), '--help'], cwd)
        if ('Subdirectories of glibc-hwcaps directories' not in help_text
                or 'Legacy HWCAP subdirectories' not in help_text):
            raise ValueError('unknown native loader search policy')
        directories.update(Path(p) for p in re.findall(
            r'^  (/[^ ]+) \(system search path\)', help_text, re.MULTILINE))
        # Include every ordering of the reported legacy components. The loader
        # may combine them, and absent higher-priority folders matter too.
        components = help_text.split('Legacy HWCAP subdirectories', 1)[1].splitlines()[1:]
        current = [line.strip().split()[0] for line in components if line.startswith('  ') and line.strip()]
        if len(current) > 5 or any('/' in name or name in ('.', '..') for name in current):
            raise ValueError('unknown legacy loader path')
        if legacy is not None and legacy != current:
            raise ValueError('inconsistent native loader policies')
        legacy = current
    for directory in tuple(directories):
        hwcaps = directory / 'glibc-hwcaps'
        directories.add(hwcaps)
        if hwcaps.is_dir():
            directories.update(p for p in hwcaps.iterdir() if p.is_dir())
        for count in range(1, len(legacy) + 1):
            directories.update(directory.joinpath(*parts)
                               for parts in itertools.permutations(legacy, count))
    return directories


def discovery_identity(kind, root, bank, directory):
    signature = hashlib.sha256(encoded({'kind': kind, 'environment': environment(),
        'bank': file_identity_name(Path(bank), root) if bank else None,
        'cwd': file_identity_name(Path.cwd(), root), 'executable': sys.executable,
        'platform': [platform.system(), platform.machine(), platform.release()],
        'cpu': cpu_identity()})).hexdigest()
    path = directory / 'discovery' / kind / (signature + '.json')
    try:
        record = json.loads(path.read_text())
        value = record['identity']
        if (record['signature'] == signature and record['schema'] == 1
                and record['sha256'] == hashlib.sha256(encoded(value)).hexdigest()
                and verify_identity(value, root)):
            return value, path, signature
    except (OSError, ValueError, TypeError, KeyError, AttributeError):
        pass
    return None, path, signature


def save_discovery(path, signature, value):
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix='.discovery-', delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(encoded({'schema': 1, 'signature': signature, 'identity': value,
                                  'sha256': hashlib.sha256(encoded(value)).hexdigest()}))
        try:
            os.replace(temporary, path)
        finally:
            temporary.unlink(missing_ok=True)
    except OSError:
        pass


def probe_python(kind, root, bank):
    """Run in the generator interpreter, with its imports and native libraries."""
    if sys.platform != 'linux':
        raise ValueError('native library discovery requires Linux /proc maps')
    script = {'font': 'gen_font.py', 'drums': 'gen_drum_samples.py',
              'breaks': 'gen_break_bank.py'}[kind]
    sys.path.insert(0, str(root / 'tools'))
    module = runpy.run_path(str(root / 'tools' / script))
    files = {root / 'tools' / script, root / 'tools' / 'asset_cache.py',
             root / 'tools' / 'build.py', Path(sys.executable)}
    settings = {}
    candidates = {}
    import site
    # Startup path selectors are not necessarily imported modules. Inventory
    # their bytes and absent candidates before trusting the saved import closure.
    startup_directories = {Path(p) for p in site.getsitepackages()}
    if site.ENABLE_USER_SITE:
        startup_directories.add(Path(site.getusersitepackages()))
    if any(getattr(finder, '__module__', '') not in
           ('_frozen_importlib', '_frozen_importlib_external') for finder in sys.meta_path):
        raise ValueError('unknown Python import finder')
    if any(getattr(hook, '__module__', '') not in
           ('zipimport', '_frozen_importlib_external') for hook in sys.path_hooks):
        raise ValueError('unknown Python import path hook')
    for folder in startup_directories:
        for startup in folder.glob('*.pth'):
            if any(line.strip().startswith(('import ', 'import\t'))
                   for line in startup.read_text().splitlines()):
                raise ValueError('executable Python startup hook')
            files.add(startup)
    for folder in sys.path:
        files.update(Path(folder or os.getcwd()).glob('*.pkg'))
    for executable in (Path(sys.executable).absolute(), Path(sys.executable).resolve()):
        for config in (executable.parent / 'pyvenv.cfg', executable.parent.parent / 'pyvenv.cfg',
                       executable.parent / (executable.name + '._pth'), executable.parent / 'python._pth'):
            candidates[file_identity_name(config, root)] = digest(config) if config.is_file() else None
            if config.is_file():
                files.add(config)
    if kind == 'font':
        settings = {'fontset': module['FONTSET'],
                    'search': [str(p) for p in module['FONTDIRS']]}
        selected = []
        for _, face, size, _, _, _, _ in module['SIZES']:
            name = face.partition('@')[0]
            path = module['font_path'](name)
            for directory in [module['FONTS'], *module['FONTDIRS']]:
                candidate = (directory / name).absolute()
                candidates[file_identity_name(candidate, root)] = digest(candidate) if candidate.is_file() else None
            files.add(path)
            selected.append(file_identity(path, root)[0])
            if not name.endswith('.bdf'):
                font = module['ImageFont'].truetype(str(path), size)
                font.getlength('A0123')  # Load FreeType, RAQM and their runtime libraries.
        settings['selected'] = selected
        import PIL
        files.update(p for p in Path(PIL.__file__).parent.rglob('*')
                     if p.is_file() and '__pycache__' not in p.parts)
        settings['features'] = {f: module['PILFeatures'].version(f)
                                for f in module['PILFeatures'].get_supported()}
    elif kind == 'drums':
        files.add(root / 'tools' / 'sampleio.py')
        files.update(root / 'assets' / '909' / name for _, name in module['SAMPLES'])
    else:
        folder = Path(bank)
        files.update((root / 'tools' / 'sampleio.py', folder / 'bank.txt'))
        rows = [ln.split() for ln in (folder / 'bank.txt').read_text().splitlines()
                if ln and not ln.startswith('#')]
        files.update(folder / (row[0] + '.wav') for row in rows)
        settings['bank_name'] = folder.name
    # All loaded Python source/extension files, including the standard math modules.
    for imported in tuple(sys.modules.values()):
        path = getattr(imported, '__file__', None)
        if path:
            files.add(Path(path))
    for line in Path('/proc/self/maps').read_text().splitlines():
        fields = line.split(None, 5)
        if len(fields) == 6 and fields[5].startswith('/'):
            files.add(Path(fields[5]))  # A deleted/unreadable library disables reuse.
    # Module and native-library lookup changes must invalidate the discovered closure.
    directories = {Path(p).absolute().parent for p in files}
    directories.update(Path(p or os.getcwd()).absolute() for p in sys.path)
    directories.update(startup_directories)
    for imported in tuple(sys.modules.values()):
        parts = getattr(imported, '__name__', '').split('.')
        for base in sys.path:
            for length in range(1, len(parts) + 1):
                candidate = Path(base or os.getcwd()).joinpath(*parts[:length]).absolute()
                if candidate.is_dir():
                    directories.add(candidate)
        directories.update(Path(p).absolute() for p in getattr(imported, '__path__', ()))
    directories.update(Path(p).absolute() for p in os.environ.get('LD_LIBRARY_PATH', '').split(':') if p)
    for imported in tuple(sys.modules.values()):
        path = getattr(imported, '__file__', None)
        name = getattr(imported, '__name__', '').split('.')[0]
        if path and name and (root / 'tools' / (name + '.py')).is_file():
            if Path(path).resolve() != (root / 'tools' / (name + '.py')).resolve():
                raise ValueError('shadowed interpreter module: ' + name)
    directories.update(loader_inputs(files, candidates=candidates))
    return {'resolution_schema': 2, 'resolutions': resolution_inputs(files, root),
            'files': dict(sorted(file_identity(p, root) for p in files)),
            'lookups': lookup_directories(directories, root), 'candidates': candidates,
            'settings': settings, 'platform': [platform.system(), platform.machine(), platform.release()],
            'python': sys.version}


def run_checked(command, root):
    return subprocess.check_output(command, cwd=root, stderr=subprocess.PIPE, text=True)


def shared_libraries(binary, root):
    output = run_checked(['ldd', str(binary)], root)
    if 'not found' in output:
        raise ValueError('unresolved renderer library')
    paths = re.findall(r'(?:=>\s*)?(/[^\s]+)\s+\(', output)
    if not paths:
        raise ValueError('no renderer library identities')
    return [Path(p) for p in paths]


def builtin_command(root, gen, binary):
    return [*shlex.split(os.environ.get('CC') or 'cc'), '-O2', '-ffp-contract=off',
            '-w', '-Ifirmware/src', '-I' + str(gen), '-o', str(binary),
            'tools/gen_builtin_break.c', 'firmware/src/dsp/drum909.c', '-lm']


def builtin_identity(root, gen, scratch):
    """Discover actual GCC commands, headers, linker inputs and dynamic libraries.

    A small link probe uses the renderer's flags and libm. This avoids compiling
    the DSP on a hit. Unsupported compiler pipelines use the original generator.
    """
    if sys.platform != 'linux':
        raise ValueError('renderer dependency discovery requires Linux')
    cc = shlex.split(os.environ.get('CC') or 'cc')
    if len(cc) != 1:
        raise ValueError('compiler wrapper dependency discovery is unavailable')
    compiler = execution_selector(cc[0], root)
    if compiler.read_bytes()[:4] != b'\x7fELF':
        raise ValueError('compiler wrapper dependency discovery is unavailable')
    executables = {cc[0]: file_identity_name(compiler, root)}
    candidates = {}
    command = builtin_command(root, gen, scratch / 'renderer')
    flags = command[1:6]
    dependencies = run_checked([str(compiler), *flags, '-M',
                               'tools/gen_builtin_break.c', 'firmware/src/dsp/drum909.c'], root)
    files = {root / 'tools' / 'gen_builtin_break.sh', root / 'tools' / 'asset_cache.py',
             root / 'tools' / 'build.py', compiler}
    for rule in dependencies.replace('\\\n', '').splitlines():
        files.update(root / p for p in shlex.split(rule.split(':', 1)[1]))
    # GCC prints the actual compiler/assembler/linker commands without execution.
    plan = subprocess.run([*command, '-###'], cwd=root, capture_output=True,
                          text=True, check=True).stderr
    programs = set()
    for line in plan.splitlines():
        if not line.startswith(' '):
            continue
        args = shlex.split(line)
        if not args:
            continue
        if any(arg.startswith('-fplugin') for arg in args):
            raise ValueError('compiler plugin dependency discovery is unavailable')
        executable = execution_selector(args[0], root)
        executables[args[0]] = file_identity_name(executable, root)
        if executable.read_bytes()[:4] != b'\x7fELF':
            raise ValueError('compiler helper wrapper discovery is unavailable')
        name = executable.name
        name = 'as' if name.endswith('-as') else name
        if name not in ('cc1', 'as', 'collect2'):
            raise ValueError('unsupported compiler pipeline')
        programs.add(name)
        files.add(executable)
        files.update(shared_libraries(executable, root))
        files.update(Path(arg) for arg in args[1:]
                     if arg.startswith('/') and Path(arg).is_file()
                     and not Path(arg).is_relative_to(scratch))
    if programs != {'cc1', 'as', 'collect2'}:
        raise ValueError('incomplete GCC dependency discovery')
    linker = run_checked([str(compiler), '-print-prog-name=ld'], root).strip()
    selected_linker = execution_selector(linker, root)
    executables[linker] = file_identity_name(selected_linker, root)
    linker = selected_linker
    if linker.read_bytes()[:4] != b'\x7fELF':
        raise ValueError('linker wrapper discovery is unavailable')
    files.add(linker)
    files.update(shared_libraries(linker, root))
    files.update(shared_libraries(compiler, root))
    specs = run_checked([str(compiler), '-dumpspecs'], root)
    external_specs = run_checked([str(compiler), '-print-file-name=specs'], root).strip()
    if Path(external_specs).is_file():
        files.add(Path(external_specs))
    source = scratch / 'link-probe.c'
    source.write_text('extern double sin(double); volatile double x;\n'
                      'int main(void) { x = sin(x); return 0; }\n')
    binary = scratch / 'link-probe'
    trace = run_checked([str(compiler), *flags, '-Wl,-t', '-o', str(binary),
                         str(source), '-lm'], root)
    for line in trace.splitlines():
        path = Path(line.strip())
        if path.is_absolute() and path.is_file() and not path.is_relative_to(scratch):
            files.add(path)
    files.update(shared_libraries(binary, root))
    search = subprocess.run([str(compiler), *flags, '-E', '-v', '-x', 'c', '/dev/null'],
                            cwd=root, capture_output=True, text=True, check=True).stderr
    include = search.split('#include <...> search starts here:', 1)[1].split('End of search list.', 1)[0]
    recursive = {root / 'tools', root / 'firmware/src'}
    recursive.update(Path(line.strip()).absolute() for line in include.splitlines() if line.strip())
    directories = recursive | {p.absolute().parent for p in files}
    # GCC searches these paths even when the selected executable/input is elsewhere.
    search_dirs = run_checked([str(compiler), '-print-search-dirs'], root)
    searches = set()
    for line in search_dirs.splitlines():
        if line.startswith(('programs: =', 'libraries: =')):
            searches.add(line.split(':', 1)[0])
            directories.update((root / p).absolute() for p in line.split('=', 1)[1].split(os.pathsep))
    if searches != {'programs', 'libraries'}:
        raise ValueError('unknown GCC search directories')
    for variable in ('PATH', 'COMPILER_PATH', 'LIBRARY_PATH', 'CPATH', 'C_INCLUDE_PATH'):
        if variable in os.environ:
            directories.update((root / p).absolute() for p in os.environ[variable].split(os.pathsep))
    directories.update(Path(p) for p in re.findall(r'ignoring nonexistent directory "([^"]+)"', search))
    directories.update(Path(p).absolute() for p in os.environ.get('LD_LIBRARY_PATH', '').split(':') if p)
    directories.update(loader_inputs(files, root, candidates))
    return {'resolution_schema': 2, 'resolutions': resolution_inputs(files, root),
            'files': dict(sorted(file_identity(p, root) for p in files)),
            'lookups': lookup_directories(directories, root, recursive),
            'recursive': sorted(file_identity_name(p, root) for p in recursive),
            'executables': executables, 'candidates': candidates, 'cc': cc, 'flags': flags[:-1], 'specs': specs,
            'platform': [platform.system(), platform.machine(), platform.release()]}, None


def identity(kind, root, gen, scratch, bank, discovery=None):
    cached = path = signature = None
    if discovery is not None:
        cached, path, signature = discovery_identity(kind, root, bank, discovery)
        if cached is not None:
            return cached, None
    if kind == 'breaks' and not bank:
        result, binary = builtin_identity(root, gen, scratch)
    else:
        data = run_checked(python_command(root / 'tools' / 'asset_cache.py', '--probe',
                                         kind, root, bank or '', scratch=scratch), Path.cwd())
        result, binary = json.loads(data), None
    result['environment'] = environment()
    result['cpu'] = cpu_identity()
    if path is not None:
        save_discovery(path, signature, result)
    return result, binary


def restore(entry, stage, names):
    try:
        manifest = json.loads((entry / 'manifest.json').read_text())
        if (not isinstance(manifest, dict) or manifest.get('key') != entry.name
                or not isinstance(manifest.get('outputs'), dict)
                or set(manifest['outputs']) != set(names)):
            return False
        manifest = manifest['outputs']
        # Read once, verify those exact bytes, then copy. Concurrent replacement is safe.
        data = {name: (entry / name).read_bytes() for name in names}
        if any(hashlib.sha256(data[n]).hexdigest() != manifest[n] for n in names):
            return False
        for name in names:
            (stage / name).write_bytes(data[name])
        return True
    except (OSError, ValueError, TypeError):
        return False


def publish(entry, stage, names):
    try:
        entry.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='.publish-', dir=entry.parent) as tmp:
            pending = Path(tmp) / 'entry'
            pending.mkdir()
            for name in names:
                shutil.copyfile(stage / name, pending / name)
            (pending / 'manifest.json').write_bytes(encoded({'key': entry.name,
                'outputs': {n: digest(pending / n) for n in names}}))
            # A competing complete entry wins. Never modify an entry in place.
            pending.rename(entry)
    except OSError:
        pass


def fallback_inputs(kind, root, gen, bank):
    files = {p for base in ('tools', 'assets', 'firmware/src')
             for p in (root / base).rglob('*')
             if p.is_file() and '__pycache__' not in p.parts}
    if kind == 'breaks':
        files.update(gen / n for n in OUTPUTS['drums'] if (gen / n).exists())
    if bank:
        files.update(p for p in Path(bank).rglob('*') if p.is_file())
    for folder in os.environ.get('X0X_FONT_DIRS', '').split(':'):
        if folder:
            files.update(p for p in Path(folder).rglob('*') if p.is_file())
    return dict(sorted(file_identity(p, root) for p in files))


def generate_asset(kind, root, gen, cache=None):
    root, gen = Path(root).resolve(), Path(gen).resolve()
    names = OUTPUTS[kind]
    bank = os.environ.get('X0X_BREAK_BANK') if kind == 'breaks' else None
    if bank:
        bank = str(Path(bank).absolute())
    gen.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.asset-', dir=gen.parent) as tmp:
        scratch = Path(tmp)
        stage = scratch / 'gen'
        stage.mkdir()
        before = binary = entry = None
        if os.environ.get('X0X_ASSET_CACHE') != '0':
            try:
                directory = cache if cache is not None else cache_directory(root)
                before, binary = identity(kind, root, gen, scratch, bank, discovery=directory)
                key = hashlib.sha256(encoded(before)).hexdigest()
                entry = directory / kind / key
            except (OSError, ValueError, subprocess.SubprocessError, KeyError, IndexError, struct.error, StopIteration):
                before = None
        fallback = fallback_inputs(kind, root, gen, bank) if before is None else None
        hit = entry is not None and restore(entry, stage, names)
        if not hit:
            if kind == 'font':
                command = python_command(root / 'tools/gen_font.py', stage / names[0], scratch=scratch)
            elif kind == 'drums':
                command = python_command(root / 'tools/gen_drum_samples.py', stage / names[0], scratch=scratch)
            elif bank:
                command = python_command(root / 'tools/gen_break_bank.py', bank, stage / names[0], scratch=scratch)
            elif before is not None:
                binary = scratch / 'renderer'
                try:
                    run_checked(builtin_command(root, gen, binary), root)
                except subprocess.CalledProcessError as error:
                    sys.stderr.write(error.stderr or '')
                    raise
                command = [str(binary), str(stage / names[0])]
            else:
                command = ['sh', str(root / 'tools/gen_builtin_break.sh'), str(stage)]
                for name in OUTPUTS['drums']:
                    shutil.copyfile(gen / name, stage / name)
            with ExitStack() as stack:
                if kind == 'breaks' and not bank and before is None:
                    # The original shell script uses one host executable per worktree.
                    host = root / 'build/host'
                    host.mkdir(parents=True, exist_ok=True)
                    lock = stack.enter_context((host / '.asset-renderer.lock').open('a'))
                    fcntl.flock(lock, fcntl.LOCK_EX)
                result = subprocess.run(command, capture_output=True, text=True)
            sys.stdout.write(result.stdout)
            sys.stderr.write(result.stderr)
            if result.returncode:
                raise subprocess.CalledProcessError(result.returncode, command)
        if before is not None:
            # Detect source, lookup, environment or runtime changes during generation/copy.
            try:
                unchanged = verify_identity(before, root)
            except (OSError, ValueError, subprocess.SubprocessError, KeyError, IndexError) as error:
                raise RuntimeError('asset inputs became unavailable: ' + kind) from error
            if not unchanged:
                raise RuntimeError('asset inputs changed: ' + kind)
            if not hit:
                publish(entry, stage, names)
        if fallback is not None and fallback != fallback_inputs(kind, root, gen, bank):
            raise RuntimeError('asset inputs changed: ' + kind)
        # Replace only successfully generated/verified files, never old build outputs.
        for name in names:
            os.replace(stage / name, gen / name)
        print('asset cache: ' + kind + (' hit' if hit else ' miss'))
        return hit


if __name__ == '__main__':
    if len(sys.argv) == 5 and sys.argv[1] == '--probe':
        print(json.dumps(probe_python(sys.argv[2], Path(sys.argv[3]).resolve(), sys.argv[4])))
    else:
        raise SystemExit('asset_cache.py is a build helper')
