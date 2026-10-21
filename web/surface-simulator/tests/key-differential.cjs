const fs=require('node:fs'),vm=require('node:vm');const ctx={};vm.createContext(ctx);vm.runInContext(fs.readFileSync(__dirname+'/../data.js','utf8')+'\n'+fs.readFileSync(__dirname+'/../model.js','utf8')+'\nglobalThis.M=SurfaceModel;',ctx);const SurfaceModel=ctx.M;
const {execFileSync}=require('node:child_process');
const cases=[];let seed=8145957;const rand=n=>{seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed%n};
for(let i=0;i<10000;i++){let n=48+rand(37),s=rand(24),r=rand(12),snap=rand(3),c=rand(31)-1,inv=rand(4);cases.push([n,s,r,snap,c,inv]);}
const actual=execFileSync(process.argv[2]||__dirname+'/key-oracle',[],{input:cases.map(c=>c.join(' ')).join('\n')+'\n'}).toString().split('\n');
let fail=[];for(let i=0;i<cases.length;i++){let [n,s,r,snap,c,inv]=cases[i],m=new SurfaceModel();m.stage.SCALE=[snap?1:0,r,s,snap===2?1:0];m.stage.CHORD=[c>=0?1:0,Math.max(0,c),inv,0];let got=m.keyNotes(n).join(',');if(got!==actual[i])fail.push({case:cases[i],actual:actual[i],model:got});}
console.log(JSON.stringify({checks:cases.length,failures:fail.length,examples:fail.slice(0,5)},null,2));if(fail.length)process.exitCode=1;
