"""Check the shipped C lookup against the manufacturer-curve provenance."""
from pathlib import Path
import csv,json,re
root=Path(__file__).resolve().parents[1]
meta=json.loads((root/'docs/data/energizer-cr2032-curve.json').read_text())
rows=list(csv.DictReader((root/'docs/data/energizer-cr2032-curve.csv').open()))
expected=[]
for row in rows:
    pair=[int(row['monotone_mv']),int(row['remaining_percent'])]
    if not expected or expected[-1][0]!=pair[0]: expected.append(pair)
assert expected==meta['lookup']
assert expected[0]==[2200,0] and expected[-1][1]==100
assert all(a[0]<b[0] and a[1]<b[1] for a,b in zip(expected,expected[1:]))
source=(root/'src/power/battery_estimate.c').read_text()
def array(name):
    return [int(x) for x in re.search(name+r'\[\] = \{([^}]+)',source)[1].split(',')]
assert list(map(list,zip(array('volts'),array('levels'))))==expected
assert meta['source'].startswith('https://www.data.energizer.com/')
print('PASS: manufacturer-curve provenance, monotone plateau policy and exact compiled lookup values')
