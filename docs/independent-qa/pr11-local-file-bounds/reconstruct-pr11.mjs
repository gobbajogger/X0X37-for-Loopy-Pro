// QA byte rendering only. No production qualification, network fetch or publication.
import {readFileSync,writeFileSync,cpSync,readdirSync,realpathSync,existsSync}from'node:fs';
import{join,resolve,relative}from'node:path';import{pathToFileURL}from'node:url';
import{execFileSync}from'node:child_process';import{createHash}from'node:crypto';
const root=realpathSync(process.cwd()),out=resolve(process.argv[2]||'');
const need=(v,s)=>{if(!v)throw Error(s)},sha=b=>createHash('sha256').update(b).digest('hex');
need(execFileSync('git',['rev-parse','HEAD'],{encoding:'utf8'}).trim()==='3ad5d846d850d5c4f6315b695a5079f65c382ef0','wrong head');
need(execFileSync('git',['rev-parse','HEAD^{tree}'],{encoding:'utf8'}).trim()==='60a287591a756a67d3ea705f44c7e4ee3e63889b','wrong tree');
need(!execFileSync('git',['status','--porcelain'],{encoding:'utf8'}).trim(),'dirty source');
need(process.argv[2]&&!existsSync(out)&&relative(root,out).startsWith('../'),'new outside path required');
const raw=readFileSync('/workspace/final-qualified-private/prepared-site/versions.json');
need(sha(raw)==='3fee246257f8298da135835d2e6e5ab4be3c4b5e0d84b403d343fdf76d0f1a49','catalog changed');
const catalog=JSON.parse(raw),{renderInstaller,installerFingerprint}=await import(pathToFileURL(join(root,'scripts/render-qualified-nightly.mjs')));
cpSync(join(root,'docs'),out,{recursive:true});writeFileSync(join(out,'install/index.html'),renderInstaller(catalog));writeFileSync(join(out,'versions.json'),raw);
const paths=[];function scan(d){for(const f of readdirSync(d,{withFileTypes:true})){need(!f.isSymbolicLink(),'symlink');const p=join(d,f.name);if(f.isDirectory())scan(p);else paths.push(relative(out,p));}}scan(out);
const files={};for(const p of paths.sort()){const b=readFileSync(join(out,p));files[p]={size:b.length,sha256:sha(b)};}
const canonical=Object.fromEntries(Object.entries(files).map(([p,v])=>[p,{sha256:v.sha256,size:v.size}]));
const manifest={schema:1,website_head:'3ad5d846d850d5c4f6315b695a5079f65c382ef0',website_tree:'60a287591a756a67d3ea705f44c7e4ee3e63889b',installer_fingerprint:installerFingerprint(),file_count:paths.length,files,files_manifest_sha256:sha(JSON.stringify(canonical)),qa_reconstruction_only:true,production_qualification_rerun:false};
const bytes=JSON.stringify(manifest,null,2)+'\n';writeFileSync('/workspace/pr11-independent-qa/prepared-manifest.json',bytes);
console.log(JSON.stringify({file_count:paths.length,installer_fingerprint:manifest.installer_fingerprint,installer:files['install/index.html'],catalog:files['versions.json'],file_map_digest:manifest.files_manifest_sha256,qa_manifest_sha256:sha(bytes)}));
