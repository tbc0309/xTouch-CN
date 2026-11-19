export function installerChannel(testEnabled) {
  return testEnabled ? {id: 'test', index: 'test/firmware.json', prefix: 'test/'} :
    {id: 'main', index: 'firmware.json', prefix: ''};
}

export function validateFirmwareCatalog(data, targets, channel) {
  const versionPattern=channel.id==='test'?/^\d+\.\d+\.\d+-test$/:/^\d+\.\d+\.\d+$/;
  if (!versionPattern.test(data.version) || !Array.isArray(data.boards) ||
      (channel.id === 'test' && (data.channel !== 'test'||data.releaseTag!==data.version||data.prerelease!==true))) throw Error('Invalid firmware channel');
  const ids = new Set();
  for (const board of data.boards) {
    const target = targets[board.id];
    const appUrl = `https://github.com/tbc0309/xTouch-CN/releases/download/${data.version}/${target?.stem}.bin`;
    if (!target || ids.has(board.id) || board.version !== data.version ||
        board.manifest !== target.manifest || !board.manifest.startsWith(channel.prefix) ||
        board.fullPath !== `${channel.prefix}firmware/${target.stem}-full.bin` ||
        board.appUrl !== appUrl || !/^[a-f0-9]{64}$/.test(board.fullSha256))
      throw Error('Firmware does not match the selected board/channel');
    ids.add(board.id);
  }
  if (!data.boards.length) throw Error('No firmware available');
  return data.boards;
}
