"""Read-only provenance walker experiment. No build or validation record is issued."""
from pathlib import Path
import os, stat, time, json, sys, importlib.util
spec=importlib.util.spec_from_file_location('baseline',Path(__file__).resolve().parents[2]/'tools/validation_inputs.py')
baseline=importlib.util.module_from_spec(spec);spec.loader.exec_module(baseline)

def direct(path,ancestors=(),exclude=()):
    return walk(Path(path).resolve(strict=True),ancestors,exclude)

def walk(actual,ancestors,exclude):
    if actual in ancestors:
        raise ValueError('dependency symlink cycle: '+str(actual))
    info=actual.stat()
    if stat.S_ISREG(info.st_mode):
        return {'sha256':baseline.file_hash(actual),'executable':bool(info.st_mode & 0o111)}
    if not stat.S_ISDIR(info.st_mode):
        raise ValueError('not a dependency file/directory: '+str(actual))
    data={}
    with os.scandir(actual) as scan:
        entries=sorted(scan,key=lambda entry:entry.name)
    for entry in entries:
        if entry.name in ('.git','__pycache__',*exclude):continue
        child=Path(entry.path)
        if entry.is_symlink():child=child.resolve(strict=True)
        data[entry.name]=walk(child,(*ancestors,actual),exclude)
    return {'sha256':baseline.digest(baseline.encoded(data))}

rows=[]
for path in sys.argv[1:]:
    row={'path':path}
    results=[]
    for name,function in [('baseline',baseline.content),('direct',direct)]:
        start=time.perf_counter();value=function(path);elapsed=time.perf_counter()-start
        row[name+'_seconds']=elapsed;results.append(value)
        print(json.dumps({'path':path,'walker':name,'seconds':round(elapsed,3)}),flush=True)
    assert results[0]==results[1],path
    row['equal']=True;row['content']=results[0];rows.append(row)
Path(__file__).with_name('walk-profile.json').write_text(json.dumps(rows,indent=2)+'\n')
print('PASS: content identities are equal',flush=True)
