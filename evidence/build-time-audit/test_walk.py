"""Scratch equivalence checks for the read-only tree-walker experiment."""
from pathlib import Path
import tempfile, os
source=Path(__file__).with_name('profile_walk.py').read_text().split('rows=[]',1)[0]
ns={'__file__':str(Path(__file__).with_name('profile_walk.py').resolve())}
exec(compile(source,'profile_walk-definitions','exec'),ns)
old,new=ns['baseline'].content,ns['direct']
count=0

def equal(path,exclude=()):
    global count
    assert old(path,exclude=exclude)==new(path,exclude=exclude),path
    count+=1

with tempfile.TemporaryDirectory() as tmp:
    root=Path(tmp)/'input';root.mkdir();equal(root)
    leaf=root/'leaf';leaf.write_bytes(b'input');equal(root);equal(leaf)
    leaf.chmod(0o755);equal(root)
    leaf.write_bytes(b'changed');equal(root)
    sub=root/'nested';sub.mkdir();(sub/'data').write_bytes(b'bytes');equal(root)
    (root/'alias').symlink_to(sub,target_is_directory=True);equal(root)
    (root/'file-alias').symlink_to(leaf);equal(root)
    (root/'.git').mkdir();(root/'.git'/'ignored').write_text('metadata');equal(root)
    (root/'ignored').mkdir();(root/'ignored'/'data').write_text('ignore');equal(root,('ignored',))
    alias=Path(tmp)/'root-alias';alias.symlink_to(root,target_is_directory=True);equal(alias)
    (sub/'cycle').symlink_to(root,target_is_directory=True)
    for function in [old,new]:
        try:function(root)
        except ValueError as e:assert 'symlink cycle' in str(e)
        else:raise AssertionError('cycle accepted')
    (sub/'cycle').unlink()
    (root/'dangling').symlink_to(root/'absent')
    for function in [old,new]:
        try:function(root)
        except FileNotFoundError:pass
        else:raise AssertionError('dangling symlink accepted')
print('PASS:',count,'identity comparisons, cycle rejection and dangling-link rejection')
