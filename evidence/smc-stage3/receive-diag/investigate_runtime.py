from pathlib import Path
import tempfile, subprocess, json, platform

out = Path(__file__).resolve().parent
report = {'platform': platform.uname()._asdict(), 'compiler': subprocess.check_output(['cc','--version'], text=True), 'trivial_runs': []}
with tempfile.TemporaryDirectory() as temp:
    src = Path(temp)/'trivial.c'
    exe = Path(temp)/'trivial'
    src.write_text('int main(void) { return 0; }\n')
    subprocess.run(['cc','-g','-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(src),'-o',str(exe)],check=True)
    for i in range(10):
        r = subprocess.run([str(exe)],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        report['trivial_runs'].append({'attempt':i+1,'exit_code':r.returncode,'output':r.stdout})
(out/'runtime-investigation.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
