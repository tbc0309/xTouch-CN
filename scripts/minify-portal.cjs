// Reuse the pinned web build dependency; readable portal sources stay unchanged.
const fs=require('node:fs');
const esbuild=require('../web/node_modules/esbuild');
const loader=process.argv[2];
process.stdout.write(esbuild.transformSync(fs.readFileSync(0,'utf8'),{loader,charset:'utf8',minify:true,target:'es2020',...(loader==='js'?{format:'iife'}:{})}).code);
