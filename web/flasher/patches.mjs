// Backports for the esptool-js 0.5 API used by ESP Web Tools 10.1.0.
// P4 revision decoding follows Espressif esptool 5.6.0, targets/esp32p4.py.
export function replaceRequired(source, before, after) {
  if (!source.includes(before)) throw Error("Installer dependency changed; review patch: " + before);
  return source.replace(before, after);
}
export function patchWebFlash(source) {
  source='import SparkMD5 from "spark-md5";\n'+source;
  source=replaceRequired(source,'        await esploader.main();',
    '        esploader.expectedChipFamily=manifest.builds.length===1?manifest.builds[0].chipFamily:undefined;\n        await esploader.main();');
  source=replaceRequired(source,'            compress: true,',
    '            compress: true,\n            calculateMD5Hash: data=>SparkMD5.hashBinary(data),');
  source=replaceRequired(source,'    const fileArray = [];',
    '    const downloadResults=await Promise.allSettled(filePromises);\n    const fileArray = [];');
  source=replaceRequired(source,'            const data = await filePromises[part];',
    '            const result=downloadResults[part];\n            if(result.status!=="fulfilled")throw result.reason;\n            const data=result.value;\n            if(!data.length)throw Error("Empty firmware image");');
  source=replaceRequired(source,'        return new Promise((resolve) => {',
    '        return new Promise((resolve,reject) => {\n            reader.addEventListener("error",()=>reject(reader.error||Error("Firmware read failed")));\n            reader.addEventListener("abort",()=>reject(Error("Firmware read cancelled")));');
  source=replaceRequired(source,'        await esploader.eraseFlash();',
    `        try {await esploader.eraseFlash();} catch(err) {
            fireStateEvent({state:"error",message:"Erase failed. Keep power connected and retry the complete installation. "+err.message,
                details:{error:"erase_failed",details:err}});
            try {await resetTransport(transport);} finally {await transport.disconnect();}
            return;
        }`);
  source=replaceRequired(source,'            message: "Failed to initialize. Try resetting your device or holding the BOOT button while clicking INSTALL.",',
    '            message: "Connection or chip check failed. Check the selected board and close other serial tools. For S3/P4, power on, hold BOOT, tap RESET and release BOOT, then reconnect. "+err.message,');
  source=replaceRequired(source,'            message: `Your ${chipFamily} board is not supported.`,',
    '            message: `Connected chip: ${chipFamily}. Selected firmware: ${build ? build.chipFamily : manifest.builds.map(b=>b.chipFamily).join(", ")}. Select the matching board. No flash was erased or written.`,');
  source=replaceRequired(source,'                message: err.message,',
    '                message: "Firmware download failed. No flash was erased or written. Check the network and retry. "+err.message,');
  return replaceRequired(source,'            message: err.message,',
    '            message: "Writing or verification failed. Keep power connected and retry the complete installation. "+err.message,');
}
export function patchLoader(source) {
  // ESP32 ROM rejects security-info. Its complete error packet needs no flush;
  // the dependency's flush waits indefinitely for the open reader to close.
  source=replaceRequired(source,'                    await this.flushInput();',
    '                    if(op!==0x14)await this.flushInput();');
  source=replaceRequired(source,
    'const chipMagicValue = (await this.readReg(this.CHIP_DETECT_MAGIC_REG_ADDR)) >>> 0;',
    `let chipMagicValue;
            if(this.expectedChipFamily==="ESP32-P4"){
                const info=await this.checkCommand("get security info",0x14,new Uint8Array(0),0,1000);
                if(!(info instanceof Uint8Array) || info.length<20 ||
                    new DataView(info.buffer,info.byteOffset,info.byteLength).getUint32(12,true)!==18)
                    throw Error("The connected chip does not match the selected P4 board.");
                chipMagicValue=0x0addbad0;
            }else{
                chipMagicValue=(await this.readReg(this.CHIP_DETECT_MAGIC_REG_ADDR))>>>0;
            }`);
  source=replaceRequired(source,'0x18: "16MB",','0x18: "16MB",\n            0x19: "32MB",');
  source=replaceRequired(source,'        await this.runStub();',`        await this.runStub();
        if(this.chip.CHIP_NAME==="ESP32-P4"){
            await this.flashSpiAttach(0);
            const size=await this.getFlashSize();
            if(size!==32768)throw Error("This P4 firmware requires 32MB Flash.");
            const params=new Uint8Array(24),view=new DataView(params.buffer);
            [0,size*1024,65536,4096,256,65535].forEach((value,i)=>view.setUint32(i*4,value,true));
            await this.checkCommand("configure P4 SPI flash",0x0b,params);
        }else{
            const required={"ESP32":4096,"ESP32-S3":16384}[this.chip.CHIP_NAME];
            if(required && await this.getFlashSize()!==required)
                throw Error("Flash capacity does not match this supported board ("+required/1024+"MB required).");
        }`);
  for(const command of ['enter compressed flash mode','enter Flash download mode']){
    const call=`        await this.checkCommand("${command}",`;
    source=replaceRequired(source,call,
      '        if(this.chip.CHIP_NAME==="ESP32-P4" && this.IS_STUB)pkt=this._appendArray(pkt,this._intToByteArray(0));\n'+call);
  }
  return replaceRequired(source,'0x18: 16384,','0x18: 16384,\n            0x19: 32768,');
}
export function patchP4(source) {
  source=replaceRequired(source,'(registerValue >> 27) & 0x07','(registerValue >> 20) & 0x07');
  source=replaceRequired(source,'(registerValue >> 4) & 0x03','(((registerValue >> 23) & 1) << 2) | ((registerValue >> 4) & 0x03)');
  return replaceRequired(source,'    async getPkgVersion(loader) {',
    `    async postConnect(loader) {
        if (await this.getMajorChipVersion(loader)<3)
            throw Error("This P4 firmware requires chip revision v3.0 or newer.");
        loader.CHIP_DETECT_MAGIC_REG_ADDR=this.UART_DATE_REG_ADDR;
        // Match Espressif's ECO6/ECO7 flash power sequence before SPI attach.
        const revision=(await this.getMajorChipVersion(loader))*100+await this.getMinorChipVersion(loader);
        if(revision!==301 && revision!==302)return;
        const dateReg=0x501153fc, ldoReg=0x501151b8, analogReg=0x501151bc;
        if(revision===302 && (await loader.readReg(this.EFUSE_BASE+0x34)&0x10000)){
            const date=await loader.readReg(dateReg);
            if((date&3)===3)await loader.writeReg(dateReg,date&~3);
            return;
        }
        const pause=ms=>new Promise(resolve=>setTimeout(resolve,ms));
        const update=async(address,set,clear=0)=>loader.writeReg(address,((await loader.readReg(address))|set)&~clear);
        await loader.writeReg(0x5011010c,1);
        await pause(10);
        await update(analogReg,1<<27);
        await update(ldoReg,0x80);
        await update(dateReg,3);
        await pause(1);
        await update(analogReg,0,1<<27);
        await update(ldoReg,0,0xff<<23);
        await update(ldoReg,0x80);
        await update(ldoReg,0,0x80);
        await pause(2);
    }
    async getPkgVersion(loader) {`);
}
