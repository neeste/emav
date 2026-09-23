import re

for fn in ['putt/mlmeer.c', 'putt/probe_w.c']:
    with open(fn, 'r') as f:
        content = f.read()
    content = content.replace('void    simpfit();', '')
    with open(fn, 'w') as f:
        f.write(content)
