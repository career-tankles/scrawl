

import sys
import urllib

for line in sys.stdin:
    line = line.strip()
    if len(line) <= 0 or line.startswith('#'):
        continue
    print line, urllib.quote(line)

