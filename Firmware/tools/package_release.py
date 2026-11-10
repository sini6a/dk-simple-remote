"""Create reviewed source and flashable firmware archives; never publish them."""
from pathlib import Path
import hashlib,json,re,zipfile
ROOT=Path(__file__).resolve().parents[1]
ROOT_FILES=['README.md','CHANGELOG.md','CONTRIBUTING.md','LICENSE','THIRD_PARTY_NOTICES.md',
            'Makefile','.gitignore','.gitattributes','sdk.lock.json']
def public_sources():
    files=[ROOT/p for p in ROOT_FILES]
    for directory in ('src','config','tests','tools','docs','LICENSES','.github'):
        files.extend(p for p in (ROOT/directory).rglob('*') if p.is_file()
                     and '__pycache__' not in p.parts and 'archive' not in p.relative_to(ROOT).parts
                     and p.suffix not in ('.pyc','.pyo'))
    return sorted(set(files))
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def archive(path,files):
    with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED) as z:
        for source,name in files:z.write(source,name)
    with zipfile.ZipFile(path) as z:
        if z.testzip():raise SystemExit('Archive CRC verification failed')
def main():
    version=re.search(r'^#define APP_FIRMWARE_VERSION "([A-Za-z0-9_.-]+)"',
                      (ROOT/'config/app_config.h').read_text(),re.M)[1]
    output=ROOT/'build/releases'/version;output.mkdir(parents=True,exist_ok=True)
    firmware=[]
    # rev2 (current PCB) is required; rev1 is added when it has been built.
    boards=[('rev2','')]
    if all((ROOT/'build'/(m+'-rev1')/'firmware-manifest.json').exists() for m in ('release','debug','diagnostic')):
        boards.append(('rev1','-rev1'))
    for board,suffix in boards:
      for mode in ('release','debug','diagnostic'):
        folder=ROOT/'build'/(mode+suffix)
        label=mode+suffix
        manifest_path=folder/'firmware-manifest.json'
        if not manifest_path.exists():raise SystemExit('Build '+label+' before packaging')
        manifest=json.loads(manifest_path.read_text())
        if manifest.get('version')!=version or not manifest.get('inputs_sha256'):
            raise SystemExit('Stale manifest: rebuild '+label)
        if manifest.get('board','rev2')!=board:raise SystemExit('Board mismatch: rebuild '+label)
        for name,sha in manifest['inputs_sha256'].items():
            path=ROOT/name
            if not path.is_file() or digest(path)!=sha:raise SystemExit('Inputs changed: rebuild '+label)
        for name,sha in manifest['artifacts_sha256'].items():
            image=folder/name
            if not image.exists() or digest(image)!=sha:raise SystemExit('Image checksum mismatch: '+label)
            firmware.append((image,label+'/'+name))
        if len(manifest['artifacts_sha256'])!=2:raise SystemExit('Expected HEX and BIN for '+label)
        firmware.append((manifest_path,label+'/firmware-manifest.json'))
    source_zip=output/('ch583-remote-'+version+'-source.zip')
    archive(source_zip,[(p,p.relative_to(ROOT).as_posix()) for p in public_sources()])
    firmware_zip=output/('ch583-remote-'+version+'-firmware.zip')
    notices=[ROOT/'LICENSE',ROOT/'THIRD_PARTY_NOTICES.md',ROOT/'LICENSES/WCH-Apache-2.0.txt',ROOT/'docs/testing.md']
    archive(firmware_zip,firmware+[(p,p.relative_to(ROOT).as_posix()) for p in notices])
    for board,suffix in boards:
        for mode in ('release','diagnostic'):
            for ext in ('hex','bin'):
                p=ROOT/'build'/(mode+suffix)/('ch583-remote-'+version+suffix+'.'+ext)
                dest=output/(mode+'-'+p.name);dest.write_bytes(p.read_bytes())
    assets=[source_zip,firmware_zip,*sorted(output.glob('release-*')),*sorted(output.glob('diagnostic-*'))]
    (output/'SHA256SUMS.txt').write_text(''.join(digest(p)+'  '+p.name+'\n' for p in assets))
    print('Release assets ready:',output.relative_to(ROOT),'boards:',', '.join(b for b,_ in boards))
    print('Source excludes SDK, generated output, local history and recovery archives.')
if __name__=='__main__':main()
