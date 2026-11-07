"""Installer package/identity regressions. Supply a local headless package."""
import hashlib
import json
import tempfile
import unittest
import urllib.error
from pathlib import Path
from unittest.mock import patch
from build_idotmatrix_web import add_firmware, verified_package, write_json
import fetch_idotmatrix_release as remote

class InstallerTests(unittest.TestCase):
    package=None

    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='pixel-installer-test-')
        self.root=Path(self.temp.name)
        self.package_dir=self.root/'package';self.package_dir.mkdir()
        for name in ['manifest.json']+[entry['file'] for entry in json.loads((self.package/'manifest.json').read_text())['files']]:
            (self.package_dir/name).write_bytes((self.package/name).read_bytes())
        self.site=self.root/'site';self.site.mkdir();(self.site/'idotmatrix.html').write_text('<html></html>')

    def tearDown(self):self.temp.cleanup()

    def mutate(self,suffix,change):
        record=json.loads((self.package_dir/'manifest.json').read_text())
        entry=next(x for x in record['files'] if x['file'].endswith(suffix))
        data=bytearray((self.package_dir/entry['file']).read_bytes());change(data)
        (self.package_dir/entry['file']).write_bytes(data)
        entry['sha256']=hashlib.sha256(data).hexdigest();entry['size']=len(data)
        write_json(self.package_dir/'manifest.json',record)

    def test_segments_preserve_nvs_and_resolve_relative_paths(self):
        candidate=add_firmware(self.site,self.package_dir)
        self.assertFalse(candidate['published'])
        board=candidate['boards'][0]
        manifest=json.loads((self.site/board['manifest']).read_text())
        for catalog_part,part in zip(board['parts'],manifest['builds'][0]['parts']):
            data=(self.site/board['manifest']).parent/part['path']
            self.assertTrue(data.is_file())
            self.assertEqual(data.read_bytes(),(self.site/catalog_part['path']).read_bytes())
            self.assertFalse(part['offset']<0x89000 and part['offset']+data.stat().st_size>0x9000)
        self.assertEqual([p['offset'] for p in board['parts']],[0,0x89000])

    def test_wrong_project_rejected(self):
        record=json.loads((self.package_dir/'manifest.json').read_text());record['project']='xTouch-CN'
        write_json(self.package_dir/'manifest.json',record)
        with self.assertRaises(ValueError):verified_package(self.package_dir)

    def test_checksum_mismatch_rejected(self):
        entry=json.loads((self.package_dir/'manifest.json').read_text())['files'][0]
        (self.package_dir/entry['file']).write_bytes(b'wrong')
        with self.assertRaises(ValueError):verified_package(self.package_dir)

    def test_populated_nvs_rejected_even_with_new_checksum(self):
        self.mutate('-full.bin',lambda blob:blob.__setitem__(0x9000,0))
        with self.assertRaises(ValueError):verified_package(self.package_dir)

    def test_malformed_layout_rejected(self):
        self.mutate('-full.bin',lambda blob:blob.__setitem__(0x8000+12,1))
        with self.assertRaises(ValueError):verified_package(self.package_dir)

    def test_non_pixel_app_rejected_even_with_new_checksum(self):
        self.mutate('r8n8.bin',lambda blob:blob.__setitem__(slice(80,112),b'xtouch_waveshare'.ljust(32,b'\0')))
        with self.assertRaises(ValueError):verified_package(self.package_dir)

    def test_missing_public_release_disables_installation(self):
        add_firmware(self.site,self.package_dir,published=True)
        with patch.object(remote,'download',side_effect=urllib.error.HTTPError('url',404,'Not Found',{},None)):
            self.assertFalse(remote.fetch(self.site))
        catalog=json.loads((self.site/'idotmatrix-firmware.json').read_text())
        self.assertFalse(catalog['published']);self.assertEqual(catalog['boards'],[])

    def test_incomplete_release_rejected_and_disabled(self):
        release={'tag_name':'1.0.2','draft':False,'prerelease':False,'assets':[]}
        with patch.object(remote,'download',return_value=json.dumps(release).encode()):
            with self.assertRaises(ValueError):remote.fetch(self.site)
        self.assertFalse(json.loads((self.site/'idotmatrix-firmware.json').read_text())['published'])

    def test_verified_release_only_enables_own_images(self):
        record=json.loads((self.package_dir/'manifest.json').read_text());version=record['version']
        names=[v['file'] for v in record['files']]
        blobs={name:(self.package_dir/name).read_bytes() for name in names}
        assets=[{'name':name,'browser_download_url':remote.REPOSITORY+'/releases/download/'+version+'/'+name,'size':len(blob),'digest':'sha256:'+hashlib.sha256(blob).hexdigest()} for name,blob in blobs.items()]
        release={'tag_name':version,'draft':False,'prerelease':False,'assets':assets}
        def download(url,*args):
            url=url.split('?',1)[0]
            if url.endswith('/latest'):return json.dumps(release).encode()
            if url.endswith('/ota.json'):
                app=next(v for v in record['files'] if v['file']==remote.BOARD+'.bin')
                return json.dumps(dict(record,size=app['size'],sha256=app['sha256'])).encode()
            return blobs[url.rsplit('/',1)[1]]
        with patch.object(remote,'download',side_effect=download):self.assertTrue(remote.fetch(self.site))
        result=json.loads((self.site/'idotmatrix-firmware.json').read_text())
        self.assertTrue(result['published']);self.assertEqual(result['project'],'idotmatrix-bambulab')

if __name__=='__main__':
    import argparse,sys
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package',required=True,type=Path)
    args,rest=parser.parse_known_args();InstallerTests.package=args.package
    unittest.main(argv=[sys.argv[0]]+rest)
