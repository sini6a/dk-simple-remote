"""Fetch the pinned official SDK; preserve existing checkouts and local changes."""
from pathlib import Path
import json,subprocess,sys
ROOT=Path(__file__).resolve().parents[1]
def main():
    lock=json.loads((ROOT/'sdk.lock.json').read_text())
    target=ROOT/lock['directory']
    if not (target/'.git').exists():
        # Preferred: vendor/ch583 is a git submodule of the parent repository.
        in_repo=subprocess.run(['git','-C',str(ROOT),'rev-parse','--is-inside-work-tree'],capture_output=True).returncode==0
        if in_repo:
            subprocess.run(['git','-C',str(ROOT),'submodule','update','--init','--',lock['directory']],check=True)
        else:  # e.g. unpacked source zip without git metadata
            target.parent.mkdir(parents=True,exist_ok=True)
            subprocess.run(['git','clone','--filter=blob:none','--no-checkout',lock['repository'],str(target)],check=True)
            subprocess.run(['git','-C',str(target),'checkout','--detach',lock['revision']],check=True)
    head=subprocess.check_output(['git','-C',str(target),'rev-parse','HEAD'],text=True).strip()
    if head!=lock['revision']:
        sys.exit('SDK revision mismatch. Existing checkout was left unchanged; use a separate clean directory.')
    changes=subprocess.check_output(['git','-C',str(target),'status','--porcelain'],text=True)
    if changes.strip():sys.exit('SDK has local changes; preserve/review them before building.')
    print('SDK ready:',head)
if __name__=='__main__':main()
