import re
import os

# Fix simp.c
for d in ['putt', 'emav']:
    path = f"{d}/simp.c"
    if not os.path.exists(path): continue
    with open(path, 'r') as f:
        content = f.read()
    
    pat = re.compile(r'void\s*\nsimpfit\(iniv, npv, mxiter, mniter, pvar, prep, peex\)\s*\nfloat\s*\*iniv;\s*\nint\s*npv, mxiter, mniter;\s*\ndouble\s*\(\*pvar\)\s*\([^)]*\);\s*\nvoid\s*\(\*prep\)\s*\([^)]*\);\s*\nint\s*\(\*peex\)\s*\(\);\s*\n{', re.MULTILINE)
    
    ansi = 'void simpfit(float *iniv, int npv, int mxiter, int mniter, double (*pvar)(float *), void (*prep)(float *), int (*peex)(void))\n{'
    
    content = pat.sub(ansi, content)
    with open(path, 'w') as f:
        f.write(content)

# Fix prototypes in putt.c
def add_prototype(filepath, prototype):
    with open(filepath, 'r') as f:
        content = f.read()
    if prototype not in content:
        # insert after last include
        matches = list(re.finditer(r'^#include .*$', content, re.MULTILINE))
        if matches:
            last = matches[-1]
            idx = last.end()
            content = content[:idx] + '\n' + prototype + content[idx:]
            with open(filepath, 'w') as f:
                f.write(content)

add_prototype('putt/putt.c', 'void Quit_wind(int);')
add_prototype('putt/hear_w.c', 'float get_first_lev(float start, float step);\nfloat get_next_lev(float cur, int yn);')
add_prototype('putt/mlmeer.c', 'void simpfit(float *iniv, int npv, int mxiter, int mniter, double (*pvar)(float *), void (*prep)(float *), int (*peex)(void));')
add_prototype('putt/probe_w.c', 'void simpfit(float *iniv, int npv, int mxiter, int mniter, double (*pvar)(float *), void (*prep)(float *), int (*peex)(void));')

