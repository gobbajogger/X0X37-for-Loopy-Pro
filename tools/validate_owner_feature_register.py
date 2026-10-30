#!/usr/bin/env python3
"""Validate the source-inventory crosswalk; does not validate firmware behavior."""
import collections, hashlib, json, pathlib, re
root=pathlib.Path(__file__).resolve().parents[1]
p=root/'docs/owner-requested-features.crosswalk.json'
d=json.loads(p.read_text()); md=(root/'docs/owner-requested-features.md').read_text()
b=d['baseline']; assert [r['id'] for r in b]==list(range(1,123))
assert len({r['heading'] for r in b})==122
expected={'IN ONE-BUILD':24,'ON ANOTHER BRANCH OR PR':45,'MISSING':15,'UNVERIFIED':38}
assert dict(collections.Counter(r['source_coverage'] for r in b))==expected
for r in b: assert f"| {r['id']} | {r['heading']} |" in md
s=d['smc_facets']; assert len(s)==34 and len({r['id'] for r in s})==34
assert {r['id'] for r in s if r['pr699_coverage']=='ABSENT_EXPLICIT_REQUIREMENT'}=={'SMC-04','SMC-05','SMC-30'}
assert all(set(r['pr699_items'])<=set(range(1,123)) for r in s)
for group in ['supplementary_non_smc','cancelled']:
 assert len({r['id'] for r in d[group]})==len(d[group])
assert d['counts']['distinct_top_level_requests']==122+3+sum(r['pr699_coverage']=='absent' for r in d['supplementary_non_smc'])==126
assert d['counts']['additional_detail_facets_not_additive']==31+sum(r['pr699_coverage']!='absent' for r in d['supplementary_non_smc'])==53
assert not ({r['id'] for r in d['supplementary_non_smc']} & {r['id'] for r in d['cancelled']})
for pin in d['pins'].values(): assert re.fullmatch('[0-9a-f]{40}',pin)
for text in [md,p.read_text()]:
 assert '/workspace/' not in text and '/home/' not in text and 'libfile_' not in text and 'Sentinel_' not in text
manifest=json.loads((root/'reference/recovered-ui/MANIFEST.json').read_text())
assert len(manifest['files'])==59
for entry in manifest['files']:
 content=(root/entry['path']).read_bytes()
 assert hashlib.sha256(content).hexdigest()==entry['sha256']
 assert len(content)==entry['bytes'] and b'\0' not in content
 content.decode('utf-8')
assert sum(p.is_file() for p in (root/'reference').rglob('*'))==61
# Public prose/metadata must not expose private conversation message identifiers.
# Reference code fixtures are excluded: their literal protocol identities are not provenance.
private_id=re.compile(r'(?i)[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}|Sentinel_[0-9a-f]+|libfile_[0-9a-f]+|(?:cloud-thread|message)[0-9a-f]{8}')
for public_doc in (root/'docs').rglob('*'):
 if public_doc.is_file() and public_doc.suffix in {'.md','.json'}:
  assert not private_id.search(public_doc.read_text()), f'private identifier in public document: {public_doc.relative_to(root)}'
assert 'NOT_RUN' in md and 'installed' in md and 'approval' in md
print(json.dumps({'result':'PASS','baseline':122,'distinct_top_level_requests':126,'nonadditive_detail_facets':53,'baseline_source_counts':expected,'smc_facets':34,'smc_absent_explicit_699':3,'non_smc_supplementary':len(d['supplementary_non_smc']),'cancelled_outside_active':len(d['cancelled']),'complete_all_chat_coverage':d['coverage_complete']},indent=2))
