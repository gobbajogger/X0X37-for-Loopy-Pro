# SPDX-License-Identifier: GPL-3.0-only
import pathlib,subprocess,tempfile,unittest
HERE=pathlib.Path(__file__).resolve().parent
class Scheduler(unittest.TestCase):
 def classify_envelopes(self,source):
  # Execute the actual shell classifier: a pattern hidden in a comment must
  # not count as a serialized generator owner merely because text exists.
  start=source.index('test_class() {');end=source.index('\n}',start)+2
  names=['ui-808-stage-rows','chicago-adsr-graph','ui-adsr-engine-graph',
         'ui-drum-amp-graph','ui-drum-single-lane','ui-settings-sections']
  script=source[start:end]+'\n'+''.join('test_class '+n+'; printf "%s %s\\n" '+n+' "$TEST_CLASS"\n' for n in names)
  result=subprocess.run(['sh','-c',script],text=True,capture_output=True,timeout=5)
  self.assertEqual(result.returncode,0,result.stderr)
  return dict(line.split() for line in result.stdout.splitlines())
 def test_envelope_generators_stay_serialized(self):
  actual=self.classify_envelopes((HERE/'run_tests.sh').read_text())
  self.assertEqual(actual,dict.fromkeys(('ui-808-stage-rows','chicago-adsr-graph',
      'ui-adsr-engine-graph','ui-drum-amp-graph','ui-drum-single-lane','ui-settings-sections'),'G'))
 def test_commented_pattern_mutant_is_parallel(self):
  source=(HERE/'run_tests.sh').read_text();anchor='\n    chicago-adsr-graph|'
  self.assertEqual(source.count(anchor),1)
  actual=self.classify_envelopes(source.replace(anchor,'    chicago-adsr-graph|'))
  self.assertEqual(actual['ui-808-stage-rows'],'G')
  for name in actual:
   if name!='ui-808-stage-rows':self.assertEqual(actual[name],'P')
 def run_case(self,mutant=False,gen_fail=False):
  s=(HERE/'run_tests.sh').read_text();start=s.index('    [ "$cls" = W ] || [ "$cls" = G ] || [ $gw = 1 ]');end=s.index('\n}\nnote()',start);body=s[start:end]
  if mutant:
   body=body.replace('[ "$cls" = W ] || [ "$cls" = G ] || [ $gw = 1 ]','[ "$cls" = W ] || [ $gw = 1 ]').replace('if [ "$cls" = G ]; then gen_lock; read -r _ <&3; fi','[ "$cls" = G ] && gen_lock')
  with tempfile.TemporaryDirectory()as d:
   script='''set -eu
OUT=out;RES=out/results;mkdir -p "$RES" "$OUT/iso";mkfifo tokens;exec 3<>tokens
printf 'x\\nx\\nx\\n' >&3
X0X_PYTHONUSERBASE=/tmp
export X0X_PYTHONUSERBASE
gen_lock(){ until mkdir glock 2>/dev/null;do sleep .01;done; }
gen_unlock(){ rmdir glock; }
run_one(){
 if [ "$2" = G1 ];then echo G1start >>events;sleep .3;echo G1end >>events;
 elif [ "$2" = P ];then echo P >>events;
 else echo "$2" >>events;fi
}
spawn(){ cls=$1;name=$2;key=$2;gw=$3;shift 3
'''+body+'''
}
'''
   if gen_fail:script+='touch out/iso/.gen-fail\nspawn G fail 1 ignored\nspawn P P 0 ignored\n'
   else:script+='spawn G G1 0 ignored\nwhile [ ! -e events ];do sleep .01;done\nspawn G G2 0 ignored\nspawn G G3 0 ignored\nspawn G G4 0 ignored\nspawn P P 0 ignored\n'
   script+='wait\nread -r a <&3; read -r b <&3; read -r c <&3\n'
   r=subprocess.run(['sh','-c',script],cwd=d,capture_output=True,timeout=5);self.assertEqual(r.returncode,0,r.stderr)
   return (pathlib.Path(d)/'events').read_text().splitlines()
 def test_parallel_not_starved(self):
  e=self.run_case();self.assertLess(e.index('P'),e.index('G1end'))
 def test_old_order_mutant_demonstrates_starvation(self):
  e=self.run_case(True);self.assertGreater(e.index('P'),e.index('G1end'))
 def test_generation_failure_no_token_leak(self):self.assertEqual(self.run_case(gen_fail=True),['P'])
 def test_generator_waiters_block_under_contention(self):
  source = (HERE / 'run_tests.sh').read_text()
  locks = source[source.index('gen_lock()'):source.index('asan_links()')]
  script = """set -eu
OUT=build/host
mkdir -p "$OUT/iso"
mkfifo generator
exec 4<>generator
echo >&4
mkdir() {
 if command mkdir "$@"; then return 0; fi
 echo retry >> retries
 return 1
}
""" + locks + """
gen_lock
for worker in 1 2 3 4 5 6 7 8; do
 (touch "ready-$worker"; gen_lock; echo "$worker" >> finished; gen_unlock) &
done
while [ "$#" -ne 8 ]; do set -- ready-*; sleep .01; done
sleep .35
test ! -e finished
gen_unlock
wait
test "$(sort -u finished | wc -l)" -eq 8
if [ -e retries ]; then echo "generator lock polled mkdir $(wc -l < retries) times"; exit 1; fi
"""
  with tempfile.TemporaryDirectory() as directory:
   result = subprocess.run(['sh', '-c', script], cwd=directory, capture_output=True,
                           text=True, timeout=8)
   self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
if __name__=='__main__':unittest.main()
