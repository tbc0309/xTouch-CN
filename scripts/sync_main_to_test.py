"""Merge current local main fixes into test without pushing or discarding changes."""
import pathlib,subprocess
root=pathlib.Path(__file__).resolve().parents[1];git=['git','-c','safe.directory='+root.as_posix(),'-C',str(root)]
def read(*a):return subprocess.check_output(git+list(a),text=True).strip()
if read('branch','--show-current')!='test':raise SystemExit('Run in the test working tree')
if read('status','--porcelain'):raise SystemExit('Commit or preserve test changes before syncing main; no reset performed')
subprocess.run(git+['merge','--no-edit','main'],check=True)
print('Local main merged into test. Run affected builds/regressions; no GitHub synchronization performed.')
