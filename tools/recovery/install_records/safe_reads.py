import os, stat

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

