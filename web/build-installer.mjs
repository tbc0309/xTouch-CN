import {build} from "esbuild";
import {readFile} from "node:fs/promises";
import {dirname,resolve} from "node:path";
import {fileURLToPath} from "node:url";
import {patchLoader,patchP4,patchWebFlash,replaceRequired} from "./flasher/patches.mjs";
const root=dirname(fileURLToPath(import.meta.url));
export const installerPatches={name:"xtouch-p4-installer",setup(b){
  b.onLoad({filter:/[\\/]esp-web-tools[\\/]dist[\\/](connect|install-dialog)\.js$/},async args=>{
    let source=await readFile(args.path,"utf8");
    if(args.path.endsWith("connect.js"))source=replaceRequired(source,'        alert(err.message);',
      '        alert("Could not open the serial port. Close other serial tools or tabs, reconnect USB and retry. "+err.message);');
    else{
      source=replaceRequired(source,'<ew-icon-button slot="headline" @click=${this._closeDialog}>',
        '<ew-icon-button slot="headline" aria-label="Close installer" @click=${this._closeDialog}>');
      source=replaceRequired(source,'label="Installation complete!"',
        'label="Installation complete! Written data verified."');
      source=replaceRequired(source,'All data on the device will be lost.',
        'Internal settings and touch calibration will be erased. SD card backups are not erased.');
      source=replaceRequired(source,'All data on the device will be erased.',
        'Internal settings and touch calibration will be erased. SD card backups are not erased.');
      source=replaceRequired(source,`This will take
          \${this._installState.chipFamily === "ESP8266"
                ? "a minute"
                : "2 minutes"}.<br />
          Keep this page visible to prevent slow down`,
        'This can take several minutes.<br />Keep this page visible. Do not unplug USB or turn off power.');
    }
    return {contents:source,loader:"js",resolveDir:dirname(args.path)};
  });
  b.onLoad({filter:/[\\/]esp-web-tools[\\/]dist[\\/]flash\.js$/},async args=>({
    contents:patchWebFlash(await readFile(args.path,"utf8")),
    loader:"js",resolveDir:dirname(args.path)
  }));
  b.onLoad({filter:/[\\/]esptool-js[\\/]lib[\\/](esploader|stubFlasher|webserial|reset|targets[\\/]esp32p4)\.js$/},async args=>{
    let source=await readFile(args.path,"utf8");
    if(args.path.endsWith("esploader.js"))source=patchLoader(source);
    else if(args.path.endsWith("esp32p4.js"))source=patchP4(source);
    else if(args.path.endsWith("stubFlasher.js"))source=replaceRequired(source,
      '"./targets/stub_flasher/stub_flasher_32p4.json"',JSON.stringify(resolve(root,"flasher/esp32p4-stub.json")));
    else if(args.path.endsWith("reset.js"))source=replaceRequired(source,
      '        await this.transport.setDTR(false);\n        await this.transport.setRTS(true);',
      `        if(this.transport.getPid()===0x7523){
            // Keep the EN/IO0 transition in one WebSerial call on CH340.
            await this.transport.device.setSignals({dataTerminalReady:false,requestToSend:true});
            await sleep(100);
            await this.transport.device.setSignals({dataTerminalReady:true,requestToSend:false});
            await sleep(this.resetDelay);
            await this.transport.setDTR(false);
            return;
        }
        await this.transport.setDTR(false);
        await this.transport.setRTS(true);`);
    else source=source.replaceAll("this.trace(","this.tracing && this.trace(");
    return {contents:source,loader:"js",resolveDir:dirname(args.path)};
  });
}};
if(process.argv[1] && resolve(process.argv[1])===fileURLToPath(import.meta.url)){
  const result=await build({entryPoints:[resolve(root,"node_modules/esp-web-tools/dist/install-button.js")],bundle:true,format:"esm",minify:true,outdir:resolve(root,"vendor"),plugins:[installerPatches],metafile:true});
  const inputs=Object.keys(result.metafile.inputs).map(path=>path.replaceAll("\\","/"));
  for(const required of ["esp-web-tools/dist/flash.js","esptool-js/lib/esploader.js","flasher/esp32p4-stub.json","spark-md5/spark-md5.js"])
    if(!inputs.some(path=>path.endsWith(required)))throw Error("Installer bundle is missing required source: "+required);
}
