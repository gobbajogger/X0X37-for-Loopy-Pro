#!/usr/bin/env python3
"""Check every owned snapshot scalar against Clang's actual declared types.

This is a mechanical producer coverage proof, not semantic engine admission or
an independent review. Only owned field paths/types and digests are exported.
"""
import hashlib,json,pathlib,re,subprocess,sys
root=pathlib.Path(__file__).resolve().parents[1]
source='#include "miami_voice_snapshot.h"\n#include "chicago_voice_snapshot.h"\n'
command=['cc','-std=c99','-Xclang','-ast-dump=json','-fsyntax-only','-Iexperiments/original_synth','-Iexperiments/wobble','-x','c','-']
result=subprocess.run(command,cwd=root,input=source,text=True,capture_output=True,check=True)
ast=json.loads(result.stdout);records={};aliases={}
def walk(node):
 if node.get('kind')=='RecordDecl'and node.get('completeDefinition'):records[node['id']]=node
 if node.get('kind')=='TypedefDecl':
  def find(item):
   owned=item.get('ownedTagDecl')or item.get('decl')
   if owned and owned.get('kind')=='RecordDecl':return owned['id']
   for child in item.get('inner',[]):
    value=find(child)
    if value:return value
  value=find(node)
  if value:aliases[node['name']]=value
 for child in node.get('inner',[]):walk(child)
walk(ast)
def flatten(type_name,path=''):
 match=re.fullmatch(r'(.+)\[(\d+)\]',type_name)
 if match:
  base,count=match.groups();return sum((flatten(base.strip(),path+'['+str(i)+']')for i in range(int(count))),[])
 if type_name in aliases:
  record=records[aliases[type_name]];out=[]
  for field in record.get('inner',[]):
   if field.get('kind')!='FieldDecl':continue
   typ=field['type']['qualType'];name=field['name'];out+=flatten(typ,(path+'.'if path else'')+name)
  return out
 kinds={'uint32_t':'U','unsigned int':'U','int32_t':'I','int':'I','uint8_t':'B','unsigned char':'B','float':'F'}
 assert type_name in kinds,(path,type_name)
 return [(path,kinds[type_name],type_name)]
report=[]
for engine in ['miami','chicago']:
 typename='original_'+engine+'_voice_snapshot';declared=flatten(typename)
 p=root/'experiments/original_synth'/f'{engine}_voice_snapshot.c'
 schema=[(path,kind)for kind,path in re.findall(r' FIELD\(([A-Z]),([^)]*)\)',p.read_text())]
 assert len(schema)==len(set(path for path,kind in schema))
 expected={(path,kind)for path,kind,typ in declared}
 assert set(schema)==expected,{'missing':sorted(expected-set(schema)),'extra':sorted(set(schema)-expected)}
 report.append({'type':typename,'scalar_fields':len(declared),'all_fields_covered_once':True,'no_pointer_or_padding_encoded':True,'field_types':declared,'source_sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
out={'scope':'Compiler AST mechanical field/type coverage, not review/admission/target ABI','compiler':subprocess.check_output(['cc','--version'],text=True).splitlines()[0],'records':report}
if len(sys.argv)>1:pathlib.Path(sys.argv[1]).write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps([{k:v for k,v in row.items()if k!='field_types'}for row in report]))
