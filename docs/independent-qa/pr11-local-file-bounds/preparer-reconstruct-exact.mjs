// Byte reconstruction only: PR11 independent qualification pending; real HTTP bytes checked separately.
// Run from the clean accepted website checkout: node /outside/reconstruct-site.mjs /outside/new-site
import {cpSync,writeFileSync,readFileSync,readdirSync,lstatSync,existsSync,realpathSync} from 'node:fs';
import {resolve,join,relative} from 'node:path';
import {pathToFileURL} from 'node:url';
import {execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';
const need=(v,s)=>{if(!v)throw Error(s)}, sha=b=>createHash('sha256').update(b).digest('hex');
const root=realpathSync(process.cwd()), out=resolve(process.argv[2]||'');
need(execFileSync('git',['rev-parse','HEAD^{tree}'],{encoding:'utf8'}).trim()==='60a287591a756a67d3ea705f44c7e4ee3e63889b','wrong source tree');
need(!execFileSync('git',['status','--porcelain'],{encoding:'utf8'}).trim(),'source not clean');
need(process.argv[2] && !existsSync(out) && relative(root,out).startsWith('../'),'use new outside directory');
const {renderInstaller,installerFingerprint}=await import(pathToFileURL(join(root,'scripts/render-qualified-nightly.mjs')).href);
need(installerFingerprint()==='733add17e2b9df6f8dcdd69a1bed3ec672f99f9165367bc5c8c811c14b8ec2a4','fingerprint changed');
const catalog = {
  "schema": 1,
  "repo": "https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly",
  "pages": "https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/",
  "latest": "nightly-54f2ffbc-dev",
  "versions": [
    {
      "version": "nightly-54f2ffbc-dev",
      "label": "DEV / HARDWARE TEST — DEVICE NOT TESTED",
      "status": "dev-checked",
      "config": "dev",
      "hardware_tested": false,
      "source_commit": "54f2ffbc3124f7449a69067dc8215a2d91a2e675",
      "source_tree": "3b5cadafba33f4c1b116660bc9d19a27a892337c",
      "models": {
        "elite": {
          "product": "SMK-37 Elite_900",
          "sha256": "d14877d6592e6dc158460e960ac5aefd1b2b775588b9f896d3c83f672d61c86c",
          "size": 651396,
          "controller_sha256": "956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a",
          "pkg": "https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/172d99d6d417d72bbd8f473ec1c66d5994c3d0c3/artifacts/nightly/dev-20261024-dc466908/d14877d6592e6dc158460e960ac5aefd1b2b775588b9f896d3c83f672d61c86c/x0x37-elite-dev.fwsc"
        },
        "pro": {
          "product": "SMK-37 Pro_900",
          "sha256": "cde289d422b61ee8b1f0a9f04ca92d244f07856667c85d757b9563e0479d3f3c",
          "size": 651392,
          "controller_sha256": "956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a",
          "pkg": "https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/172d99d6d417d72bbd8f473ec1c66d5994c3d0c3/artifacts/nightly/dev-20261024-dc466908/cde289d422b61ee8b1f0a9f04ca92d244f07856667c85d757b9563e0479d3f3c/x0x37-pro-dev.fwsc"
        }
      },
      "title": "DEV / HARDWARE TEST — Elite and Pro",
      "description": "Software-checked DEV firmware from accepted NIGHTLY source. DEVICE NOT TESTED. Requires the exact matching owner-supplied local controller; no controller is hosted. Browser-route qualification only; not the legacy native CLI or bundled installer.",
      "evidence_url": "https://api.github.com/repos/gobbajogger/x0x37-for-Loopy-Pro_nightly/pulls/10/reviews/5638858728"
    }
  ],
  "history": []
};
cpSync(join(root,'docs'),out,{recursive:true});
writeFileSync(join(out,'install/index.html'),renderInstaller(catalog));
writeFileSync(join(out,'versions.json'),JSON.stringify(catalog,null,2)+'\n');
const paths=[]; function scan(d){for(const e of readdirSync(d,{withFileTypes:true})){const p=join(d,e.name);need(!e.isSymbolicLink(),'symlink in output');if(e.isDirectory())scan(p);else if(e.isFile())paths.push(relative(out,p));}}scan(out);
const files={};for(const p of paths.sort()){const b=readFileSync(join(out,p));files[p]={size:b.length,sha256:sha(b)};}
need(paths.length===115,'wrong file count');
need(files['install/index.html'].sha256==='5c51f29146e137ceed2e104ea3787fbcd324ad5cbb30deffb30e812accdfb0d3','installer differs');
need(files['versions.json'].sha256==='3fee246257f8298da135835d2e6e5ab4be3c4b5e0d84b403d343fdf76d0f1a49','catalog differs');
const canonical=Object.fromEntries(Object.entries(files).map(([p,v])=>[p,{sha256:v.sha256,size:v.size}]));
const mapHash=sha(JSON.stringify(canonical));
need(mapHash==='ca290a85d8f613db5c39e9757bc8a922fca148362ec4374b4563cafcac8db24a','full output map differs');
const manifest={schema:1,website_head:'3ad5d846d850d5c4f6315b695a5079f65c382ef0',website_tree:'60a287591a756a67d3ea705f44c7e4ee3e63889b',installer_fingerprint:installerFingerprint(),renderer_exit_code:null,file_count:115,files,files_manifest_sha256:mapHash,only_changes_from_source_docs:['install/index.html','versions.json'],source_clean_after_render:true,history_count:122,controller_bytes_in_output:false,qualification_in_output:false,qualified_real_package_browser_qa:'PENDING'};
need(sha(JSON.stringify(manifest,null,2)+'\n')==='7857e5b2784f03f7bdcc9c9f9be6d16b23b31f1d9b7ce1e28622e65ff8b81d7d','full original manifest differs');
console.log('RECONSTRUCTED: 115 files and PR11 candidate full manifest match. No new qualification, HTTP or browser PASS asserted.');
